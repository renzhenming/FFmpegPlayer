#include "IDemux.h"
#include "XLog.h"

void IDemux::Main() {
    while(!isExit){
        XData d = Read();
        //XLOGI("IDemux Read %d",d.size);
        //if(d.size<=0)break;
        if (d.size>0){
            Notify(d);
            XLOGI("demux data.size:%d",d.size);
        } else{
            XLOGI("demux reach the end break");
            break;
        }
    }
}