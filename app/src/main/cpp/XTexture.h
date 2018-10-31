//
// Created by renzhenming on 2018/10/24.
//

#ifndef FFMPEGPLAYER_XTEXTURE_H
#define FFMPEGPLAYER_XTEXTURE_H

//数值和ffmpeg中保持一致
enum XTextureType {
    // Y 4  u 1 v 1
    XTEXTURE_YUV420P = 0,
    // Y4   uv1
    XTEXTURE_NV12 = 25,
    // Y4   vu1
    XTEXTURE_NV21 = 26
};

class XTexture {
public:
    static XTexture *Create();

    virtual bool Init(void *window, XTextureType type = XTEXTURE_YUV420P) = 0;

    virtual void Draw(unsigned char *data[], int width, int height) = 0;
};

#endif //FFMPEGPLAYER_XTEXTURE_H
