
#ifndef FFMPEGPLAYER_IAUDIOPLAY_H
#define FFMPEGPLAYER_IAUDIOPLAY_H

#include "IObserver.h"
#include "XParameter.h"

class IAudioPlay:public IObserver{
public:
    virtual void Update(XData data);
    virtual bool StartPlay(XParameter out)=0;
};

#endif