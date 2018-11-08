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
    while (!isExit) {

        if (IsPause()) {
            XSleep(2);
            continue;
        }

        mutex.lock();

        if (!audioPlay || !vdecode) {
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
        XLOGE("IPlayer::Open resample->Open %s failed!", path);
    }
    XLOGI("IPlayer::Open %s success!", path);
    mutex.unlock();
    return true;
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

    if (adecode) {
        adecode->Start();
    }

    if (!demux || !demux->Start()) {
        mutex.unlock();
        XLOGE("IPlayer::Start demux->Start failed!");
        return false;
    }

    if (audioPlay) {
        audioPlay->StartPlay(outParam);
    }

    XThread::Start();

    mutex.unlock();
    return true;
}


void IPlayer::Close() {
    mutex.lock();
    //先关闭主体线程再清理观察者线程
    XThread::Stop();

    //解封装
    if (demux)
        demux->Stop();
    //解码
    if (vdecode)
        vdecode->Stop();
    if (adecode)
        adecode->Stop();
    //IAudioPlay中有一个GetData的循环队列，由于这个循环是在OpenSLES的RegisterCallback
    //中执行的一个死循环，所以导致就算是播放完成后它也不会退出，因此，在视频播放到最后的时候，由于队列中
    //再也取不到数据了，所以while循环进入无限休眠状态，此时取重新打开视频，会失败，因为close销毁会失败
    //这个循环是区别于我们通过XThread开启的循环的，因为XThread中的循环通过isExit和IsRunning进行管理
    //而这个没有，所以我们仿照XThread的处理方式进行设置
    if (audioPlay)
        audioPlay->Stop();
    //清理缓冲队列
    if (vdecode)
        vdecode->Clear();
    if (adecode)
        adecode->Clear();
    if (audioPlay)
        audioPlay->Clear();

    //清理资源
    if (audioPlay)
        audioPlay->Close();
    if (videoView)
        videoView->Close();
    if (vdecode)
        vdecode->Close();
    if (adecode)
        adecode->Close();
    if (demux)
        demux->Close();
    mutex.unlock();
}

bool IPlayer::Seek(double percent) {
    bool result = false;

    if (!demux) return result;

    //暂停所有线程
    SetPause(true);

    mutex.lock();

    //清理缓冲队列
    if (vdecode)
        //清理缓冲队列，包括ffmpeg的缓冲和我们自己的缓冲
        vdecode->Clear();
    else {
        //seek是seek的视频，如果视频解码器不存在，则seek失败
        mutex.unlock();
        SetPause(false);
        return result;
    }

    if (adecode)
        adecode->Clear();

    if (audioPlay)
        audioPlay->Clear();

    //跳转到关键帧
    result = demux->Seek(percent);

    //解码到实际需要显示的帧
    int seekPts = percent * demux->totalMs;

    while (!isExit) {
        XData data = demux->Read();
        if (data.size <= 0) break;
        if (data.isAudio) {
            if (data.pts < seekPts) {
                data.Drop();
                continue;
            }

            //写入缓冲队列
            demux->Notify(data);
            continue;
        }

        //解码需要显示的帧之前的数据
        vdecode->SendPacket(data);
        data.Drop();
        XData frame = vdecode->RecvFrame();
        if (frame.size <= 0) {
            continue;
        }
        if (frame.pts >= seekPts) {
            break;
        }
    }
    mutex.unlock();

    //重新开启线程
    SetPause(false);
    return result;
}

double IPlayer::PlayPos() {
    double pos = 0.0;
    mutex.lock();

    int total = 0;
    if (demux)
        total = demux->totalMs;
    if (total > 0) {
        if (vdecode) {
            pos = (double) vdecode->pts / (double) total;
        }
    }
    mutex.unlock();
    return pos;
}

void IPlayer::SetPause(bool isP) {
    mutex.lock();
    XThread::SetPause(isP);
    if (demux)
        demux->SetPause(isP);
    if (vdecode)
        vdecode->SetPause(isP);
    if (adecode)
        adecode->SetPause(isP);
    if (audioPlay)
        audioPlay->SetPause(isP);
    mutex.unlock();
    XLOGI("SetPause finish");
}