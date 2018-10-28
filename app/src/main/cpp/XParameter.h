#ifndef FFMPEG_XPARAMETER_H
#define FFMPEG_XPARAMETER_H

struct AVCodecParameters;

class XParameter {
public:
    AVCodecParameters *avCodecParameters = 0;

    //播放音频的时候设置的输出结果，默认
    int channels = 2;
    int sample_rate = 44100;
};

#endif