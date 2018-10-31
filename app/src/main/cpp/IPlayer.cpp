#include "IPlayer.h"
#include "IDemux.h"
#include "IDecode.h"
#include "IAudioPlay.h"
#include "IVideoView.h"
#include "IResample.h"
#include "XLog.h"

// error: 'static' can only be specified inside the class definition
//static IPlayer * IPlayer::Get(unsigned char index){
IPlayer *IPlayer::Get(unsigned char index) {

    static IPlayer player[256];
    return &player[index];
}

void IPlayer::InitView(void *window) {
    if (videoView) {
        videoView->SetRender(window);
    }
}

void IPlayer::Main() {
    while(!isExit){
        mutex.lock();

        if(!audioPlay || !vdecode){
            mutex.unlock();
            XSleep(2);
            continue;
        }

        //获取音频的pts告诉视频
        int apts = audioPlay->pts;
        vdecode->synPts = apts;
        mutex.unlock();
        XSleep(2);
    }
}

bool IPlayer::Open(const char *path) {
    mutex.lock();
    //解封装
    if (!demux || !demux->Open(path)) {
        XLOGE("IPlayer::Open demux->Open %s failed!", path);
        return false;
    }

    //解码，解码可能不需要，解封之后可能就是原始数据，这种情况不需要解码，所以解码失败也不要return
    if (!vdecode || !vdecode->Open(demux->GetVParam(), isHardDecode)) {
        XLOGE("IPlayer::Open vdecode->Open %s failed!", path);
        //return false;
    }

    if (!adecode || !adecode->Open(demux->GetAParam())) {
        XLOGE("IPlayer::Open adecode->Open %s failed!", path);
        //return false;
    }

    //重采样，有可能不需要，解码后或者解封装后可能就是可以直接播放的数据
    //如果用户没有配置输出参数，则取输入参数作为输出
    if (outParam.sample_rate <= 0)
        outParam = demux->GetAParam();
    if (!resample || !resample->Open(demux->GetAParam(), outParam)) {
        XLOGE("resample->Open %s failed!", path);
    }
    XLOGI("IPlayer::Open %s success!", path);
    mutex.unlock();
    return true;
}


bool IPlayer::Start() {
    mutex.lock();
    if (!demux || !demux->Start()) {
        XLOGE("IPlayer::Start demux->Start failed!");
        return false;
    }
    if (adecode) {
        adecode->Start();
    }
    if (audioPlay) {
        audioPlay->StartPlay(outParam);
    }
    if (vdecode) {
        vdecode->Start();
    }

    XLOGI("IPlayer::Start success!");
    XThread::Start();
    mutex.unlock();
    return true;
}