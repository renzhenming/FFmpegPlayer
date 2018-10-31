#include "IDemux.h"
#include "XLog.h"

void IDemux::Main() {
    while (!isExit) {
        XData d = Read();
        if (d.size > 0) {
            XLOGI("IDemux::Main 开始解封装 data.size:%d", d.size);
            Notify(d);
        } else {
            XLOGI("IDemux::Main 解封装完成 break");
            break;
        }
    }
}