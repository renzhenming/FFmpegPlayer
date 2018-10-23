#ifndef FFMPEG_IDECODE_H
#define FFMPEG_IDECODE_H

#include "IObserver.h"
#include "XParameter.h"
#include <list>

class IDecode : public IObserver{
public:
    //打开解码器
    virtual bool Open(XParameter xParameter)=0;

    //future模型，发送数据到线程解码
    virtual bool SendPacket(XData packet) = 0;

    //从线程中获取解码数据
    virtual XData RecvFrame() = 0;

    //由主体notify的数据 阻塞
    virtual void Update(XData packet);

    bool isAudio = false;
    bool isVideo = false;

    //缓冲队列的容量
    int maxList = 100;

protected:
    virtual void Main();

    //缓冲队列，读取到packet会放入缓冲队列，然后从队列中获取
    std::list<XData> packs;
    std::mutex packsMutex;
};

#endif