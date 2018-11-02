//
// Created by renzhenming on 2018/10/24.
//

#ifndef FFMPEGPLAYER_XSHADER_H
#define FFMPEGPLAYER_XSHADER_H

#include <mutex>

//定义一个心的枚举类型，和XTextureType值保持一致，用于接收XTextureType
//定义一个新的是为了不让这两个类产生关联
enum XShaderType {
    XSHADER_YUV420P = 0,    //软解码和虚拟机
    XSHADER_NV12 = 25,      //手机硬解码
    XSHADER_NV21 = 26
};

class XShader {
public:
    virtual bool Init(XShaderType type = XSHADER_YUV420P);

    virtual void Close();

    //获取材质并映射到内存
    virtual void
    GetTexture(unsigned int index, int width, int height, unsigned char *buf, bool isa = false);

    virtual void Draw();

protected:
    unsigned int vsh = 0;
    unsigned int fsh = 0;
    unsigned int program = 0;
    unsigned int texts[100] = {0};
    std::mutex mutex;
};

#endif //FFMPEGPLAYER_XSHADER_H
