#ifndef FFMPEGPLAYER_SLAUDIOPLAY_H
#define FFMPEGPLAYER_SLAUDIOPLAY_H

#include "IAudioPlay.h"

class SLAudioPlay : public IAudioPlay {
public:
    virtual bool StartPlay(XParameter out);

    virtual void Close();

    void PlayCall(void *bufqueue);

    SLAudioPlay();

    virtual ~SLAudioPlay();

protected:
    //内部空间用来拷贝音频数据
    unsigned char *buf = 0;
    std::mutex mutex;
};

#endif