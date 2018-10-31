#include "IDecode.h"
#include "XLog.h"

void IDecode::Update(XData packet) {
    //判断，只有在XData数据和解码器类型相同的时候才能开始解码
    //换句话说，视频解码器解码视频data，音频解码器解码音频data
    if (packet.isAudio != isAudio) {
        return;
    }


    while (!isExit) {
        packsMutex.lock();

        //阻塞，当队列中数据量没有达到极值的时候不断的加入
        if (packs.size() < maxList) {
            packs.push_back(packet);
            //log 报错（引用了packs的缘故） error: reference to non-static member function must be called;
            //did you mean to call it with no arguments?
            //XLOGI("IDecode::Update 解封装后将packet加入缓冲队列，当前队列size= %d ",packs.size);
            XLOGI("IDecode::Update 解封装后将packet加入缓冲队列");
            packsMutex.unlock();
            break;
        } else {
            XLOGI("IDecode::Update 解封装后队列长度达到极值，等待解码器消费packet");
        }
        packsMutex.unlock();
        XSleep(1);
    }
}

void IDecode::Main() {
    while (!isExit) {
        packsMutex.lock();

        //判断音视频同步
        if (isVideo && synPts > 0) {
            if (synPts < pts) {
                XLOGI("音视频同步..等待.. 视频pts = %d 音频pts=%d", synPts, pts);
                packsMutex.unlock();
                XSleep(1);
                continue;
            }else{
                XLOGI("音视频同步..无需等待.. 视频pts = %d 音频pts=%d", synPts, pts);
            }
        }

        if (packs.empty()) {
            XLOGI("IDecode ::Main 从队列种取packet,队列为空，进入等待状态");
            //如果队列中没有数据，则线程休眠
            packsMutex.unlock();
            XSleep(1);
            continue;
        }

        //取出packet
        XData pack = packs.front();

        XLOGI("IDecode ::Main 从队列中取packet,size=%d", pack.size);
        //从队列移除
        packs.pop_front();

        //发送数据到解码线程，一个数据包，可能解码多个结果
        if (this->SendPacket(pack)) {
            XLOGI("IDecode ::Main 发送packet到线程解码成功 pack.size=%", pack.size);
            while (!isExit) {
                //从线程获取解码数据
                XData frame = RecvFrame();
                if (!frame.data) {
                    XLOGE("IDecode ::Main 从解码器种读取解码后的数据失败  frame.size=%", frame.size);
                    break;
                }
                pts = frame.pts;
                XLOGI("IDecode ::Main 从解码器种读取解码后的数据成功 开始通知观察者 frame.size = %d", frame.size);
                //发送数据给观察者
                this->Notify(frame);
            }
        } else {
            XLOGE("IDecode ::Main 发送packet到线程解码失败 pack.size=%", pack.size);
        }
        pack.Drop();
        packsMutex.unlock();
    }
}