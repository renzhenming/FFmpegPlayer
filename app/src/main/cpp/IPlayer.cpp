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
        videoView->Close();
        videoView->SetRender(window);
    }
}

/**
 * 音视频同步线程，不断的获取音频的pts,传递到视频，
 * 视频根据音频pts进行同步
 */
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
    Close();
    mutex.lock();
    //解封装
    if (!demux || !demux->Open(path)) {
        mutex.unlock();
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

void IPlayer::Close() {
    mutex.lock();
    //先关闭主体线程再清理观察者线程
    XThread::Stop();

    //解封装
    if(demux)
        demux->Stop();
    //解码
    if(vdecode)
        vdecode->Stop();
    if(adecode)
        adecode->Stop();
    //清理缓冲队列
    if(vdecode)
        vdecode->Clear();
    if(adecode)
        adecode->Clear();
    if(audioPlay)
        audioPlay->Clear();

    //清理资源
    if(audioPlay)
        audioPlay->Close();
    if(videoView)
        videoView->Close();
    if(vdecode)
        vdecode->Close();
    if (adecode)
        adecode->Close();
    if(demux)
        demux->Close();
    mutex.unlock();
}

/**
 * 注意播放线程和解码线程的开启顺序，如果把vdecode->Start();放在后边开启，会导致
 * 视频前边部分帧播放丢失的问题
 * @return
 */
bool IPlayer::Start() {
    mutex.lock();

    if (vdecode) {
        vdecode->Start();
    }
    XLOGI("IPlayer::vdecode->Start() success!");
    if (!demux || !demux->Start()) {
        mutex.unlock();
        XLOGE("IPlayer::Start demux->Start failed!");
        return false;
    }
    XLOGI("IPlayer::demux->Start() success!");
    if (adecode) {
        adecode->Start();
    }
    XLOGI("IPlayer::adecode->Start() success!");
    if (audioPlay) {
        audioPlay->StartPlay(outParam);
    }

    XLOGI("IPlayer::audioPlay->StartPlay(outParam) success!");
    XThread::Start();

    mutex.unlock();
    return true;
}