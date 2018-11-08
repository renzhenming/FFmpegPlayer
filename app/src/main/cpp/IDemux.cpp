#include "IDemux.h"
#include "XLog.h"

void IDemux::Main() {
    while (!isExit) {

        //判断是否已暂停
        if(IsPause()){
            XSleep(2);
            XLOGI("IDemux::Main pause");
            continue;
        }

        //开始解封装
        XData d = Read();
        if (d.size > 0) {
            XLOGI("IDemux::Main start demux data.size:%d", d.size);
            Notify(d);
        } else {
            XSleep(2);
        }
    }
}