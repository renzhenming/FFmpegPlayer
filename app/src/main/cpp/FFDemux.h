#ifndef FFMPEG_FFDEMUX_H
#define FFMPEG_FFDEMUX_H

#include "IDemux.h"

//为什么可以不用引入头文件，直接定义？TODO
struct AVFormatContext;

class FFDemux : public IDemux {

public:
    //构造方法,做一些初始化的工作
    FFDemux();
    //打开文件，或者流媒体 rmtp http rtsp
    virtual bool Open(const char *url);
    //读取一帧数据，数据由调用者清理
    virtual XData Read();

private:
    AVFormatContext *avFormatContext = 0;
};

#endif