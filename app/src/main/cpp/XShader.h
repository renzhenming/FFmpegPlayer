//
// Created by renzhenming on 2018/10/24.
//

#ifndef FFMPEGPLAYER_XSHADER_H
#define FFMPEGPLAYER_XSHADER_H

class XShader{
public:
    virtual bool Init();
protected:
    unsigned int vsh = 0;
    unsigned int fsh = 0;
    unsigned int program = 0;
};

#endif //FFMPEGPLAYER_XSHADER_H
