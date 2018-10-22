#ifndef FFMPEG_FFDECODE_H
#define FFMPEG_FFDECODE_H

#include "IDecode.h"

struct AVCodecContext;

class FFDecode : public IDecode{
public:
    virtual bool Open(XParameter xParameter);

protected:
    AVCodecContext *avCodecContext = 0;
};

#endif