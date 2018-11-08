//
// Created by renzhenming on 2018/10/31.
//

#include "IPlayerProxy.h"
#include "FFPlayerBuilder.h"

void IPlayerProxy::Init(void *vm) {
    mutex.lock();
    if (vm) {
        FFPlayerBuilder::InitHard(vm);
    }
    if (!player) {
        player = FFPlayerBuilder::Get()->BuilderPlayer();
    }
    mutex.unlock();
}

bool IPlayerProxy::Open(const char *path) {
    bool result = false;
    mutex.lock();
    if (player) {
        result = player->Open(path);
    }
    mutex.unlock();
    return result;
}

bool IPlayerProxy::Seek(double percent){
    bool result = false;
    mutex.lock();
    if(player){
        player->Seek(percent);
    }
    mutex.unlock();
    return result;
}

bool IPlayerProxy::Start() {
    bool result = false;
    mutex.lock();
    if (player) {
        result = player->Start();
    }
    mutex.unlock();
    return result;
}

void IPlayerProxy::InitView(void *window) {
    mutex.lock();
    if (player) {
        player->InitView(window);
    }
    mutex.unlock();
}

//获取当前的播放进度 0.0 ~ 1.0
double IPlayerProxy::PlayPos()
{
    double pos = 0.0;
    mutex.lock();
    if(player)
    {
        pos = player->PlayPos();
    }
    mutex.unlock();
    return pos;
}

bool IPlayerProxy::IsPause(){
    bool re = false;
    mutex.lock();
    if(player)
        re = player->IsPause();
    mutex.unlock();
    return re;
}

void IPlayerProxy::SetPause(bool isP){
    mutex.lock();
    if(player)
        player->SetPause(isP);
    mutex.unlock();
}