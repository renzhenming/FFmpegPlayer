#ifndef FFMPEG_FFDEMUX_H
#define FFMPEG_FFDEMUX_H

#include "IDemux.h"

class FFDemux : public IDemux {

public:
    //打开文件，或者流媒体 rmtp http rtsp
    virtual bool Open(const char *url);
    //读取一帧数据，数据由调用者清理
    virtual XData Read();
};

#endif