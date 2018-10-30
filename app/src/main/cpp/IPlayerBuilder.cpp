/*******************************************************************************
**                                                                            **
**                     Jiedi(China nanjing)Ltd.                               **
**	               创建：夏曹俊，此代码可用作为学习参考                       **
*******************************************************************************/

#include "IPlayerBuilder.h"
#include "IVideoView.h"
#include "IResample.h"
#include "IDecode.h"
#include "IAudioPlay.h"
#include "IDemux.h"

IPlayer *IPlayerBuilder::BuilderPlayer(unsigned char index)
{
    IPlayer *play = CreatePlayer(index);

    //解封装
    IDemux *de = CreateDemux();

    //视频解码
    IDecode *vdecode = CreateDecode();

    //音频解码
    IDecode *adecode = CreateDecode();

    //解码器观察解封装
    de->AddObserver(vdecode);
    de->AddObserver(adecode);

    //显示观察视频解码器
    IVideoView *view = CreateVideoView();
    vdecode->AddObserver(view);

    //重采样观察音频解码器
    IResample *resample = CreateResample();
    adecode->AddObserver(resample);

    //音频播放观察重采样
    IAudioPlay *audioPlay = CreateAudioPlay();
    resample->AddObserver(audioPlay);

    play->demux = de;
    play->adecode = adecode;
    play->vdecode = vdecode;
    play->videoView = view;
    play->resample = resample;
    play->audioPlay = audioPlay;
    return play;
}