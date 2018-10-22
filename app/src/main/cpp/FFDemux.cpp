
#include "FFDemux.h"
#include "XLog.h"

extern "C"{
#include "libavformat/avformat.h"
}

FFDemux::FFDemux(){
    static bool isFirst = true;
    if(isFirst){
        isFirst = false;

        //注册封装器
        av_register_all();

        //注册解码器
        avcodec_register_all();

        //初始化网络
        avformat_network_init();

        XLOGI("ffmpeg register success");
    }
}
//打开文件，或者流媒体 rmtp http rtsp
bool FFDemux::Open(const char *url){

    //打开视频文件
    XLOGI("open file %s begin",url);
    int result = avformat_open_input(&avFormatContext,url,0,0);
    if(result != 0){
        char buf[1024]={0};
        av_strerror(result,buf,sizeof(buf));
        XLOGE("FFDemux open %s failed! %s",url,buf);
        return false;
    }
    XLOGI("FFDemux open %s success!",url);

    //读取视频信息
    result = avformat_find_stream_info(avFormatContext,0);
    if(result !=0){
        char buf[1024]={0};
        av_strerror(result,buf,sizeof(buf));
        XLOGE("FFDemux avformat_find_stream_info %s failed! %s",url,buf);
        return false;
    }
    this->totalMs = avFormatContext->duration/(AV_TIME_BASE/1000);
    XLOGI("FFDemux avformat_find_stream_info %s success!totalMs =%d",url,totalMs);
    return true;
};
//读取一帧数据，数据由调用者清理
XData FFDemux::Read(){
    if(!avFormatContext)
        return XData();
    XData d;
    AVPacket *avPacket = av_packet_alloc();
    int result = av_read_frame(avFormatContext,avPacket);
    if(result != 0){
        av_packet_free(&avPacket);
        return XData();
    }
    XLOGI("pack size is %d ptss %lld",avPacket->size,avPacket->pts);
    //为什么*avPacket可以强转成unsigned char*？TODO
    d.data = (unsigned char*)avPacket;
    d.size = avPacket->size;
    return d;
};