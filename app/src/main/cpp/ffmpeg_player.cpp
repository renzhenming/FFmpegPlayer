/***************************************************************************************************
**  neon 单线程下解码视频结果
**       每秒帧数27~68
 *       CPU占用16%左右
 *       内存占用67M左右
**
**  neon 八线程下解码视频结果
**       每秒解码帧数100~170帧
 *       CPU占用70%左右
 *       内存占用90M左右
 *
 *  neon h264_mediacodec硬解码 设置单线程
 *       每秒解码帧数46~58，硬解码是一个固定值，这是由于计算误差产生的
 *       CPU占用3%左右
 *       内存占用23M左右
 *
 *  neon h264_mediacodec硬解码 设置8线程
 *       每秒解码帧数46~85，线程数对帧率无影响，因为硬解码帧率是固定的
 *       CPU和内存的占用可忽略不计
** ************************************************************************************************/
#include <jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"ffmpeg_player_warn",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg_player_error",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg_player_info",__VA_ARGS__)

//千万记住，ffmpeg是C语言编写的，在C++中使用必须开启开启混合编译，不然会一直报错，
//undefined reference to 'xxxx'

//extern "C"的主要作用就是为了能够正确实现C++代码调用其他C语言代码。加上extern "C"后，会指示编译器这部分代码按
//C语言的进行编译，而不是C++的。由于C++支持函数重载，因此编译器编译函数的过程中会将函数的参数类型也加到编
//译后的代码中，而不仅仅是函数名；而C语言并不支持函数重载，因此编译C语言代码的函数时不会带上函数的参数类
//型，一般之包括函数名。

extern "C"{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavcodec/jni.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
};


extern "C"
JNIEXPORT jstring JNICALL
Java_com_rzm_ffmpegplayer_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    const char * configure = avcodec_configuration();
    return env->NewStringUTF(configure);
}

static double r2d(AVRational r) {
    LOGI("r.num= %lld r.den=%lld",r.num,r.den);
    return r.num==0||r.den == 0 ? 0 :(double)r.num/(double)r.den;

}

//当前时间戳 clock
long long GetNowMs() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    int sec = tv.tv_sec%360000;
    long long t = sec*1000+tv.tv_usec/1000;
    return t;
}

extern "C"
JNIEXPORT
jint JNI_OnLoad(JavaVM *vm,void *res){
    av_jni_set_java_vm(vm,0);
    return JNI_VERSION_1_4;
}

