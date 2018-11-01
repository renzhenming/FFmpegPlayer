//
// Created by renzhenming on 2018/10/26.
//
extern "C" {
#include <libswresample/swresample.h>
#include <libavcodec/avcodec.h>
}

#include "FFResample.h"
#include "XLog.h"

bool FFResample::Open(XParameter in, XParameter out) {
    Close();
    mutex.lock();
    //创建上下文对象
    swrContext = swr_alloc();
    //给重采样上下文填充参数
    swr_alloc_set_opts(
            swrContext,
            //输出的channel layout
            av_get_default_channel_layout(out.channels),
            //输出的样本格式
            AV_SAMPLE_FMT_S16,
            //输出的采样率
            out.sample_rate,
            //输入的channel layout
            av_get_default_channel_layout(in.avCodecParameters->channels),
            //输入的样本格式
            (AVSampleFormat) in.avCodecParameters->format,
            //输入的采样率
            in.avCodecParameters->sample_rate,
            0, 0

    );

    //初始化上下文
    int result = swr_init(swrContext);
    if (result != 0) {
        mutex.unlock();
        XLOGE("swr_init failed!");
        return false;
    } else {
        XLOGI("swr_init success!");
    }

    //保存音频信息
    outChannels = in.avCodecParameters->channels;
    outFormat = AV_SAMPLE_FMT_S16;

    XLOGI("FFResample::Open --> 音频重采样初始化成功");
    mutex.unlock();
    return true;
}

void FFResample::Close() {
    mutex.lock();
    if(swrContext){
        swr_free(&swrContext);
    }
    mutex.unlock();
}

XData FFResample::Resample(XData inData) {
    if (inData.size <= 0 || !inData.data) return XData();

    mutex.lock();
    if (!swrContext) {
        mutex.unlock();
        return XData();
    }

    AVFrame *frame = (AVFrame *) inData.data;

    XData out;
    //输出空间 = 通道数*单通道样本数*样本字节大小
    int outsize =
            outChannels * frame->nb_samples * av_get_bytes_per_sample((AVSampleFormat) outFormat);
    if (outsize <= 0) {
        mutex.unlock();
        return XData();
    }
    out.Alloc(outsize);
    uint8_t *outArr[2] = {0};
    outArr[0] = out.data;
    int len = swr_convert(swrContext, outArr, frame->nb_samples, (const uint8_t **) frame->data,
                          frame->nb_samples);
    if (len <= 0) {
        mutex.unlock();
        out.Drop();
        return XData();
    }
    out.pts = inData.pts;
    XLOGI("FFResample::Resample swr_convert success = %d", len);
    mutex.unlock();
    return out;
}