#ifndef QN_AUDIO_SOURCE_MIXER_H
#define QN_AUDIO_SOURCE_MIXER_H

#include "qn_common_def.h"

namespace qiniu {

class QINIU_EXPORT_DLL QNAudioSource {
 public:
  /**
   * 获取音效标识
   */
  virtual int GetID() = 0;

 protected:
  ~QNAudioSource(){};
};

class QINIU_EXPORT_DLL QNAudioSourceMixerListener {
 public:
  /**
   * 错误回调
   *
   * @param error_code    错误码
   * @param error_message 错误信息
   */
  virtual void OnAudioSourceMixerError(int error_code,
                                       const std::string& error_message) = 0;
 protected:
  ~QNAudioSourceMixerListener(){};
};

class QNAudioSourceMixer {
 public:
  /**
   * 创建 QNAudioSource 实例对象，用于混音 PCM 数据的场景
   *
   * @param source_id 源 id，必须保证唯一性
   */
  virtual QNAudioSource* CreateAudioSource(int source_id) = 0;

  /**
   * 释放 QNAudioSource 实例对象
   */
  virtual void DestroyAudioSource(QNAudioSource* ptr) = 0;

   /**
   * 设置是否推送到远端，默认推送
   *
   * @param source_id 源 id
   * @param enable, false: 只在本地播放音乐，true: 将本地播放的音乐发布至远端。
   */
  virtual void SetPublishEnable(int source_id, bool enable) = 0;

  /**
   * 是否推送到远端
   *
   * @param source_id 源 id
   * @return false: 只在本地播放音乐， true: 将本地播放的音乐发布至远端
   */
  virtual bool IsPublishEnable(int source_id) = 0;

  /**
   * 设置某音频源音量
   *
   * @param source_id 源 id
   * @param volume 音效音量，范围 0～1.0
   */
  virtual void SetVolume(int source_id, float volume) = 0;

  /**
   * 获取音量，未设置时，默认为 1.0
   *
   * @param source_id 源 id
   */
  virtual float GetVolume(int source_id) = 0;

  /**
   * 设置所有音频源音量
   *
   * @param volume 音效音量，范围 0～1.0
   */
  virtual void SetAllSourcesVolume(float volume) = 0;

  /**
   * 推送 PCM 音频
   *
   * @source_id  源 id
   * @param data 音频数据
   * @param data_size 数据长度
   * @param bits_per_sample 位宽，即每个采样点占用位数
   * @param sample_rate 采样率
   * @param channels 声道数
   *
   * @return 成功返回 0，其它请参考错误码列表
   */
  virtual int32_t PushAudioFrame(int source_id, const uint8_t* data,
                                 uint32_t data_size, uint32_t bits_per_sample,
                                 uint32_t sample_rate, uint32_t channels) = 0;

 protected:
  QNAudioSourceMixer(){};
  ~QNAudioSourceMixer(){};
};

}  // namespace qiniu

#endif
