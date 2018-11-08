
#include "FFDemux.h"
#include "XLog.h"

extern "C" {
#include "libavformat/avformat.h"
}


//分数转为浮点数
/**
 * PTS：Presentation Time Stamp。PTS主要用于度量解码后的视频帧什么时候被显示出来
DTS：Decode Time Stamp。DTS主要是标识读入内存中的ｂｉｔ流在什么时候开始送入解码器中进行解码

也就是pts反映帧什么时候开始显示，dts反映数据流什么时候开始解码

怎么理解这里的“什么时候”呢？如果有某一帧，假设它是第10秒开始显示。那么它的pts是多少呢。是10？还是10s？还是两者都不是。

为了回答这个问题，先引入FFmpeg中时间基的概念，也就是time_base。它也是用来度量时间的。
如果把1秒分为25等份，你可以理解就是一把尺，那么每一格表示的就是1/25秒。此时的time_base={1，25}
如果你是把1秒分成90000份，每一个刻度就是1/90000秒，此时的time_base={1，90000}。
所谓时间基表示的就是每个刻度是多少秒
pts的值就是占多少个时间刻度（占多少个格子）。它的单位不是秒，而是时间刻度。只有pts加上time_base两者同时在一起，才能表达出时间是多少。
好比我只告诉你，某物体的长度占某一把尺上的20个刻度。但是我不告诉你，这把尺总共是多少厘米的，你就没办法计算每个刻度是多少厘米，你也就无法知道物体的长度。
pts=20个刻度
time_base={1,10} 每一个刻度是1/10厘米
所以物体的长度=pts*time_base=20*1/10 厘米

在ffmpeg中。av_q2d(time_base)=每个刻度是多少秒
此时你应该不难理解 pts*av_q2d(time_base)才是帧的显示时间戳。

下面理解时间基的转换，为什么要有时间基转换。
首先，不同的封装格式，timebase是不一样的。另外，整个转码过程，不同的数据状态对应的时间基也不一致。拿mpegts封装格式25fps来说（只说视频，音频大致一样，但也略有不同）。非压缩时候的数据（即YUV或者其它），在ffmpeg中对应的结构体为AVFrame,它的时间基为AVCodecContext 的time_base ,AVRational{1,25}。
压缩后的数据（对应的结构体为AVPacket）对应的时间基为AVStream的time_base，AVRational{1,90000}。
因为数据状态不同，时间基不一样，所以我们必须转换，在1/25时间刻度下占10格，在1/90000下是占多少格。这就是pts的转换。

根据pts来计算一桢在整个视频中的时间位置：
timestamp(秒) = pts * av_q2d(st->time_base)

duration和pts单位一样，duration表示当前帧的持续时间占多少格。或者理解是两帧的间隔时间是占多少格。一定要理解单位。
pts：格子数
av_q2d(st->time_base): 秒/格

计算视频长度：
time(秒) = st->duration * av_q2d(st->time_base)

ffmpeg内部的时间与标准的时间转换方法：
ffmpeg内部的时间戳 = AV_TIME_BASE * time(秒)
AV_TIME_BASE_Q=1/AV_TIME_BASE

av_rescale_q(int64_t a, AVRational bq, AVRational cq)函数
这个函数的作用是计算a*bq / cq来把时间戳从一个时间基调整到另外一个时间基。在进行时间基转换的时候，应该首先这个函数，因为它可以避免溢出的情况发生。
函数表示在bq下的占a个格子，在cq下是多少。

关于音频pts的计算：
音频sample_rate:samples per second，即采样率，表示每秒采集多少采样点。
比如44100HZ，就是一秒采集44100个sample.
即每个sample的时间是1/44100秒

一个音频帧的AVFrame有nb_samples个sample，所以一个AVFrame耗时是nb_samples*（1/44100）秒
即标准时间下duration_s=nb_samples*（1/44100）秒，
转换成AVStream时间基下
duration=duration_s / av_q2d(st->time_base)
基于st->time_base的num值一般等于采样率,所以duration=nb_samples.
pts=n*duration=n*nb_samples

补充：
next_pts-current_pts=current_duration,根据数学等差公式an=a1+(n-1)*d可得pts=n*d
 * @param r
 * @return
 */
