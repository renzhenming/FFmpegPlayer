//
// Created by renzhenming on 2018/10/31.
//

#ifndef FFMPEGPLAYER_IPLAYERPROXY_H
#define FFMPEGPLAYER_IPLAYERPROXY_H

#include <mutex>
#include "IPlayer.h"

class IPlayerProxy : public IPlayer {

public:

    /**
     * 获取IPlayerProxy单例对象
     * @return
     */
    static IPlayerProxy *Get() {

        /**
         * static的作用：
         * 1.控制变量的可见范围，只对本文件可见
         * 2.生命周期是整个源程序
         */
        static IPlayerProxy proxy;
        return &proxy;
    }

    /**
     * 1.初始化硬解码环境
     * 2.创建player对象
     * 3.创建解封装器
     * 4.创建音视频解码器
     * 5.创建视频显示器
     * 6.创建音频播放器
     * 7.创建音频采样器
     *
     * 8.音视频解码器加入解封装器的观察队列
     * 9.视频播放器加入视频解码器的观察队列
     * 10.音频采样器加入音频解码器的观察队列
     * 11.音频播放器加入音频采样器的观察队列
     *
     * 一系列流程下来后，在player对象中就持有了这些模块对象的引用
     */
    void Init(void *vm = 0);

    /**
     * Open执行的内容包括：
     * 1.解封装器open,打开视频文件
     * 2.音视频解码器open
     * 3.音频采样器初始化
     *
     * @param path 视频文件路径，可以是本地或者网络
     * @return
     */
    virtual bool Open(const char *path);

    /**
     * 1.视频解码线程开启,要先开启视频解码线程
     * 2.音频解码线程开启，等待解码音频帧，保证解码器线程再解封装器线程之前开启
     * 3.音视频同步线程开启，准备进行音视频同步
     * 4.开启解封装线程，解封装开始
     * 5.音频播放器开启，开始播放音频
     * @return
     */
    virtual bool Start();

    virtual bool Seek(double position);

    virtual void InitView(void *window);

    //获取当前的播放进度 0.0 ~ 1.0
    virtual double PlayPos();

    virtual void SetPause(bool isP);

    virtual bool IsPause();

protected:

    /**
     * 将IPlayerProxy构造方法定义protected，禁止在外部创建
     * IPlayerProxy对象，而提供Get用于获取IPlayerProxy对象
     */
    IPlayerProxy() {}

    IPlayer *player = 0;

    std::mutex mutex;
};

#endif //FFMPEGPLAYER_IPLAYERPROXY_H
