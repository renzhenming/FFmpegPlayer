#ifndef FFMPEG_IDEMUX_H
#define FFMPEG_IDEMUX_H

#include "XData.h"
#include "IObserver.h"
#include "XParameter.h"

//解封装接口
class IDemux : public IObserver {
public:
    //打开文件，或者流媒体， rmtp http rtsp
    //纯虚函数:是一种特殊的虚函数，在许多情况下，在基类中不能对虚函数给出有意义的实现，而把它声明为纯虚函数，
    // 它的实现留给该基类的派生类去做。这就是纯虚函数的作用。
    // virtual <类型><函数名>(<参数表>)=0;
    virtual bool Open(const char *url) = 0;

    virtual void Close() = 0;

    //position 0.0 ~1.0
    virtual bool Seek(double percent) = 0;

    //获取视频参数
    virtual XParameter GetVParam() = 0;

    //获取音频参数
    virtual XParameter GetAParam() = 0;

    //读取一帧数据，数据由调用者清理
    virtual XData Read() = 0;

    //总时长（毫秒）
    int totalMs = 0;

protected:

    virtual void Main();
};

#endif