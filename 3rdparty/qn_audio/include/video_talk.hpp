//#include "../config.h"
#pragma once
#include "simple_client.h"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unistd.h>

using namespace std;
using namespace qiniu;

enum PubState { kUnpublished, kPublishing, kPublished };

struct VideoTalkContext {
  qiniu::QNCameraVideoTrack *video_track = nullptr;
  qiniu::QNMicrophoneAudioTrack *audio_track = nullptr;

  bool joined = false;
  PubState camera_published = kUnpublished;
  PubState mic_published = kUnpublished;
};

class PubCallback : public QNPublishResultCallback {
public:
  PubCallback(VideoTalkContext *user_data, bool is_camera)
      : user_data_(user_data), is_camera_(is_camera){};

  void OnPublished() override {
    if (is_camera_) {
      user_data_->camera_published = kPublished;
    } else {
      user_data_->mic_published = kPublished;
    }
  }

  void OnPublishError(int error_code, const string &error_message) override {
    if (is_camera_) {
      user_data_->camera_published = kUnpublished;
    } else {
      user_data_->mic_published = kUnpublished;
    }
  }

private:
  VideoTalkContext *user_data_ = nullptr;
  bool is_camera_ = false;
};

class VideoTalk : public SimpleClient {

public:
  explicit VideoTalk() {
    camera_pub_callback_ =
        std::unique_ptr<PubCallback>(new PubCallback(&ctx_, true));
    mic_pub_callback_ =
        std::unique_ptr<PubCallback>(new PubCallback(&ctx_, false));
  }

  ~VideoTalk() {
    VideoTalk::Leave();
    this_thread::sleep_for(chrono::milliseconds(500));
  }

  void Join(string token, string user_data) override {
    if (ctx_.joined) {
      return;
    }
    client_->SetAutoSubscribe(true);
    client_->Join(token, user_data);
  }

