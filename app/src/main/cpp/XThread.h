#ifndef FFMPEG_XTHREAD_H
#define FFMPEG_XTHREAD_H

//线程休眠
void XSleep(int mis);

//c++11 线程库
class XThread {
public:

    //启动线程
    virtual bool Start();

    //通过控制isExit安全停止线程（不一定成功）
    virtual void Stop();

    //入口主函数
    virtual void Main() {

    }

    //暂停
    virtual void SetPause(bool isPause);

    //暂停状态
    virtual bool IsPause(){
        isPausing = isPause;
        return isPause;
    }

protected:
    bool isExit = false;
    bool isRunning = false;
    bool isPause = false;
    bool isPausing = false;

private:
    void ThreadMain();
};

#endif