static double r2d(AVRational r) {
    //av_q2d(time_base)=每个刻度是多少秒 ,pts*av_q2d(time_base)才是帧的显示时间戳。
    return r.num == 0 || r.den == 0 ? 0. : (double) r.num / (double) r.den;
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
 * position是总时长的百分比，不是具体的位置
 */
bool FFDemux::Seek(double percent){
    if(percent < 0 || percent >1){
        XLOGE("FFDemux::Seek seek position must between 0.0~1.0");
        return false;
    }

    bool result = false;

    mutex.lock();

    if(!avFormatContext){
        mutex.unlock();
        return false;
    }

    //清理读取的缓存
    avformat_flush(avFormatContext);

    long long seekPts = 0;

    //总时间*seek百分比 = seek时间点
    seekPts = avFormatContext->streams[videoStream]->duration*percent;

    result = av_seek_frame(avFormatContext,videoStream,seekPts,AVSEEK_FLAG_FRAME|AVSEEK_FLAG_BACKWARD);

    mutex.unlock();

    return result;
}

/**
 * 打开文件，或者流媒体 rmtp http rtsp
 * @param url
 * @return
 */
bool FFDemux::Open(const char *url) {

    Close();
    mutex.lock();
    //打开视频文件
    XLOGI("begin open file %s", url);

    //avformat_close_input何时调用？
    int result = avformat_open_input(&avFormatContext, url, 0, 0);
    if (result != 0) {
        mutex.unlock();
        char buf[1024] = {0};
        av_strerror(result, buf, sizeof(buf));
        XLOGE("FFDemux open %s failed! %s", url, buf);
        return false;
    }
    XLOGI("FFDemux open %s success!", url);

    //读取视频信息
    result = avformat_find_stream_info(avFormatContext, 0);
    if (result != 0) {
        mutex.unlock();
        char buf[1024] = {0};
        av_strerror(result, buf, sizeof(buf));
        XLOGE("FFDemux avformat_find_stream_info %s failed! %s", url, buf);
        return false;
    }
    this->totalMs = avFormatContext->duration / (AV_TIME_BASE / 1000);
    XLOGI("FFDemux avformat_find_stream_info %s success!totalMs =%d", url, totalMs);

    mutex.unlock();
    //解封装之后获取到视频和音频的参数
    GetVParam();
    GetAParam();
    return true;
};

void FFDemux:: Close(){
    mutex.lock();
    if(avFormatContext){
        avformat_close_input(&avFormatContext);
    }
    mutex.unlock();
}

/**
 * 获取视频参数
 * @return
 */
XParameter FFDemux::GetVParam() {
    mutex.lock();
    if (!avFormatContext) {
        mutex.unlock();
        XLOGE("GetVParam failed !AVFormatContext is null");
        return XParameter();
    }

    //获取视频流索引
    int videoIndex = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, 0, 0);
    if (videoIndex < 0) {
        mutex.unlock();
        XLOGE("av_find_best_stream video failed");
        return XParameter();
    }

    this->videoStream = videoIndex;
    //获取参数对象
    XParameter parameter;
    parameter.avCodecParameters = avFormatContext->streams[videoIndex]->codecpar;
    mutex.unlock();
    return parameter;
}

/**
 * 获取音频参数
 * @return
 */
XParameter FFDemux::GetAParam() {
    mutex.lock();
    if (!avFormatContext) {
        mutex.unlock();
        XLOGE("GetAParam failed !AVFormatContext is null");
        return XParameter();
    }

    //获取视频流索引
    int audioIndex = av_find_best_stream(avFormatContext, AVMEDIA_TYPE_AUDIO, -1, -1, 0, 0);
    if (audioIndex < 0) {
        mutex.unlock();
        XLOGE("av_find_best_stream audio failed");
        return XParameter();
    }

    this->audioStream = audioIndex;

    //获取参数对象
    XParameter parameter;
    parameter.avCodecParameters = avFormatContext->streams[audioIndex]->codecpar;
    parameter.channels = avFormatContext->streams[audioStream]->codecpar->channels;
    parameter.sample_rate = avFormatContext->streams[audioStream]->codecpar->sample_rate;
    mutex.unlock();
    return parameter;
}

/**
 * 解码一帧数据，数据由调用者清理
 */
XData FFDemux::Read() {
    mutex.lock();
    if (!avFormatContext){
        mutex.unlock();
        return XData();
    }
    XData d;
    AVPacket *avPacket = av_packet_alloc();
    int result = av_read_frame(avFormatContext, avPacket);
    if (result != 0) {
        mutex.unlock();
        av_packet_free(&avPacket);
        return XData();
    }
    //XLOGI("pack size is %d ptss %lld",avPacket->size,avPacket->pts);
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
        mutex.unlock();
        return XData();
    }

    //转换pts（Presentation Time Stamp PTS主要用于度量解码后的视频帧什么时候被显示出来 ）
    avPacket->pts = avPacket->pts *
                    (1000 * r2d(avFormatContext->streams[avPacket->stream_index]->time_base));

    //DTS :Decode Time Stamp。DTS主要是标识读入内存中的ｂｉｔ流在什么时候开始送入解码器中进行解码
    avPacket->dts = avPacket->dts *
                    (1000 * r2d(avFormatContext->streams[avPacket->stream_index]->time_base));

    d.pts = (int) avPacket->pts;

    mutex.unlock();
    return d;
};

