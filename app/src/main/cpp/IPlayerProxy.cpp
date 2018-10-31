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