/**
 * 播放视频，支持本地和网络两种
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_rzm_ffmpegplayer_FFmpegPlayer_playVideo(JNIEnv *env, jobject instance, jstring url_,
                                                 jobject surface) {
    const char *url = env->GetStringUTFChars(url_, 0);

    //初始化解封装
    av_register_all();
    //初始化全局网络组件，可选推荐使用，在使用网络协议的场景中这是必选的(rtfp http)
    avformat_network_init();
    //初始化所有的解码器
    avcodec_register_all();

    AVFormatContext *avFormatContext = NULL;
    //指定输入的格式，如果为NULL,将自动检测输入格式，所以可置为NULL
    //AVInputFormat *fmt = NULL;
    //打开输入文件，可以是本地视频或者网络视频
    int result = avformat_open_input(&avFormatContext,url,NULL,NULL);

    //打开输入内容失败
    if(result != 0){
        LOGE("avformat_open_input failed!:%s",av_err2str(result));
        return;
    }

    //打开输入成功
    LOGI("avformat_open_input success!:%s",av_err2str(result));

    //读取媒体文件的分组以获得流信息。这个对于没有标题的文件格式（如MPEG）很有用。这个函数还计算实际的帧率在
    //MPEG-2重复的情况下帧模式。
    result = avformat_find_stream_info(avFormatContext,NULL);
    if (result < 0){
        LOGE("avformat_find_stream_info failed: %s",av_err2str(result));
    }

    //获取到了输入文件信息，打印一下视频时长和nb_streams
    LOGI("duration = %lld nb_streams=%d",avFormatContext->duration,avFormatContext->nb_streams);

    //分离音视频，获取音视频在源文件中的streams index
    int videoIndex = 0;
    int audioIndex = 1;
    int fps = 0;
    for(int i = 0; i < avFormatContext->nb_streams; i++){
        AVStream *avStream = avFormatContext->streams[i];
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            //找到视频index
            videoIndex = i;
            LOGI("video index = %d",videoIndex);
            //FPS是图像领域中的定义，是指画面每秒传输帧数
            fps = r2d(avStream->avg_frame_rate);

            LOGI("video info ---- fps = %d fps den= %d fps num= %d width=%d height=%d code id=%d format=%d",
                 fps,
                 avStream->avg_frame_rate.den,
                 avStream->avg_frame_rate.num,
                 avStream->codecpar->width,
                 avStream->codecpar->height,
                 avStream->codecpar->codec_id,
                 avStream->codecpar->format
            );

        } else if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            //找到音频index
            audioIndex = i;
            LOGI("audio index = %d sampe_rate=%d channels=%d sample_format=%d",
                 audioIndex,
                 avStream->codecpar->sample_rate,
                 avStream->codecpar->channels,
                 avStream->codecpar->format
            );
        }
    }

    //上边通过遍历streams音视频的index,还可以通过提供的接口获取
    //videoIndex = av_find_best_stream(avFormatContext,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    //audioIndex = av_find_best_stream(avFormatContext,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    //LOGI("av_find_best_stream videoIndex=%d audioIndex=%d",videoIndex,audioIndex);

    /***************************************video解码器*********************************************/
    //找到视频解码器(软解码)
    AVCodec *videoAVCodec = avcodec_find_decoder(avFormatContext->streams[videoIndex]->codecpar->codec_id);
    //硬解码，硬解码需要Jni_OnLoad中做设置否则ffmpeg_player_error: avcodec_open2 video failed!
    //AVCodec *videoAVCodec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (videoAVCodec == NULL){
        LOGE("avcodec_find_decoder failed !");
        return;
    }
    //初始化视频解码器上下文对象
    AVCodecContext *videoCodecContext = avcodec_alloc_context3(videoAVCodec);
    //根据所提供的编解码器的值填充编解码器上下文参数
    avcodec_parameters_to_context(videoCodecContext,avFormatContext->streams[videoIndex]->codecpar);
    //设置视频解码器解码的线程数，解码时将会以你设定的线程进行解码
    videoCodecContext->thread_count = 8;
    //打开解码器
    result = avcodec_open2(videoCodecContext,NULL,NULL);
    if (result != 0){
        LOGE("avcodec_open2 video failed! %s",av_err2str(result));
        return;
    }
    /***********************************************************************************************/


    /***************************************audio解码器*********************************************/

    //找到音频解码器(软解码)
    AVCodec *audioAVCodec = avcodec_find_decoder(avFormatContext->streams[audioIndex]->codecpar->codec_id);
    //硬解码
    //codec = avcodec_find_decoder_by_name("h264_mediacodec");
    //初始化音频解码器上下文对象
    AVCodecContext *audioCodecContext = avcodec_alloc_context3(audioAVCodec);
    //根据所提供的编解码器的值填充编解码器上下文参数
    avcodec_parameters_to_context(audioCodecContext,avFormatContext->streams[audioIndex]->codecpar);
    //设置音频解码器解码的线程数，解码时将会以你设定的线程进行解码
    audioCodecContext->thread_count = 1;
    //打开音频解码器
    result = avcodec_open2(audioCodecContext,NULL,NULL);
    if (result != 0){
        LOGE("avcodec_open2 audio failed!");
        return;
    }
    /***********************************************************************************************/

    //读取帧数据
    //Allocate an AVPacket and set its fields to default values
    //存储压缩数据,对于视频，它通常应该包含一个压缩帧。对于音频它可能包含几个压缩帧
    //av_packet_free()
    AVPacket *avPacket = av_packet_alloc();
    //av_frame_free() 回收
    AVFrame *avFrame = av_frame_alloc();

    //********************测试每秒解码帧数代码*******************
    long long start = GetNowMs();
    int frameCount = 0;
    //********************测试每秒解码帧数代码*******************

    //*************************像素格式转换******************************
    //像素格式转换的上下文
    SwsContext *swsContext = NULL;
    //像素格式转换的输出宽度和高度
    int destWidth = 1280;
    int destHeight = 720;
    char *rgb = new char[1920*1080*4];
    //*************************像素格式转换******************************

    //********************音频重采样*****************************
    char *pcm = new char[48000*4*2];

    SwrContext *swrContext = swr_alloc();
    //给重采样上下文填充参数
    swrContext = swr_alloc_set_opts(
            swrContext,
            //输出的channel layout
            av_get_default_channel_layout(2),
            //输出的样本格式
            AV_SAMPLE_FMT_S16,
            //输出的采样率
            audioCodecContext->sample_rate,

            //输入的channel layout
            av_get_default_channel_layout(audioCodecContext->channels),
            //输入的样本格式
            audioCodecContext->sample_fmt,
            //输入的采样率
            audioCodecContext->sample_rate,
            NULL,
            NULL

    );
    //swr_init(), swr_free()
    //设置参数之后进行上下文初始化
    result = swr_init(swrContext);
    if (result != 0){
        LOGW("swr_init failed!");
    } else {
        LOGW("swr_init success!");
    }
    //********************音频重采样*****************************

    for (;;) {

        //********************测试每秒解码帧数代码*******************
        if(GetNowMs() - start >=3000){
            LOGI("now decode fps is %d",frameCount/3);
            start = GetNowMs();
            frameCount = 0;
        }
        //********************测试每秒解码帧数代码*******************

        //Return the next frame of a stream.
        int read_result = av_read_frame(avFormatContext,avPacket);
        if(read_result != 0){
            //读取到结尾处,从20秒位置继续开始播放
            LOGI("读取到结尾处 %s",av_err2str(read_result));
            //跳转到指定的position播放，最后一个参数表示
            //int pos = 200000 * r2d(avFormatContext->streams[videoIndex]->time_base);
            //av_seek_frame(avFormatContext,videoIndex,pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME );
            //LOGI("avFormatContext->streams[videoIndex]->time_base= %d",avFormatContext->streams[videoIndex]->time_base);
            //continue;
            break;
        }
        LOGW("stream = %d size =%d pts=%lld flag=%d pos = %d",
             avPacket->stream_index,avPacket->size,avPacket->pts,avPacket->flags,avPacket->pos
        );

        //解码测试
        /*if(avPacket->stream_index != videoIndex){
            continue;
        }*/

        AVCodecContext *codecContext = videoCodecContext;
        if (avPacket->stream_index == audioIndex){
            codecContext = audioCodecContext;
        }

        //将packet发送到解码器中进行解码
        read_result = avcodec_send_packet(codecContext,avPacket);
        if (read_result != 0){
            LOGE("avcodec_send_packet failed! %s",av_err2str(read_result));
            continue;
        }
        LOGI("avcodec_send_packet success! %s",av_err2str(read_result));
        //packet使用完成之后执行，否则内存会急剧增长
        //不再引用这个packet指向的空间，并且将packet置为default状态
        //avcodec_send_packet执行之后，这个packet对象就已经没有用了，
        //此时这个packet就像一个壳，他把自己包装的data交给了解码器，
        //这个壳就没有用了
        av_packet_unref(avPacket);

        for(;;){
            if(codecContext == videoCodecContext){
                LOGI("begin receive video frame!");
            }else{
                LOGI("begin receive audio frame!");
            }
            //从解码器中返回的已经解码的数据
            read_result = avcodec_receive_frame(codecContext,avFrame);
            if(read_result != 0){
                LOGE("avcodec_receive_frame failed! %s",av_err2str(read_result));
                break;
            }


            //视频
            if(codecContext == videoCodecContext) {
                //*****测试每秒解码帧数代码*******
                frameCount++;
                //*****测试每秒解码帧数代码*******

                //*************************像素格式转换******************************
                //sws_getContext()  sws_freeContext()
                swsContext = sws_getCachedContext(
                        swsContext,
                        avFrame->width,
                        avFrame->height,
                        (AVPixelFormat)avFrame->format,
                        destWidth,
                        destHeight,
                        AV_PIX_FMT_RGBA,
                        // flag 指定用于重新缩放的算法和选项 SWS_FAST_BILINEAR双线性的
                        SWS_FAST_BILINEAR,
                        NULL,NULL,NULL
                );
                if (swsContext == NULL){
                    LOGE("sws_getCachedContext failed!");
                }
                uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
                data[0] =(uint8_t *)rgb;
                int lines[AV_NUM_DATA_POINTERS] = {0};
                lines[0] = destWidth * 4;
                int h = sws_scale(
                        swsContext,
                        //输入的源buffer
                        (const uint8_t **)avFrame->data,
                        //输入的stride，可以把stride看做每一行的字节数
                        //对于视频，指每个图片行的字节大小。
                        //对于音频，指每个平面的字节大小
                        avFrame->linesize,
                        //处理的起点位置,0表示从头开始处理
                        0,
                        //源的高度
                        avFrame->height,
                        //输出的缓冲区buffer
                        data,
                        //输出的stride,和输入对应
                        lines

                );
                LOGI("sws_scale = %d",h);
                //*************************像素格式转换******************************
            }else{
                //音频

                //********************音频重采样*****************************
                uint8_t *out[2] = {0};
                out[0] = (uint8_t*) pcm;
                int len = swr_convert(
                              swrContext,
                              //输出缓冲区
                              out,
                              //输出一帧音频含有的样本数
                              avFrame->nb_samples,
                              //输入的源缓冲区
                              (const uint8_t**)avFrame->data,
                              //输入的每帧音频含有的样本数
                              avFrame->nb_samples
                );

                LOGI("swr_convert = %d",len);
                //********************音频重采样*****************************
            }
        }


    }
    delete rgb;
    delete pcm;
    //关闭上下文
    avformat_close_input(&avFormatContext);
    env->ReleaseStringUTFChars(url_, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_rzm_ffmpegplayer_FFmpegPlayer_play(JNIEnv *env, jobject instance, jstring j_url,
                                            jobject suface) {
    const char *c_url = env->GetStringUTFChars(j_url, 0);

    //初始化解封装
    av_register_all();
    //初始化网络
    avformat_network_init();
    avcodec_register_all();

    AVFormatContext *avFormatContext;
    //打开视频文件，将文件信息封装到AVFormatContext中
    int result = avformat_open_input(&avFormatContext,c_url,NULL,NULL);
    if(result != 0){
        LOGE("avformat_open_input failed!:%s",av_err2str(result));
        return;
    }
    LOGI("avformat_open_input %s success!",c_url);

    //读取视频流信息
    result = avformat_find_stream_info(avFormatContext,NULL);
    if(result != 0){
        LOGE("avformat_find_stream_info failed: %s",av_err2str(result));
        return;
    }
    LOGI("avformat_find_stream_info success,duration = %lld nb_streams = %d",avFormatContext->duration,avFormatContext->nb_streams);

    int videoIndexOfStream;
    int audioIndexOfStream;

    //帧率，要避免动作不流畅的最低是30
    int fps;

    //一个视频中含有好几条流信息，视频流 音频流 字幕流等等，从中找到视频流
    for (int i =0 ; i < avFormatContext->nb_streams ; i++){
        //每一个流都被封装在AVStream中
        AVStream *avStream = avFormatContext->streams[i];

        //找到video对应的AVStream，记录他的index
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){

            videoIndexOfStream = i;
            LOGI("find video stream index=%d",i);
            fps = r2d(avStream->avg_frame_rate);
            LOGI("fps = %d,width=%d height= %d code_id = %d format=%d",fps,avStream->codecpar->width,
                 avStream->codecpar->height,avStream->codecpar->codec_id,avStream->codecpar->format);

        }else if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){

            audioIndexOfStream = i;
            LOGI("find audio stream index=%d",i);
            /**
             * 采样率（Sample Rate）：每秒从连续信号中提取并组成离散信号的采样个数，它用赫兹（Hz）来表示。
             * 一般音乐CD的采样率是44100Hz，所以视频编码中的音频采样率保持在这个级别就完全足够了，通常视频转换器也将这个采样率作为默认设置。
             *
             * channels:声道数是指支持能不同发声的音响的个数，它是衡量音响设备的重要指标之一,左声道 右声道 立体声等等
             */
            LOGI("sample_rate-%d,channels=%d sample_format=%d",avStream->codecpar->sample_rate,
                 avStream->codecpar->channels,avStream->codecpar->format);
        }
    }

    //获取音频流信息
    //audioIndexOfStream = av_find_best_stream(avFormatContext,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    LOGE("av_find_best_stream audioStream = %d",audioIndexOfStream);

    //打开视频解码器
    //软解码器
    AVCodec* videoCodec = avcodec_find_decoder(avFormatContext->streams[videoIndexOfStream]->codecpar->codec_id);
    //硬解码
    //videoCodec = avcodec_find_decoder_by_name("h264_mediacodec");
    if(videoCodec == NULL){
        LOGE("videoCodec find failed!");
        return;
    }

    //视频解码器初始化,获取到视频解码器相关信息封装到AVCodecContext
    AVCodecContext *videoCodecContext = avcodec_alloc_context3(videoCodec);
    //视频信息存储到上下文对象
    avcodec_parameters_to_context(videoCodecContext,avFormatContext->streams[videoIndexOfStream]->codecpar);
    videoCodecContext->thread_count = 8;
    result = avcodec_open2(videoCodecContext,NULL,0);
    LOGI("videoCodecContext timebase = %d/ %d",videoCodecContext->time_base.num,videoCodecContext->time_base.den);
    if (result != 0){
        LOGE("avcodec_open2 video failed!");
        return;
    }

    //打开音频解码器
    //软解码器
    AVCodec* audioCodec = avcodec_find_decoder(avFormatContext->streams[audioIndexOfStream]->codecpar->codec_id);
    //硬解码
    //audioCodec = avcodec_find_decoder_by_name("h264_mediacodec");
    if(audioCodec == NULL){
        LOGE("audioCodec find failed!");
        return;
    }

    //音频解码器初始化,获取到音频解码器相关信息封装到AVCodecContext
    AVCodecContext * audioCodecContext = avcodec_alloc_context3(audioCodec);
    //音频信息存储到上下文对象
    avcodec_parameters_to_context(audioCodecContext,avFormatContext->streams[audioIndexOfStream]->codecpar);
    audioCodecContext->thread_count = 8;
    result = avcodec_open2(audioCodecContext,NULL,0);
    if(result != 0){
        LOGE("avcodec_open2  audio failed!");
        return;
    }

    //读取帧数据
    AVPacket *avPacket = av_packet_alloc();
    AVFrame *avFrame = av_frame_alloc();
    long long start = GetNowMs();
    int frameCount = 0;

    //初始化像素格式转换的上下文


    //音频重采样上下文初始化
    SwrContext *swrContext = swr_alloc();
    swr_alloc_set_opts(swrContext,av_get_default_channel_layout(2),
                       AV_SAMPLE_FMT_S16,audioCodecContext->sample_rate,
                       av_get_default_channel_layout(audioCodecContext->channels),
                       audioCodecContext->sample_fmt,audioCodecContext->sample_rate,
                       0,0);

    result = swr_init(swrContext);
    if (result != 0){
        LOGE("swr_init failed!");
    }else{
        LOGI("swr_init succeed!");
    }

    //初始化像素格式转换的上下文
    SwsContext *vctx = NULL;
    int outWidth = 1280;
    int outHeight = 720;
    char *rgb = new char[1920*1080*4];
    char *pcm = new char[48000*4*2];

    //初始化显示窗口
    ANativeWindow *aNativeWindow = ANativeWindow_fromSurface(env,suface);
    ANativeWindow_setBuffersGeometry(aNativeWindow,outWidth,outHeight,WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer aNativeWindowBuffer;

    for(;;){
        //超过3秒
        if(GetNowMs() - start >=3000){
            LOGI("now decode fps is %d",frameCount/3);
            start = GetNowMs();
            frameCount = 0;
        }

        int re = av_read_frame(avFormatContext,avPacket);
        if(re != 0)
        {

            LOGW("读取到结尾处!");
            int pos = 20 * r2d(avFormatContext->streams[videoIndexOfStream]->time_base);
            av_seek_frame(avFormatContext,videoIndexOfStream,pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME );
            break;
        }

        AVCodecContext *cc=videoCodecContext;
        if(avPacket->stream_index == audioIndexOfStream){
            cc=audioCodecContext;
        }

        //发送到线程中解码
        re = avcodec_send_packet(cc,avPacket);
        //清理
        int p = avPacket->pts;
        av_packet_unref(avPacket);

        if (re != 0){
            LOGE("avcodec_send_packet failed!");
            continue;
        }

        for(;;)
        {
            re = avcodec_receive_frame(cc,avFrame);
            if(re !=0)
            {
                //LOGW("avcodec_receive_frame failed!");
                break;
            }
            //LOGW("avcodec_receive_frame %lld",frame->pts);
            //如果是视频帧
            if(cc == videoCodecContext)
            {
                frameCount++;
                vctx = sws_getCachedContext(vctx,
                                            avFrame->width,
                                            avFrame->height,
                                            (AVPixelFormat)avFrame->format,
                                            outWidth,
                                            outHeight,
                                            AV_PIX_FMT_RGBA,
                                            SWS_FAST_BILINEAR,
                                            0,0,0
                );
                if(!vctx)
                {
                    LOGW("sws_getCachedContext failed!");
                }
                else
                {
                    uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
                    data[0] =(uint8_t *)rgb;
                    int lines[AV_NUM_DATA_POINTERS] = {0};
                    lines[0] = outWidth * 4;
                    int h = sws_scale(vctx,
                                      (const uint8_t **)avFrame->data,
                                      avFrame->linesize,0,
                                      avFrame->height,
                                      data,lines);
                    LOGW("sws_scale = %d",h);
                    if(h > 0)
                    {
                        ANativeWindow_lock(aNativeWindow,&aNativeWindowBuffer,0);
                        uint8_t *dst = (uint8_t*)aNativeWindowBuffer.bits;
                        memcpy(dst,rgb,outWidth*outHeight*4);
                        ANativeWindow_unlockAndPost(aNativeWindow);
                    }
                }

            }
            else //音频
            {
                uint8_t *out[2] = {0};
                out[0] = (uint8_t*) pcm;

                //音频重采样
                int len = swr_convert(swrContext,out,
                                      avFrame->nb_samples,
                                      (const uint8_t**)avFrame->data,
                                      avFrame->nb_samples);
                LOGE("swr_convert = %d",len);
            }

        }
    }
    delete rgb;
    delete pcm;

    //关闭上下文
    avformat_close_input(&avFormatContext);

    env->ReleaseStringUTFChars(j_url, c_url);
}