#include "IAudioPlay.h"
#include "XLog.h"

void IAudioPlay::Update(XData data) {
    //压入缓冲队列
    XLOGI("IAudioPlay::Update %d", data.size);

    if (data.size <= 0 || !data.data) return;

    while (!isExit) {
        framesMutex.lock();

        //队列长度超过极值，进入等待状态
        if (frames.size() > maxFrames) {
            XLOGI("IAudioPlay::Update 音频队列满，进入等待状态");
            framesMutex.unlock();
            XSleep(1);
            continue;
        }
        frames.push_back(data);
        XLOGI("IAudioPlay::Update 加入音频队列");
        framesMutex.unlock();
        break;
    }
}

XData IAudioPlay::GetData() {
    XData d;
    while (!isExit) {
        framesMutex.lock();
        if (!frames.empty()) {
            d = frames.front();
            frames.pop_front();
            framesMutex.unlock();
            pts = d.pts;
            return d;
        }
        framesMutex.unlock();
        XSleep(1);
    }
    return d;
}