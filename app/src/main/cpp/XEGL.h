//
// Created by renzhenming on 2018/10/24.
//

#ifndef FFMPEGPLAYER_XEGL_H
#define FFMPEGPLAYER_XEGL_H

class XEGL {
public:
    virtual bool Init(void *window)= 0;

    static XEGL *Get();

    virtual void Draw() = 0;

protected:
    //单利模式，将构造方法设置为非public类型
    XEGL() {}
};

#endif //FFMPEGPLAYER_XEGL_H
