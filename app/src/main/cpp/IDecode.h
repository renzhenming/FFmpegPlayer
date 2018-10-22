#ifndef FFMPEG_IDECODE_H
#define FFMPEG_IDECODE_H

#include "IObserver.h"
#include "XParameter.h"

class IDecode : public IObserver{
public:
    //打开解码器
    virtual bool Open(XParameter xParameter)=0;
};

#endif