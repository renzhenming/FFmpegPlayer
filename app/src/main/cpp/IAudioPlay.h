
#ifndef FFMPEGPLAYER_IAUDIOPLAY_H
#define FFMPEGPLAYER_IAUDIOPLAY_H

#include "IObserver.h"
#include "XParameter.h"
#include <list>

class IAudioPlay:public IObserver{
public:
    //缓冲满后阻塞
    virtual void Update(XData data);
    //获取缓冲数据，如没有则阻塞
    virtual XData GetData();
    virtual bool StartPlay(XParameter out)=0;

    //最大缓冲值
    int maxFrames = 100;
protected:
    std::list<XData> frames;
    std::mutex framesMutex;
};

#endif