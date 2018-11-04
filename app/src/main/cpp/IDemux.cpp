#include "IDemux.h"
#include "XLog.h"

void IDemux::Main() {
    while (!isExit) {

        if(IsPause()){
            XSleep(2);
            XLOGI("IDemux::Main 进入暂停状态");
            continue;
        }

        XData d = Read();
        if (d.size > 0) {
            XLOGI("IDemux::Main 开始解封装 data.size:%d", d.size);
            Notify(d);
        } else {
            XSleep(2);
        }
    }
}