  void PublishCameraTrack(bool multi_stream) {
    if (!ctx_.joined) {
      return;
    }
    if (ctx_.camera_published != kUnpublished) {
      return;
    }
    LocalTrackList local_track_list;

    // camera video track
    // select camera
    int count = QNRTC::GetCameraCount();
    assert(count != 0);
    QNCameraInfo target_camera;
    int target_cap_size = 0;
    for (int i = 0; i < count; ++i) {
      auto camera_info = QNRTC::GetCameraInfo(i);
      if (camera_info.capabilities.size() > target_cap_size) {
        target_camera = camera_info;
      }
    }
    QNCameraCapability target_camera_cap;
    for (int i = 0; i < target_camera.capabilities.size(); ++i) {
      if (target_camera.capabilities[i].video_frame_type ==
          QNVideoFrameType::kI420) {
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

  void UnpublishCameraTrack() {
    if (ctx_.camera_published == kPublished) {
      LocalTrackList local_track_list;
      local_track_list.push_front(ctx_.video_track);
      client_->UnPublish(local_track_list);
      if (ctx_.video_track != nullptr) {
        this_thread::sleep_for(chrono::milliseconds(100));
        QNRTC::DestroyLocalTrack(ctx_.video_track);
      }
      ctx_.video_track = nullptr;
      ctx_.camera_published = kUnpublished;
    }
  }

  void PublishMicTrack() {
    if (!ctx_.joined) {
      return;
    }
    if (ctx_.mic_published != kUnpublished) {
      return;
    }
    LocalTrackList local_track_list;

    // microphone audio track
    if (ctx_.audio_track == nullptr) {
      QNMicrophoneAudioTrackConfig audio_track_config;
      audio_track_config.audio_quality = {44100, 2, 16, 44100};
      ctx_.audio_track = QNRTC::CreateMicrophoneAudioTrack(audio_track_config);
    }
    local_track_list.push_front(ctx_.audio_track);
    client_->Publish(local_track_list, mic_pub_callback_.get());
    ctx_.mic_published = kPublishing;
  }

  void UnpublishMicTrack() {
    if (ctx_.mic_published == kPublished) {
      LocalTrackList local_track_list;
      local_track_list.push_front(ctx_.audio_track);
      client_->UnPublish(local_track_list);
      if (ctx_.audio_track != nullptr) {
        this_thread::sleep_for(chrono::milliseconds(100));
        QNRTC::DestroyLocalTrack(ctx_.audio_track);
      }
      ctx_.audio_track = nullptr;
      ctx_.mic_published = kUnpublished;
    }
  }

  void SubsribeAll() {
    if (ctx_.joined) {
      RemoteTrackList remote_track_list;
      auto remote_users = client_->GetRemoteUsers();
      for (auto remote_user : remote_users) {
        for (auto remote_audio_track : remote_user.remote_audio_track_list) {
          if (!remote_audio_track->IsSubscribed()) {
            remote_track_list.push_front(remote_audio_track);
          }
        }
        for (auto remote_video_track : remote_user.remote_video_track_list) {
          if (!remote_video_track->IsSubscribed()) {
            remote_track_list.push_front(remote_video_track);
          }
        }
      }
      if (!remote_track_list.empty()) {
        client_->Subscribe(remote_track_list);
      }
    }
  }

  void UnsubsribeAll() {
    RemoteTrackList remote_track_list;
    auto remote_users = client_->GetRemoteUsers();
    for (auto remote_user : remote_users) {
      for (auto remote_audio_track : remote_user.remote_audio_track_list) {
        if (remote_audio_track->IsSubscribed()) {
          remote_track_list.push_front(remote_audio_track);
        }
      }
      for (auto remote_video_track : remote_user.remote_video_track_list) {
        if (remote_video_track->IsSubscribed()) {
          remote_track_list.push_front(remote_video_track);
        }
      }
    }
    if (!remote_track_list.empty()) {
      client_->UnSubscribe(remote_track_list);
    }
  }
  void SetAutoSubscribe(bool autosub){
  	client_->SetAutoSubscribe(true);
  }

  void MuteMicTrack() {
    if (ctx_.audio_track != nullptr) {
      ctx_.audio_track->SetMuted(true);
    }
  }

  void UnmuteMicTrack() {
    if (ctx_.audio_track != nullptr) {
      ctx_.audio_track->SetMuted(false);
    }
  }

  void Leave() override {
    UnsubsribeAll();
    UnpublishCameraTrack();
    UnpublishMicTrack();
    if (ctx_.joined) {
      client_->Leave();
      ctx_.joined = false;
    }
  }

  void State() {
    cout << "join state:" << ctx_.joined << endl;
    cout << "camera publish state:" << ctx_.camera_published << endl;
    cout << "mic publish state:" << ctx_.mic_published << endl;

    auto remote_users = client_->GetRemoteUsers();
    for (auto remote_user : remote_users) {
      for (auto remote_audio_track : remote_user.remote_audio_track_list) {
        cout << remote_audio_track->GetTrackID() << ":"
             << remote_audio_track->IsSubscribed() << endl;
      }
      for (auto remote_video_track : remote_user.remote_video_track_list) {
        cout << remote_video_track->GetTrackID() << ":"
             << remote_video_track->IsSubscribed() << endl;
      }
    }
  }
  bool getState(){
  	return ctx_.joined;
  }

protected:
  void OnConnectionStateChanged(
      qiniu::QNConnectionState state,
      const qiniu::QNConnectionDisconnectedInfo *info) override {
    cout << "------------------OnConnectionStateChanged state:" << state << endl;
	cout << "------------------ ctx_.joined " << ctx_.joined << "\n";
    // ??????????????????
	if (state == kDisconnected && info->reason== QNConnectionDisconnectedInfo::kKickOut){
		// 
		std::cout<<"---------------------------------- Leave kKickOut ----------------------------"<<std::endl;
	}
	if (state == kDisconnected && info->reason== QNConnectionDisconnectedInfo::kRoomClosed){
		// 
		std::cout<<"---------------------------------- Leave kRoomClosed----------------------------"<<std::endl;
	}
	if (state == kDisconnected && info->reason== QNConnectionDisconnectedInfo::kRoomFull){
		// 
		std::cout<<"---------------------------------- Leave kRoomFull ----------------------------"<<std::endl;
	}
	if (state == kDisconnected && info->reason== QNConnectionDisconnectedInfo::kRoomError){
		// 
		std::cout<<"---------------------------------- Leave kRoomError----------------------------"<<std::endl;
	}
	if (state == QNConnectionState::kConnected || state == QNConnectionState::kReconnected)
	{
		ctx_.joined = true;
		PublishMicTrack();
		SubsribeAll();
	} else {
		ctx_.joined = false;
	}
  }
  void OnUserPublished(const string &remote_user_id,const RemoteTrackList &track_list) {
	  cout << "------------------OnUserPublished user:" << remote_user_id << endl;
	  SubsribeAll();
  }
  void OnUserJoined(const string &remote_user_id,
		  const string &user_data) {
	  cout << "------------------OnUserJoined user:" << remote_user_id << endl;
	  SubsribeAll();

  }

  void OnUserLeft(const string &remote_user_id) {
	  cout << "------------------OnUserLeft user:" << remote_user_id << endl;
	  SubsribeAll();

  }



private:
  VideoTalkContext ctx_;
  unique_ptr<PubCallback> camera_pub_callback_ = nullptr;
  unique_ptr<PubCallback> mic_pub_callback_ = nullptr;
};

//void print_cmd() {
//	cout << "-------------------------------------" << endl;
//	cout << "| ??????????????????                             |" << endl;
//	cout << "| join:               ????????????             |" << endl;
//	cout << "| leave:              ????????????             |" << endl;
//	cout << "| pub_camera:         ???????????????           |" << endl;
//	cout << "| pub_camera_m:       ????????????????????????     |" << endl;
//	cout << "| unpub_camera:       ?????????????????????       |" << endl;
//	cout << "| pub_mic:            ???????????????           |" << endl;
//	cout << "| unpub_mic:          ?????????????????????       |" << endl;
//	cout << "| sub:                ???????????????           |" << endl;
//	cout << "| unsub:              ?????????????????????       |" << endl;
//	cout << "| mute:               ??????                 |" << endl;
//	cout << "| unmute:             ????????????             |" << endl;
//	cout << "| state:              ????????????             |" << endl;
//	cout << "| cmd:                ????????????             |" << endl;
//	cout << "| exit:               ??????                 |" << endl;
//	cout << "-------------------------------------" << endl;
//}

/*int main(int argc, char *argv[]) {
  Config config;
  assert(!config.app.app_id.empty());
  assert(!config.app.room_name.empty());

  string version;
  QNRTC::GetVersion(version);
  cout << "SDK ??????:" << version << endl;

  QNRTCSetting setting;
  QNRTC::Init(setting, nullptr);
  QNRTC::SetLogFile(QNLogLevel::kLogInfo, config.app.log_dir,
  "qn-rtc-demo.log");

  std::string token;
  GetRoomToken_s(config.app.app_id, config.app.room_name, "linux_demo_video_talk",
  "api-demo.qnsdk.com", 10000, token);
  unique_ptr<VideoTalk> video_talk = unique_ptr<VideoTalk>(new VideoTalk());
  print_cmd();

  token = "QOlAiGjncrTvoUWz7zJ9TtSVI6MG2eKyg01nzyuq:gLTa4O1X4sl-oFDo7TQ3OwQZuTE=:eyJhcHBJZCI6Imdtb2ZmNWQ0ayIsInJvb21OYW1lIjoiMTIzNDU2Nzg5IiwidXNlcklkIjoiMTIzNDU2Nzg5IiwiZXhwaXJlQXQiOjE2NjU1NTM4NDIsInBlcm1pc3Npb24iOiJ1c2VyIn0=";
  while (true) {

  string cmd;
  cin >> cmd;
  if (cmd == "join") {
  video_talk->Join(token, "user_data_from_linux_sdk");
  } else if (cmd == "leave") {
  video_talk->Leave();
  } else if (cmd == "pub_camera") {
  video_talk->PublishCameraTrack(false);
  } else if (cmd == "pub_camera_m") {
  video_talk->PublishCameraTrack(true);
  } else if (cmd == "unpub_camera") {
  video_talk->UnpublishCameraTrack();
  } else if (cmd == "pub_mic") {
  video_talk->PublishMicTrack();
  } else if (cmd == "unpub_mic") {
  video_talk->UnpublishMicTrack();
  } else if (cmd == "sub") {
  video_talk->SubsribeAll();
  } else if (cmd == "unsub") {
  video_talk->UnsubsribeAll();
  } else if (cmd == "mute") {
  video_talk->MuteMicTrack();
  } else if (cmd == "unmute") {
  video_talk->UnmuteMicTrack();
  } else if (cmd == "state") {
  video_talk->State();
  } else if (cmd == "cmd") {
  print_cmd();
  } else if (cmd == "exit") {
  video_talk.reset(nullptr);
  cout << "exited" << endl;
  return 0;
  } else {
  cout << "????????????" << endl;
  print_cmd();
  }
  }
  }*/
