#ifndef FFMPEG_FFDECODE_H
#define FFMPEG_FFDECODE_H

#include "IDecode.h"

struct AVCodecContext;
struct AVFrame;

//继承的时候，这个public表示IObserver中的属性或方法对子类TestObserver是public
class FFDecode : public IDecode {
public:
    //使用硬解码时需要初始化
    static void InitHard(void *vm);

    //打开解码器
    virtual bool Open(XParameter xParameter, bool isHard);

    //future模型 发送数据到线程解码
    virtual bool SendPacket(XData packet);

    //从线程中获取解码结果，再次调用会复用上次空间，线程不安全
    virtual XData RecvFrame();

protected:
    AVCodecContext *avCodecContext = 0;
    AVFrame *frame = 0;
};

#endif