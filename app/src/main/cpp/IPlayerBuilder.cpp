
#include "IPlayerBuilder.h"
#include "IVideoView.h"
#include "IResample.h"
#include "IDecode.h"
#include "IAudioPlay.h"
#include "IDemux.h"

IPlayer * IPlayerBuilder::BuildPlayer(unsigned char index){

    //创建播放器
    IPlayer *play = CreatePlayer(index);

    //解封装
    IDemux *demux = CreateDemux();

    //视频解码
    IDecode *vdecode = CreateDecode();

    //音频解码
    IDecode *adecode = CreateDecode();

    //解码器观察解封装
    demux->AddObserver(vdecode);
    demux->AddObserver(adecode);

    //创建VideoView
    IVideoView *view = CreateVideoView();
    vdecode->AddObserver(view);

    //重采样观察音频解码器
    IResample *resample = CreateResample();
    adecode->AddObserver(resample);

    //音频播放器观察重采样结果
    IAudioPlay *audioPlay = CreateAudioPlay();
    resample->AddObserver(audioPlay);

    play->demux = demux;
    play->adecode = adecode;
    play->vdecode = vdecode;
    play->videoView = view;
    play->resample = resample;
    play->audioPlay = audioPlay;
    return play;
}