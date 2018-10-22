#ifndef FFMPEG_XPARAMETER_H
#define FFMPEG_XPARAMETER_H

struct AVCodecParameters;

class XParameter {
public:
    AVCodecParameters *avCodecParameters = 0;
};

#endif