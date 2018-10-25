
extern "C"{
#include <libavcodec/avcodec.h>
}
#include "FFDecode.h"
#include "XLog.h"

/**
 * 打开解码器
 * @param xParameter
 * @return
 */
bool FFDecode ::Open(XParameter xParameter) {
    if (!xParameter.avCodecParameters) {
        XLOGE("FFDecode failed!CodecParameters is null");
        return false;
    }
    AVCodecParameters *avCodecParameters = xParameter.avCodecParameters;

    //获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
    if (!avCodec){
        XLOGE("avcodec_find_decoder %d failed!",avCodecParameters->codec_id);
        return false;
    }

    if (xParameter.avCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO){
        XLOGI("video avcodec_find_decoder success!");
    }else if(xParameter.avCodecParameters->codec_type == AVMEDIA_TYPE_AUDIO){
        XLOGI("audio avcodec_find_decoder success!");
    }


    //创建解码器上下文
    avCodecContext = avcodec_alloc_context3(avCodec);

    //复制参数到上下文
    avcodec_parameters_to_context(avCodecContext,avCodecParameters);

    //打开解码器
    int result = avcodec_open2(avCodecContext,0,0);
    if (result != 0){
        char buf[1024] = {0};
        av_strerror(result,buf, sizeof(buf)-1);
        XLOGE("avcodec_open2 failed %s",buf);
        return false;
    }

    if(avCodecContext->codec_type == AVMEDIA_TYPE_VIDEO){
        this->isVideo = true;
        this->isAudio = false;

        XLOGI("video avcodec_open2 success!");
    }else if(avCodecContext->codec_type == AVMEDIA_TYPE_AUDIO){
        this->isVideo = false;
        this->isAudio = true;

        XLOGI("audio avcodec_open2 success!");
    }
    return true;
}

/**
 * future模型 发送数据到线程解码
 * @param packet
 * @return
 */
bool FFDecode ::SendPacket(XData packet){
    if(packet.size <= 0 || !packet.data)return false;

    if (!avCodecContext){
        return false;
    }

    int result = avcodec_send_packet(avCodecContext,(AVPacket *)packet.data);

    if(result != 0){
        return false;
    }

    return true;
}

/**
 * 从线程中获取解码结果
 * @return
 */
XData FFDecode ::RecvFrame(){
    if(!avCodecContext){
        return XData();
    }

    if(!frame){
        frame = av_frame_alloc();
    }
    //从解码线程中获取帧数据
    int result = avcodec_receive_frame(avCodecContext,frame);

    if (result != 0){
        return XData();
    }

    XData data;
    data.data = (unsigned char *)frame;

    if (avCodecContext->codec_type == AVMEDIA_TYPE_VIDEO){
        //视频size计算方法0 1 2 分别对应yuv,三者相加相当于一行的size 乘以高度

        data.size = (frame->linesize[0]+frame->linesize[1]+frame->linesize[2])*frame->height;
        data.width = frame->width;
        data.height = frame->height;
    }else if(avCodecContext->codec_type == AVMEDIA_TYPE_AUDIO){

        //音频size计算方法 ： 样本字节数 * 单通道样本数 * 通道数
        data.size = av_get_bytes_per_sample((AVSampleFormat)frame->format)*frame->nb_samples*2;
    }

    memcpy(data.datas,frame->data, sizeof(data.datas));

    return data;
}























