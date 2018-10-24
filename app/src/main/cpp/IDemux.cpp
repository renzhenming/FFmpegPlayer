#include "IDemux.h"
#include "XLog.h"

void IDemux::Main() {
    while(!isExit){
        XData d = Read();
        //XLOGI("IDemux Read %d",d.size);
        //if(d.size<=0)break;
        if (d.size>0)
            Notify(d);
        else
            XLOGI("IDemux Read result is 0");
    }
}