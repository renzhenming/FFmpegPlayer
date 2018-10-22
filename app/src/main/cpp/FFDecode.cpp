
extern "C"{
#include <libavcodec/avcodec.h>
}
#include "FFDecode.h"
#include "XLog.h"

bool FFDecode ::Open(XParameter xParameter) {
    if (!xParameter.avCodecParameters) return false;
    AVCodecParameters *avCodecParameters = xParameter.avCodecParameters;

    //获取解码器
    AVCodec *avCodec = avcodec_find_decoder(avCodecParameters->codec_id);
    if (!avCodec){
        XLOGE("avcodec_find_decoder %d failed!",avCodecParameters->codec_id);
        return false;
    }
    XLOGI("avcodec_find_decoder success!");

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

    XLOGI("avcodec_open2 success!");
    return true;
}