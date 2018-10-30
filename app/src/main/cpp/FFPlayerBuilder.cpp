#include "FFPlayerBuilder.h"
#include "FFDemux.h"
#include "FFdecode.h"
#include "FFResample.h"
#include "GLVideoView.h"
#include "SLAudioPlay.h"

IDemux *FFPlayerBuilder::CreateDemux()
{
    IDemux *ff = new FFDemux();
    return ff;
}

IDecode *FFPlayerBuilder::CreateDecode()
{
    IDecode *ff = new FFDecode();
    return ff;
}

IResample *FFPlayerBuilder::CreateResample()
{
    IResample *ff = new FFResample();
    return ff;
}

IVideoView *FFPlayerBuilder::CreateVideoView()
{
    IVideoView *ff = new GLVideoView();
    return ff;
}

IAudioPlay *FFPlayerBuilder::CreateAudioPlay()
{
    IAudioPlay *ff = new SLAudioPlay();
    return ff;
}

IPlayer *FFPlayerBuilder::CreatePlayer(unsigned char index)
{
    return IPlayer::Get(index);
}

void FFPlayerBuilder::InitHard(void *vm)
{
    FFDecode::InitHard(vm);
}