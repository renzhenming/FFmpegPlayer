#ifndef FFMPEG_IPLAY_H
#define FFMPEG_IPLAY_H

#include <mutex>
#include "XThread.h"
#include "XParameter.h"

class IDemux;

class IAudioPlay;

class IVideoView;

class IResample;

class IDecode;

class IPlayer : public XThread {
public:
    static IPlayer *Get(unsigned char index = 0);

    virtual bool Open(const char *path);

    virtual bool Seek(double percent);

    virtual void Close();

    virtual bool Start();

    virtual void InitView(void *window);

    //获取当前的播放进度 0.0 ~ 1.0
    virtual double PlayPos();

    virtual void SetPause(bool isP);

    //是否视频硬解码
    bool isHardDecode = true;

    //音频输出参数配置
    XParameter outParam;

    IDemux *demux = 0;
    IDecode *vdecode = 0;
    IDecode *adecode = 0;
    IResample *resample = 0;
    IVideoView *videoView = 0;
    IAudioPlay *audioPlay = 0;

protected:
    //用作音视频同步
    void Main();
    std::mutex mutex;
    //单例模式
    IPlayer() {};
};

#endif