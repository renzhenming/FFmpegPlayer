
#include "FFDemux.h"
#include "XLog.h"

//打开文件，或者流媒体 rmtp http rtsp
bool FFDemux::Open(const char *url){
    XLOGI("open file %s begin",url);
    return true;
};
//读取一帧数据，数据由调用者清理
XData FFDemux::Read(){
    XData d;
    return d;
};