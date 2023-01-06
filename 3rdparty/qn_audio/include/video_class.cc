#include "../config.h"
#include "simple_client.h"
#include "json.hpp"

#include <signal.h>
#include <mutex>
#include <map>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;
using namespace qiniu;
using json = nlohmann::json;

enum PubState
{
  kUnpublished,
  kPublishing,
  kPublished
};

struct VideoTalkContext
{
  qiniu::QNCameraVideoTrack *video_track = nullptr;
  qiniu::QNMicrophoneAudioTrack *audio_track = nullptr;

  bool joined = false;
  PubState camera_published = kUnpublished;
  PubState mic_published = kUnpublished;
};

class PubCallback : public QNPublishResultCallback
{
private:
  VideoTalkContext *user_data_ = nullptr;
  bool is_camera_ = false;

public:
  PubCallback(VideoTalkContext *user_data, bool is_camera)
      : user_data_(user_data), is_camera_(is_camera){};

  void OnPublished() override
  {
    if (is_camera_)
    {
      user_data_->camera_published = kPublished;
    }
    else
    {
      user_data_->mic_published = kPublished;
    }
  }

  void OnPublishError(int error_code, const string &error_message) override
  {
    if (is_camera_)
    {
      user_data_->camera_published = kUnpublished;
    }
    else
    {
      user_data_->mic_published = kUnpublished;
    }
  }
};

class VideoTalk : public SimpleClient
{
private:
  VideoTalkContext ctx_;
  unique_ptr<PubCallback> camera_pub_callback_ = nullptr;
  unique_ptr<PubCallback> mic_pub_callback_ = nullptr;

public:
  explicit VideoTalk()
  {
    camera_pub_callback_ =
        std::unique_ptr<PubCallback>(new PubCallback(&ctx_, true));
    mic_pub_callback_ =
        std::unique_ptr<PubCallback>(new PubCallback(&ctx_, false));
  }

  ~VideoTalk()
  {
    VideoTalk::Leave();
    this_thread::sleep_for(chrono::milliseconds(500));
  }

  void Join(string token, string user_data) override
  {
    if (ctx_.joined)
    {
      return;
    }
    client_->SetAutoSubscribe(false);
    client_->Join(token, user_data);
  }

  void PublishCameraTrack(bool multi_stream)
  {
    if (!ctx_.joined)
    {
      return;
    }
    if (ctx_.camera_published != kUnpublished)
    {
      return;
    }
    LocalTrackList local_track_list;

    // camera video track
    // select camera
    int count = QNRTC::GetCameraCount();
    assert(count != 0);
    QNCameraInfo target_camera;
    int target_cap_size = 0;
    for (int i = 0; i < count; ++i)
    {
      auto camera_info = QNRTC::GetCameraInfo(i);
      if (camera_info.capabilities.size() > target_cap_size)
      {
        target_camera = camera_info;
      }
    }
    QNCameraCapability target_camera_cap;
    for (int i = 0; i < target_camera.capabilities.size(); ++i)
    {
      if (target_camera.capabilities[i].video_frame_type ==
          QNVideoFrameType::kI420)
      {
        target_camera_cap = target_camera.capabilities[i];
        break;
      }
    }
    auto cap = target_camera_cap;
    cout << cap.width << "," << cap.height << "," << cap.max_fps << ","
         << cap.video_frame_type << endl;
    QNCameraVideoTrackConfig video_track_config;
    video_track_config.id = target_camera.id;
    video_track_config.capture_config = {cap.width, cap.height, cap.max_fps};
    video_track_config.encoder_config = {cap.width, cap.height, cap.max_fps,
                                         cap.width * cap.height * 2};
    video_track_config.multi_profile_enabled = multi_stream;
    ctx_.video_track = QNRTC::CreateCameraVideoTrack(video_track_config);
    local_track_list.push_front(ctx_.video_track);

    client_->Publish(local_track_list, camera_pub_callback_.get());
    ctx_.camera_published = kPublishing;
  }

  void UnpublishCameraTrack()
  {
    if (ctx_.camera_published == kPublished)
    {
      LocalTrackList local_track_list;
      local_track_list.push_front(ctx_.video_track);
      client_->UnPublish(local_track_list);
      if (ctx_.video_track != nullptr)
      {
        this_thread::sleep_for(chrono::milliseconds(100));
        QNRTC::DestroyLocalTrack(ctx_.video_track);
      }
      ctx_.video_track = nullptr;
      ctx_.camera_published = kUnpublished;
    }
  }

