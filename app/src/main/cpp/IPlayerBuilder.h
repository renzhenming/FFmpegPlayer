#ifndef FFMPEG_IPLAYRBUILDER_H
#define FFMPEG_IPLAYRBUILDER_H

class IPlayerBuilder{

public:
    virtual IPlayer *BuildPlayer(unsigned char index = 0);
protected:
    virtual IDemux *CreateDemux() = 0;
    virtual IDecode *CreateDecode() = 0;
    virtual IResample *CreateResample() = 0;
    virtual IVideoView *CreateVideoView()  = 0;
    virtual IAudioPlay *CreateAudioPlay() = 0;
    virtual IPlayer *CreatePlayer(unsigned char index=0) = 0;
};

#endif