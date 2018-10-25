//
// Created by renzhenming on 2018/10/24.
//

#ifndef FFMPEGPLAYER_XTEXTURE_H
#define FFMPEGPLAYER_XTEXTURE_H

class XTexture{
public:
    static XTexture *Create();
    virtual bool Init(void *window) = 0;
    virtual void Draw(unsigned char *data[],int width,int height) = 0;
};

#endif //FFMPEGPLAYER_XTEXTURE_H