  void PublishMicTrack()
  {
    if (!ctx_.joined)
    {
      return;
    }
    if (ctx_.mic_published != kUnpublished)
    {
      return;
    }
    LocalTrackList local_track_list;

    // microphone audio track
    if (ctx_.audio_track == nullptr)
    {
      QNMicrophoneAudioTrackConfig audio_track_config;
      audio_track_config.audio_quality = {44100, 2, 16, 44100};
      ctx_.audio_track = QNRTC::CreateMicrophoneAudioTrack(audio_track_config);
    }
    local_track_list.push_front(ctx_.audio_track);
    client_->Publish(local_track_list, mic_pub_callback_.get());
    ctx_.mic_published = kPublishing;
  }

  void UnpublishMicTrack()
  {
    if (ctx_.mic_published == kPublished)
    {
      LocalTrackList local_track_list;
      local_track_list.push_front(ctx_.audio_track);
      client_->UnPublish(local_track_list);
      if (ctx_.audio_track != nullptr)
      {
        this_thread::sleep_for(chrono::milliseconds(100));
        QNRTC::DestroyLocalTrack(ctx_.audio_track);
      }
      ctx_.audio_track = nullptr;
      ctx_.mic_published = kUnpublished;
    }
  }

  void SubsribeAll()
  {
    if (ctx_.joined)
    {
      RemoteTrackList remote_track_list;
      auto remote_users = client_->GetRemoteUsers();
      for (auto remote_user : remote_users)
      {
        for (auto remote_audio_track : remote_user.remote_audio_track_list)
        {
          if (!remote_audio_track->IsSubscribed())
          {
            remote_track_list.push_front(remote_audio_track);
          }
        }
        for (auto remote_video_track : remote_user.remote_video_track_list)
        {
          if (!remote_video_track->IsSubscribed())
          {
            remote_track_list.push_front(remote_video_track);
          }
        }
      }
      if (!remote_track_list.empty())
      {
        client_->Subscribe(remote_track_list);
      }
    }
  }

  void UnsubsribeAll()
  {
    RemoteTrackList remote_track_list;
    auto remote_users = client_->GetRemoteUsers();
    for (auto remote_user : remote_users)
    {
      for (auto remote_audio_track : remote_user.remote_audio_track_list)
      {
        if (remote_audio_track->IsSubscribed())
        {
          remote_track_list.push_front(remote_audio_track);
        }
      }
      for (auto remote_video_track : remote_user.remote_video_track_list)
      {
        if (remote_video_track->IsSubscribed())
        {
          remote_track_list.push_front(remote_video_track);
        }
      }
    }
    if (!remote_track_list.empty())
    {
      client_->UnSubscribe(remote_track_list);
    }
  }

  void MuteMicTrack()
  {
    if (ctx_.audio_track != nullptr)
    {
      ctx_.audio_track->SetMuted(true);
    }
  }

  void UnmuteMicTrack()
  {
    if (ctx_.audio_track != nullptr)
    {
      ctx_.audio_track->SetMuted(false);
    }
  }

  void Leave() override
  {
    UnsubsribeAll();
    UnpublishCameraTrack();
    UnpublishMicTrack();
    if (ctx_.joined)
    {
      client_->Leave();
      ctx_.joined = false;
    }
  }

  void State()
  {
    cout << "join state:" << ctx_.joined << endl;
    cout << "camera publish state:" << ctx_.camera_published << endl;
    cout << "mic publish state:" << ctx_.mic_published << endl;

    auto remote_users = client_->GetRemoteUsers();
    for (auto remote_user : remote_users)
    {
      for (auto remote_audio_track : remote_user.remote_audio_track_list)
      {
        cout << remote_audio_track->GetTrackID() << ":"
             << remote_audio_track->IsSubscribed() << endl;
      }
      for (auto remote_video_track : remote_user.remote_video_track_list)
      {
        cout << remote_video_track->GetTrackID() << ":"
             << remote_video_track->IsSubscribed() << endl;
      }
    }
  }

  bool GetState()
  {
    return ctx_.joined;
  }

protected:
  void OnConnectionStateChanged(
      qiniu::QNConnectionState state,
      const qiniu::QNConnectionDisconnectedInfo *info) override
  {
    cout << "------------------OnConnectionStateChanged state:" << state
         << endl;
    // 连接状态变化
    if (state == QNConnectionState::kConnected ||
        state == QNConnectionState::kReconnected)
    {
      ctx_.joined = true;
    }
    else
    {
      ctx_.joined = false;
    }
  }
};

struct ControllerData
{
  /* data */
  std::map<std::string, json> GetData;
  std::map<std::string, json> SetData;
  void _init()
  {
    GetData = {{"stateStreamService", {"state", -1}},
               {"statePushFlow", {"state", -1}},
               {"stateSip", {"state", -1}},
               {"stateVideoSlice", {{"state", -1}, {"addr", ""}}}};
    SetData = {{"switchStreamService", {"switch", -1}},
               {"switchVideoSlice", {"switch", -1}},
               {"switchPushFlow", {{"switch", -1}, {"addr", ""}}},
               {"switchSip", {{"switch", -1}, {"token", ""}}}};
  }
};