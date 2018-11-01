
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/jni.h>
}

#include "FFDecode.h"
#include "XLog.h"

void FFDecode::InitHard(void *vm) {
    av_jni_set_java_vm(vm, 0);
}

/**
 * 打开解码器
 * @param xParameter
 * @param isHard 是否硬解码
 * @return
 */
bool FFDecode::Open(XParameter xParameter, bool isHard) {
    Close();
    //parameter在解封装之时就已经区分了音频参数和视频参数，所以，如果传入的parameter时音频
    //那么这里解码就是针对音频解码，如果传入的parameter时视频，那么就是视频解码
    if (!xParameter.avCodecParameters) {
        XLOGE("FFDecode failed!CodecParameters is null");
        return false;
    }
    AVCodecParameters *avCodecParameters = xParameter.avCodecParameters;

    //获取解码器
    AVCodec *avCodec;
    if (isHard) {
        avCodec = avcodec_find_decoder_by_name("h264_mediacodec");
    } else {
        avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
    }

    if (!avCodec) {
        XLOGE("avcodec_find_decoder %d failed!", avCodecParameters->codec_id);
        return false;
    }

    if (xParameter.avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO) {
        XLOGI("video avcodec_find_decoder  %d success! %d ", avCodecParameters->codec_id, isHard);
    } else if (xParameter.avCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO) {
        XLOGI("audio avcodec_find_decoder success!");
    }

    mutex.lock();

    //创建解码器上下文
    avCodecContext = avcodec_alloc_context3(avCodec);

    //复制参数到上下文
    avcodec_parameters_to_context(avCodecContext, avCodecParameters);

    //打开解码器
    int result = avcodec_open2(avCodecContext, 0, 0);
    if (result != 0) {
        mutex.unlock();
        char buf[1024] = {0};
        av_strerror(result, buf, sizeof(buf) - 1);
        XLOGE("avcodec_open2 failed %s", buf);
        return false;
    }

    if (avCodecContext->codec_type == AVMEDIA_TYPE_VIDEO) {
        this->isVideo = true;
        this->isAudio = false;

        XLOGI("video avcodec_open2 success!");
    } else if (avCodecContext->codec_type == AVMEDIA_TYPE_AUDIO) {
        this->isVideo = false;
        this->isAudio = true;

        XLOGI("audio avcodec_open2 success!");
    }
    mutex.unlock();
    return true;
}

void FFDecode::Close(){
    IDecode::Clear();
    mutex.lock();

    pts = 0;
    if(frame){
        av_frame_free(&frame);
    }

    if(avCodecContext){
        avcodec_close(avCodecContext);
        avcodec_free_context(&avCodecContext);
    }

    mutex.unlock();
}

/**
 * future模型 发送数据到线程解码
 * @param packet
 * @return
 */
bool FFDecode::SendPacket(XData packet) {
    if (packet.size <= 0 || !packet.data)return false;

    mutex.lock();
    if (!avCodecContext) {
        mutex.unlock();
        return false;
    }

    int result = avcodec_send_packet(avCodecContext, (AVPacket *) packet.data);

    mutex.unlock();
    if (result != 0) {
        return false;
    }

    return true;
}

/**
 * 从线程中获取解码结果
 * @return
 */
XData FFDecode::RecvFrame() {
    mutex.lock();
    if (!avCodecContext) {
        mutex.unlock();
        return XData();
    }

    if (!frame) {
        frame = av_frame_alloc();
    }
    //从解码线程中获取帧数据
    int result = avcodec_receive_frame(avCodecContext, frame);

    if (result != 0) {
        mutex.unlock();
        return XData();
    }

    XData data;
    data.data = (unsigned char *) frame;

    if (avCodecContext->codec_type == AVMEDIA_TYPE_VIDEO) {
        //视频size计算方法0 1 2 分别对应yuv,三者相加相当于一行的size 乘以高度

        data.size = (frame->linesize[0] + frame->linesize[1] + frame->linesize[2]) * frame->height;
        data.width = frame->width;
        data.height = frame->height;

        XLOGI("video format is %d", frame->format);
    } else if (avCodecContext->codec_type == AVMEDIA_TYPE_AUDIO) {

        //音频size计算方法 ： 样本字节数 * 单通道样本数 * 通道数
        data.size = av_get_bytes_per_sample((AVSampleFormat) frame->format) * frame->nb_samples * 2;
    }
    data.format = frame->format;

    memcpy(data.datas, frame->data, sizeof(data.datas));

    data.pts = frame->pts;

    mutex.unlock();
    return data;
}
























