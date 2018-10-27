#ifndef FFMPEGPLAYER_SLAudioPlay_H
#define FFMPEGPLAYER_SLAudioPlay_H

#include "IAudioPlay.h"

class SLAudioPlay:public IAudioPlay{
public:
    virtual bool StartPlay(XParameter out);
    void PlayCall(void *bufqueue);
};

#endif