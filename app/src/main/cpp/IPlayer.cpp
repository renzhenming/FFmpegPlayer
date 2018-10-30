#include "IPlayer.h"
#include "IDemux.h"
#include "IDecode.h"
#include "IAudioPlay.h"
#include "IVideoView.h"
#include "IResample.h"
#include "XLog.h"

// error: 'static' can only be specified inside the class definition
//static IPlayer * IPlayer::Get(unsigned char index){
IPlayer * IPlayer::Get(unsigned char index){

    static IPlayer player[256];
    return &player[index];
}

bool IPlayer::Open(const char *path){
    //解封装
    if(!demux || !demux->Open(path)){
        XLOGE("IPlayer::Open demux->Open %s failed!",path);
        return false;
    }

    //解码，解码可能不需要，解封之后可能就是原始数据，这种情况不需要解码，所以解码失败也不要return
    if(!vdecode || !vdecode->Open(demux->GetVParam(),isHardDecode)){
        XLOGE("IPlayer::Open vdecode->Open %s failed!",path);
        //return false;
    }

    if(!adecode ||!adecode->Open(demux->GetAParam())){
        XLOGE("IPlayer::Open adecode->Open %s failed!",path);
        //return false;
    }

    //重采样，有可能不需要，解码后或者解封装后可能就是可以直接播放的数据
    XParameter outParam = demux->GetAParam();
    if(!resample || !resample->Open(demux->GetAParam(),outParam)){
        XLOGE("resample->Open %s failed!",path);
    }
    XLOGI("IPlayer::Open success!",path);
    return true;
}


bool IPlayer::Start(){

    return true;
}