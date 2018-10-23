#ifndef FFMPEG_XOBSERVER_H
#define FFMPEG_XOBSERVER_H

#include <vector>
#include <mutex>
#include "XData.h"
#include "XThread.h"

class IObserver : public XThread{

public:
    //观察者接收数据
    virtual void Update(XData data){}

    //添加观察者 (线程安全)
    void AddObserver(IObserver *observer);

    //通知观察者
    void Notify(XData data);

protected:

    std::vector<IObserver *>observers;
    std::mutex mutex;
};

#endif