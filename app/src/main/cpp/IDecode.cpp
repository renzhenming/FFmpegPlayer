#include "IDecode.h"
#include "XLog.h"

void IDecode::Update(XData packet){
    if(packet.isAudio != isAudio){
        return;
    }

    XLOGI("Update packs.push_back %d ",packet.size);
    while(!isExit){
        packsMutex.lock();

        //阻塞
        if(packs.size() < maxList){
            packs.push_back(packet);
            packsMutex.unlock();
            break;
        }else{
            XLOGI("packet list is full,waiting to be consume....");
        }

        packsMutex.unlock();
        XSleep(1);
    }
}

void IDecode ::Main(){
    while(!isExit){
        packsMutex.lock();

        if(packs.empty()){
            XLOGI("list is empty, waiting to be added ....");
            //如果队列中没有数据，则线程休眠
            packsMutex.unlock();
            XSleep(1);
            continue;
        }

        //去除packet
        XData pack = packs.front();

        XLOGI("get a packet in packet list,size=%d",pack.size);
        //从队列移除
        packs.pop_front();

        //发送数据到解码线程，一个数据包，可能解码多个结果
        if(this->SendPacket(pack)){
            while(!isExit){
                //从线程获取解码数据
                XData frame = RecvFrame();
                if(!frame.data) break;
                XLOGI("get a frame after decoding,size= ,notify...",frame.size);
                //发送数据给观察者
                this->Notify(frame);
            }
        }
        pack.Drop();
        packsMutex.unlock();
    }
}