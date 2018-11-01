#ifndef FFMPEG_FFDEMUX_H
#define FFMPEG_FFDEMUX_H

#include "IDemux.h"

struct AVFormatContext;

class FFDemux : public IDemux {

public:
    //构造方法,做一些初始化的工作
    FFDemux();

    //打开文件，或者流媒体 rmtp http rtsp
    virtual bool Open(const char *url);

    virtual void Close();

    //获取视频参数
    virtual XParameter GetVParam();

    //获取音频参数
    virtual XParameter GetAParam();

    //读取一帧数据，数据由调用者清理
    virtual XData Read();

private:
    AVFormatContext *avFormatContext = 0;
    int audioStream = 1;
    int videoStream = 0;
    std::mutex mutex;
};

#endif