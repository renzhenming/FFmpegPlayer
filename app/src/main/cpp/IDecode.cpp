#include "IDecode.h"
#include "XLog.h"

void IDecode::Update(XData packet){
    if(packet.isAudio != isAudio){
        return;
    }


    while(!isExit){
        packsMutex.lock();

        //阻塞
        if(packs.size() < maxList){
            packs.push_back(packet);
            //log 报错 error: reference to non-static member function must be called; did you mean to call it with no arguments?
            //XLOGI("IDecode-> 解封装后将packet加入缓冲队列，当前队列size= %d ",packs.size);
            packsMutex.unlock();
            break;
        }else{
            //XLOGI("IDecode-> 解封装后队列长度达到极值，等待解码器消费packet");
        }
        packsMutex.unlock();
        XSleep(1);
    }
}

void IDecode ::Main(){
    while(!isExit){
        packsMutex.lock();

        if(packs.empty()){
            //XLOGI("IDecode-> 从packet队列种取packet,队列为空，线程休眠");
            //如果队列中没有数据，则线程休眠
            packsMutex.unlock();
            XSleep(1);
            continue;
        }

        //去除packet
        XData pack = packs.front();

        //XLOGI("IDecode-> 从packet队列种取packet,size=%d",pack.size);
        //从队列移除
        packs.pop_front();

        //发送数据到解码线程，一个数据包，可能解码多个结果
        if(this->SendPacket(pack)){
            //XLOGI("IDecode-> 发送packet到线程解码成功 pack.size=%",pack.size);
            while(!isExit){
                //从线程获取解码数据
                XData frame = RecvFrame();
                if(!frame.data) break;
                //XLOGI("IDecode-> 从解码器种读取解码后的数据成功 开始通知观察者 frame.size = %d",frame.size);
                //发送数据给观察者
                this->Notify(frame);
            }
        }
        pack.Drop();
        packsMutex.unlock();
    }
}