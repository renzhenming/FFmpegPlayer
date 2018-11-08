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

void IDecode::Clear() {
    packsMutex.lock();
    while (!packs.empty()) {
        packs.front().Drop();
        packs.pop_front();
    }
    pts = 0;
    synPts = 0;
    packsMutex.unlock();
}

void IDecode::Main() {
    while (!isExit) {

        //判断是否暂停解码
        if (IsPause()) {
            XSleep(2);
            XLOGI("IDecode::Main pause");
            continue;
        }

        packsMutex.lock();

        //判断音视频同步，视频同步音频
        if (isVideo && synPts > 0) {
            if (synPts < pts) {
                XLOGI("IDecode::Main video waiting for audio,audio pts = "
                              "%d video pts=%d", synPts, pts);
                packsMutex.unlock();
                XSleep(1);
                continue;
            }
        }

        //如果队列为空，则进入等待状态
        if (packs.empty()) {
            XLOGI("IDecode ::Main decode list is null ,waiting ... ");
            //如果队列中没有数据，则线程休眠
            packsMutex.unlock();
            XSleep(1);
            continue;
        }

        //解码队列中有数据
        XData pack = packs.front();

        XLOGI("IDecode ::Main decode list catch packet,size=%d", pack.size);
        //从队列移除
        packs.pop_front();

        //发送数据到解码线程，一个数据包，可能解码多个结果
        if (this->SendPacket(pack)) {
            while (!isExit) {
                //从线程获取解码数据
                XData frame = RecvFrame();
                if (!frame.data) {
                    break;
                }
                pts = frame.pts;
                XLOGI("IDecode ::Main decode frame success frame.size = %d", frame.size);
                //发送数据给观察者
                this->Notify(frame);
            }
        } else {
            XLOGE("IDecode ::Main decode frame failed");
        }
        pack.Drop();
        packsMutex.unlock();
    }
}