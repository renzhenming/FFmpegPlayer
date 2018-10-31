
#include "FFDemux.h"
#include "XLog.h"

extern "C" {
#include "libavformat/avformat.h"
}

/**
 * 初始化FFmpeg
 */
FFDemux::FFDemux() {
    static bool isFirst = true;
    if (isFirst) {
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

/**
 * 打开文件，或者流媒体 rmtp http rtsp
 * @param url
 * @return
 */
bool FFDemux::Open(const char *url) {

    //打开视频文件
    XLOGI("begin open file %s", url);
    int result = avformat_open_input(&avFormatContext, url, 0, 0);
    if (result != 0) {
        char buf[1024] = {0};
        av_strerror(result, buf, sizeof(buf));
        XLOGE("FFDemux open %s failed! %s", url, buf);
        return false;
    }
    XLOGI("FFDemux open %s success!", url);

    //读取视频信息
    result = avformat_find_stream_info(avFormatContext, 0);
    if (result != 0) {
        char buf[1024] = {0};
        av_strerror(result, buf, sizeof(buf));
        XLOGE("FFDemux avformat_find_stream_info %s failed! %s", url, buf);
        return false;
    }
    this->totalMs = avFormatContext->duration / (AV_TIME_BASE / 1000);
    XLOGI("FFDemux avformat_find_stream_info %s success!totalMs =%d", url, totalMs);

    //解封装之后获取到视频和音频的参数
    GetVParam();
    GetAParam();
    return true;
};

/**
 * 获取视频参数
 * @return
 */
XParameter FFDemux::GetVParam() {
    if (!avFormatContext) {
        XLOGE("GetVParam failed !AVFormatContext is null");
        return XParameter();
    }

    //获取视频流索引
    int videoIndex = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
    if (videoIndex < 0) {
        XLOGE("av_find_best_stream video failed");
        return XParameter();
    }

    this->videoStream = videoIndex;
    //获取参数对象
    XParameter parameter;
    parameter.avCodecParameters = avFormatContext->streams[videoIndex]->codecpar;

    return parameter;
}

/**
 * 获取音频参数
 * @return
 */
XParameter FFDemux::GetAParam() {
    if (!avFormatContext) {
        XLOGE("GetAParam failed !AVFormatContext is null");
        return XParameter();
    }

    //获取视频流索引
    int audioIndex = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);
    if (audioIndex < 0) {
        XLOGE("av_find_best_stream audio failed");
        return XParameter();
    }

    this->audioStream = audioIndex;

    //获取参数对象
    XParameter parameter;
    parameter.avCodecParameters = avFormatContext->streams[audioIndex]->codecpar;
    parameter.channels = avFormatContext->streams[audioStream]->codecpar->channels;
    parameter.sample_rate = avFormatContext->streams[audioStream]->codecpar->sample_rate;
    return parameter;
}

/**
 * 解码一帧数据，数据由调用者清理
 */
XData FFDemux::Read() {
    if (!avFormatContext)
        return XData();
    XData d;
    AVPacket *avPacket = av_packet_alloc();
    int result = av_read_frame(avFormatContext, avPacket);
    if (result != 0) {
        av_packet_free(&avPacket);
        return XData();
    }
    //XLOGI("pack size is %d ptss %lld",avPacket->size,avPacket->pts);
    //为什么*avPacket可以强转成unsigned char*？TODO
    d.data = (unsigned char *) avPacket;
    d.size = avPacket->size;

    if (avPacket->stream_index == audioStream) {
        d.isAudio = true;
        d.isVideo = false;
    } else if (avPacket->stream_index == videoStream) {
        d.isVideo = true;
        d.isAudio = false;
    } else {
        av_packet_free(&avPacket);
        return XData();
    }

    return d;
};