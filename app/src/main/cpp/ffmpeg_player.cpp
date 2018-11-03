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
#include <string>
//播放视频
#include <android/native_window.h>
#include <android/native_window_jni.h>

//播放音频
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

//OpenGLES EGL
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,"ffmpeg_player_warn",__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg_player_error",__VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,"ffmpeg_player_info",__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_ERROR,"ffmpeg_player_info",__VA_ARGS__)

//千万记住，ffmpeg是C语言编写的，在C++中使用必须开启开启混合编译，不然会一直报错，
//undefined reference to 'xxxx'

//extern "C"的主要作用就是为了能够正确实现C++代码调用其他C语言代码。加上extern "C"后，会指示编译器这部分代码按
//C语言的进行编译，而不是C++的。由于C++支持函数重载，因此编译器编译函数的过程中会将函数的参数类型也加到编
//译后的代码中，而不仅仅是函数名；而C语言并不支持函数重载，因此编译C语言代码的函数时不会带上函数的参数类
//型，一般之包括函数名。

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
};


#include "FFDemux.h"
#include "IObserver.h"
#include "XLog.h"
#include "IDecode.h"
#include "FFDecode.h"
#include "IVideoView.h"
#include "GLVideoView.h"
#include "FFResample.h"
#include "IAudioPlay.h"
#include "SLAudioPlay.h"
#include "IPlayerProxy.h"

extern "C"
JNIEXPORT void JNICALL
Java_com_rzm_ffmpegplayer_FFmpegPlayer_initView(JNIEnv *env, jobject instance, jobject surface) {

    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    IPlayerProxy::Get()->InitView(window);
}


/**
重复打开报错：
E/libOpenSLES: frameworks/wilhelm/src/itf/IPlay.c:38: pthread_mutex_lock_timeout_np returned 110
E/XPlay: IDecode ::Main 从解码器种读取解码后的数据失败  frame.size=
E/libEGL: eglCreateWindowSurface: native_window_api_connect (win=0xcbd7e008) failed (0xffffffed) (already connected to another API?)
eglCreateWindowSurface:481 error 3003 (EGL_BAD_ALLOC)
E/XPlay: eglCreateWindowSurface failed!
XTexture init -- > EGL init failed
call to OpenGL ES API with no current context (logged once per thread)

原因分析
再次打开视频的时候我们是从另一个页面退回到视频播放页面，此时surfaceCreated方法会再次回调，传递surface过来，但是要知道，传递
surface和打开视频播放是两个线程进行的，当第二次点击播放按钮的时候，会立即重新调用Java_com_rzm_ffmpegplayer_OpenUrl_Open
播放视频，但此时，surfaceCreated方法未必已经在打开之前传递了surface，导致，第二次播放视频使用了第一次传递的surface，大概原因就是这样
*/

extern "C"
JNIEXPORT void JNICALL
Java_com_rzm_ffmpegplayer_OpenUrl_Open(JNIEnv *env, jobject instance, jstring path) {
    const char *url = env->GetStringUTFChars(path,0);
    IPlayerProxy::Get()->Open(url);
    IPlayerProxy::Get()->Start();

    env->ReleaseStringUTFChars(path,url);
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_rzm_ffmpegplayer_FFmpegPlayer_getCurrentPosition(JNIEnv *env, jclass instance) {
    return IPlayerProxy::Get()->PlayPos();
}

static double r2d(AVRational r) {
    LOGI("r.num= %d r.den=%d", r.num, r.den);
    return r.num == 0 || r.den == 0 ? 0 : (double) r.num / (double) r.den;

}

//当前时间戳 clock
long long GetNowMs() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int sec = tv.tv_sec % 360000;
    long long t = sec * 1000 + tv.tv_usec / 1000;
    return t;
}

extern "C"
JNIEXPORT
jint JNI_OnLoad(JavaVM *vm, void *res) {

    IPlayerProxy::Get()->Init(vm);
    /*IPlayerProxy::Get()->Open("/sdcard/take.mp4");

    IPlayerProxy::Get()->Open("/sdcard/Qiuyinong.mp4");

    IPlayerProxy::Get()->Open("/sdcard/1080.mp4");

    IPlayerProxy::Get()->Start();*/

    return JNI_VERSION_1_4;
}

/*
IVideoView *view = NULL;
extern "C"
JNIEXPORT
jint JNI_OnLoad(JavaVM *vm, void *res) {
    //av_jni_set_java_vm(vm, 0);
    FFDecode::InitHard(vm);


    //FFDemux创建时会做一些初始化的动作
        IDemux *demux = new FFDemux();
        //解封装，这一步执行之后，会得到AVFormatContext上下文对象，同时音视频
        //轨道index也会被赋值
        //demux->Open("/sdcard/1080.mp4");

        IDecode *videoDecode = new FFDecode();
        //打开视频解码器
        //videoDecode->Open(demux->GetVParam(),true);

        IDecode *audioDecode = new FFDecode();
        //打开音频解码器
        //audioDecode->Open(demux->GetAParam());

        //把视频解码器和音频解码器设置到解封装器的观察者，这样，当解封装完成之后，
        //解码器会收到解封装之后的数据XData,开始解码
        demux->AddObserver(videoDecode);
        demux->AddObserver(audioDecode);

        //创建播放器
        view = new GLVideoView();
        //把播放器添加到解码器的观察者队列中，解码一帧完成，播放器就会立即收到
        //解码数据开始进行播放
        videoDecode->AddObserver(view);

        //创建音频重采样对象
        IResample *resample = new FFResample();
        XParameter audioOutParam = demux->GetAParam();

        //初始化音频重采样，可以通过第二个参数设置输出的声道数和采样率，这里默认使用源音频的参数设置
        //resample->Open(demux->GetAParam(),audioOutParam);
        //将重采样器添加到音频解码器的观察者队列，音频解码成功之后会收到数据开始重采样
        audioDecode->AddObserver(resample);

        //创建音频播放器
        IAudioPlay *audioPlay = new SLAudioPlay();
        //将音频播放器添加到重采样器的观察者队列，重采样成功之后开始播放音频
        resample->AddObserver(audioPlay);

        //初始化OpenSLES进行音频播放
        //audioPlay->StartPlay(audioOutParam);

        //解封装器开始解封装视频文件，解封装成功之后加入队列(解码器中内置队列)，队列达到极值后进入等待状态
        //demux->Start();
        //视频解码器开始从队列中取出数据解码，如果队列为空则进入等待状态，
        //videoDecode->Start();
        //音频解码器开始从队列中取出数据解码，如果队列为空则进入等待状态，
        //audioDecode->Start();

    IPlayer::Get()->demux = demux;
    IPlayer::Get()->adecode = audioDecode;
    IPlayer::Get()->vdecode = videoDecode;
    IPlayer::Get()->videoView = view;
    IPlayer::Get()->resample = resample;
    IPlayer::Get()->audioPlay = audioPlay;


    IPlayer::Get()->Open("/sdcard/1080.mp4");
    IPlayer::Get()->Start();

    return JNI_VERSION_1_4;
}*/

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
    int result = avformat_open_input(&avFormatContext, url, 0, 0);

    //打开输入内容失败
    if (result != 0) {
        LOGE("avformat_open_input failed!:%s", av_err2str(result));
        return;
    }

    //打开输入成功
    LOGI("avformat_open_input success!:%s", url);

    //读取媒体文件的分组以获得流信息。这个对于没有标题的文件格式（如MPEG）很有用。这个函数还计算实际的帧率在
    //MPEG-2重复的情况下帧模式。
    result = avformat_find_stream_info(avFormatContext, 0);
    if (result < 0) {
        LOGE("avformat_find_stream_info failed: %s", av_err2str(result));
    }

    //获取到了输入文件信息，打印一下视频时长和nb_streams
    LOGI("duration = %lld nb_streams=%d", avFormatContext->duration, avFormatContext->nb_streams);

    //分离音视频，获取音视频在源文件中的streams index
    int videoIndex = 0;
    int audioIndex = 1;
    int fps = 0;
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        AVStream *avStream = avFormatContext->streams[i];
        if (avStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            //找到视频index
            videoIndex = i;
            LOGI("video index = %d", videoIndex);
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

        } else if (avStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
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
    AVCodec *videoAVCodec = avcodec_find_decoder(
            avFormatContext->streams[videoIndex]->codecpar->codec_id);
    //硬解码，硬解码需要Jni_OnLoad中做设置否则ffmpeg_player_error: avcodec_open2 video failed!
    //AVCodec *videoAVCodec = avcodec_find_decoder_by_name("h264_mediacodec");
    if (videoAVCodec == NULL) {
        LOGE("avcodec_find_decoder failed !");
        return;
    } else {

    }
    //初始化视频解码器上下文对象
    AVCodecContext *videoCodecContext = avcodec_alloc_context3(videoAVCodec);
    //根据所提供的编解码器的值填充编解码器上下文参数
    avcodec_parameters_to_context(videoCodecContext,
                                  avFormatContext->streams[videoIndex]->codecpar);
    //设置视频解码器解码的线程数，解码时将会以你设定的线程进行解码
    videoCodecContext->thread_count = 8;
    //打开解码器
    result = avcodec_open2(videoCodecContext, 0, 0);
    if (result != 0) {
        LOGE("avcodec_open2 video failed! %s", av_err2str(result));
        return;
    }
    /***********************************************************************************************/


    /***************************************audio解码器*********************************************/

    //找到音频解码器(软解码)
    AVCodec *audioAVCodec = avcodec_find_decoder(
            avFormatContext->streams[audioIndex]->codecpar->codec_id);
    //硬解码
    //AVCodec *audioAVCodec = avcodec_find_decoder_by_name("h264_mediacodec");
    //初始化音频解码器上下文对象
    AVCodecContext *audioCodecContext = avcodec_alloc_context3(audioAVCodec);
    //根据所提供的编解码器的值填充编解码器上下文参数
    avcodec_parameters_to_context(audioCodecContext,
                                  avFormatContext->streams[audioIndex]->codecpar);
    //设置音频解码器解码的线程数，解码时将会以你设定的线程进行解码
    audioCodecContext->thread_count = 1;
    //打开音频解码器
    result = avcodec_open2(audioCodecContext, 0, 0);
    if (result != 0) {
        LOGE("avcodec_open2 audio failed!%s", av_err2str(result));
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
    char *rgb = new char[1920 * 1080 * 4];
    //*************************像素格式转换******************************

    //********************音频重采样*****************************
    char *pcm = new char[48000 * 4 * 2];

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
            0, 0
    );
    //swr_init(), swr_free()
    //设置参数之后进行上下文初始化
    result = swr_init(swrContext);
    if (result != 0) {
        LOGW("swr_init failed!");
    } else {
        LOGW("swr_init success!");
    }
    //********************音频重采样*****************************

    //********************窗口初始化*****************************

    //获取到一个java层的Surface
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    //设置播放窗口的属性，包括宽高和格式
    ANativeWindow_setBuffersGeometry(nativeWindow, destWidth, destHeight, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer nativeWindow_buffer;

    //********************窗口初始化*****************************

    for (;;) {

        //********************测试每秒解码帧数代码*******************
        if (GetNowMs() - start >= 3000) {
            LOGI("now decode fps is %d", frameCount / 3);
            start = GetNowMs();
            frameCount = 0;
        }
        //********************测试每秒解码帧数代码*******************

        //Return the next frame of a stream.
        int read_result = av_read_frame(avFormatContext, avPacket);
        if (read_result != 0) {
            //读取到结尾处,从20秒位置继续开始播放
            LOGI("读取到结尾处 %s", av_err2str(read_result));
            //跳转到指定的position播放，最后一个参数表示
            //int pos = 200000 * r2d(avFormatContext->streams[videoIndex]->time_base);
            //av_seek_frame(avFormatContext,videoIndex,pos,AVSEEK_FLAG_BACKWARD|AVSEEK_FLAG_FRAME );
            //LOGI("avFormatContext->streams[videoIndex]->time_base= %d",avFormatContext->streams[videoIndex]->time_base);
            //continue;
            break;
        }
        LOGW("stream = %d size =%d pts=%d flag=%d pos = %d",
             avPacket->stream_index, avPacket->size, avPacket->pts, avPacket->flags, avPacket->pos
        );

        AVCodecContext *codecContext = videoCodecContext;
        if (avPacket->stream_index == audioIndex) {
            codecContext = audioCodecContext;
        }

        //将packet发送到解码器中进行解码
        read_result = avcodec_send_packet(codecContext, avPacket);
        if (read_result != 0) {
            LOGE("avcodec_send_packet failed! %s", av_err2str(read_result));
            continue;
        }
        LOGI("avcodec_send_packet success! %s", av_err2str(read_result));
        //packet使用完成之后执行，否则内存会急剧增长
        //不再引用这个packet指向的空间，并且将packet置为default状态
        //avcodec_send_packet执行之后，这个packet对象就已经没有用了，
        //此时这个packet就像一个壳，他把自己包装的data交给了解码器，
        //这个壳就没有用了
        av_packet_unref(avPacket);

        if (read_result != 0) {
            LOGE("avcodec_send_packet failed!%s", av_err2str(read_result));
            continue;
        }

        for (;;) {
            //从解码器中返回的已经解码的数据
            read_result = avcodec_receive_frame(codecContext, avFrame);
            if (read_result != 0) {
                LOGE("avcodec_receive_frame failed! %s", av_err2str(read_result));
                break;
            } else {
                LOGI("avcodec_receive_frame success! %s", av_err2str(read_result));
            }

            //视频
            if (codecContext == videoCodecContext) {
                //*****测试每秒解码帧数代码*******
                frameCount++;
                //*****测试每秒解码帧数代码*******

                //*************************像素格式转换******************************
                //sws_getContext()  sws_freeContext()
                swsContext = sws_getCachedContext(
                        swsContext,
                        avFrame->width,
                        avFrame->height,
                        (AVPixelFormat) avFrame->format,
                        destWidth,
                        destHeight,
                        AV_PIX_FMT_RGBA,
                        // flag 指定用于重新缩放的算法和选项 SWS_FAST_BILINEAR双线性的
                        SWS_FAST_BILINEAR,
                        0, 0, 0
                );
                if (swsContext == NULL) {
                    LOGE("sws_getCachedContext failed!");
                }
                uint8_t *data[AV_NUM_DATA_POINTERS] = {0};
                data[0] = (uint8_t *) rgb;
                int lines[AV_NUM_DATA_POINTERS] = {0};
                lines[0] = destWidth * 4;
                int h = sws_scale(
                        swsContext,
                        //输入的源buffer
                        (const uint8_t **) avFrame->data,
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
                LOGI("sws_scale = %d", h);

                if (h > 0) {
                    ANativeWindow_lock(nativeWindow, &nativeWindow_buffer, 0);
                    uint8_t *dst = (uint8_t *) nativeWindow_buffer.bits;
                    memcpy(dst, rgb, destWidth * destHeight * 4);
                    ANativeWindow_unlockAndPost(nativeWindow);
                }
                //*************************像素格式转换******************************
            } else {
                //音频

                //********************音频重采样*****************************
                uint8_t *out[2] = {0};
                out[0] = (uint8_t *) pcm;
                int len = swr_convert(
                        swrContext,
                        //输出缓冲区
                        out,
                        //输出一帧音频含有的样本数
                        avFrame->nb_samples,
                        //输入的源缓冲区
                        (const uint8_t **) avFrame->data,
                        //输入的每帧音频含有的样本数
                        avFrame->nb_samples
                );

                LOGI("swr_convert = %d", len);
                //********************音频重采样*****************************
            }
            av_frame_unref(avFrame);

        }


    }
    av_frame_free(&avFrame);
    delete[]rgb;
    delete[]pcm;
    //关闭上下文
    avformat_close_input(&avFormatContext);
    env->ReleaseStringUTFChars(url_, url);
}

void CallBack(SLAndroidSimpleBufferQueueItf bf, void *contex) {
    LOGI("CallBack ....");
    static FILE *fp = NULL;
    static char *buf = NULL;
    if (!buf) {
        buf = new char[1024 * 1024];
    }
    if (!fp) {
        fp = fopen("/sdcard/test.pcm", "rb");
    }
    if (!fp)return;
    if (feof(fp) == 0) {
        int len = fread(buf, 1, 1024, fp);
        if (len > 0)
            (*bf)->Enqueue(bf, buf, len);
    }
}

/**
 * 播放音频
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_rzm_ffmpegplayer_FFmpegPlayer_playAudio(JNIEnv *env, jobject instance, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, 0);

    /***********  1 创建引擎 获取SLEngineItf***************/

    //Object是一个资源的抽象集合,可以通过它获取各种资源,所有的Object在OpenSL里面我们拿到的都是一个SLObjectItf
    SLObjectItf engineObject;
    //Interface则是方法的集合,例如SLRecordItf里面包含了和录音相关的方法,SLPlayItf包含了和播放相关的方法。
    // 我们功能都是通过调用Interfaces的方法去实现的

    //SLEngineItf是OpenSL里面最重要的一个Interface,我们可以通过它去创建各种Object,例如播放器、录音器、
    // 混音器的Object,然后在用这些Object去获取各种Interface去实现各种功能
    SLEngineItf engineInterface;
    //SLresult是unsigned int 类型
    SLresult result;
    result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS)
        return;
    //创建出来之后必须先调用Realize方法做初始化。在不需要使用的时候调用Destroy方法释放资源
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS)
        return;

    //一个Object里面可能包含了多个Interface,所以GetInterface方法有个SLInterfaceID参数来指定到的需要获取Object里面的那个Interface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (result != SL_RESULT_SUCCESS)
        return;
    if (engineInterface) {
        LOGI("get SLEngineItf success");
    } else {
        LOGI("get SLEngineItf failed");
    }
    /***********         1 创建引擎       ***************/

    /***********  2 创建混音器 ***************/
    SLObjectItf mixObjectItf = NULL;
    result = (*engineInterface)->CreateOutputMix(engineInterface, &mixObjectItf, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("CreateOutputMix failed");
        return;
    } else {
        LOGE("CreateOutputMix success");
    }

    //实例化混音器
    result = (*mixObjectItf)->Realize(mixObjectItf, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("mixer init failed");
    } else {
        LOGI("mixer init success");
    }
    /***********  2 创建混音器 ***************/

    /***********  3 配置音频信息 ***************/

    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, mixObjectItf};

    //数据的去处 和SLDataSource是相对的
    SLDataSink slDataSink = {&outputMix, 0};


    //缓冲队列
    SLDataLocator_AndroidSimpleBufferQueue queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10};
    //音频格式
    SLDataFormat_PCM pcmFormat = {
            //播放pcm格式的数据
            SL_DATAFORMAT_PCM,
            //声道数，两个声道，立体声
            2,
            //44100Hz的频率
            SL_SAMPLINGRATE_44_1,
            //位数16位
            SL_PCMSAMPLEFORMAT_FIXED_16,
            //和位数保持一致
            SL_PCMSAMPLEFORMAT_FIXED_16,
            //立体声，前左前右
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            //字节序，小端
            SL_BYTEORDER_LITTLEENDIAN
    };

    //数据的来源
    SLDataSource slDataSource = {&queue, &pcmFormat};
    /***********  3 配置音频信息 ***************/

    /************* 4 创建播放器 ****************/
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_TRUE};
    SLObjectItf slPlayerItf = NULL;
    SLPlayItf slPlayItf;
    SLAndroidSimpleBufferQueueItf pcmQueue = NULL;

    /**
     * SLEngineItf self,
	 * SLObjectItf * pPlayer,
	 * SLDataSource *pAudioSrc, 数据的来源
	 * SLDataSink *pAudioSnk, 数据的去处
	 * SLuint32 numInterfaces, 与下面的SLInterfaceID和SLboolean配合使用,用于标记SLInterfaceID数组和SLboolean的大小
	 * const SLInterfaceID * pInterfaceIds, 这里需要传入一个数组,指定创建的播放器会包含哪些Interface
	 * const SLboolean * pInterfaceRequired  这里也是一个数组,用来标记每个需要包含的Interface
     */
    result = (*engineInterface)->CreateAudioPlayer(engineInterface, &slPlayerItf, &slDataSource,
                                                   &slDataSink, sizeof(ids) / sizeof(SLInterfaceID),
                                                   ids, req);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("create audio player failed");
    } else {
        LOGE("create audio player success");
    }
    //初始化播放器
    result = (*slPlayerItf)->Realize(slPlayerItf, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("audio player init failed");
    } else {
        LOGE("audio player init success");
    }
    //获取player接口
    result = (*slPlayerItf)->GetInterface(slPlayerItf, SL_IID_PLAY, &slPlayItf);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("player get SL_IID_PLAY failed");
    } else {
        LOGI("player get SL_IID_PLAY success");
    }

    //获取播放队列接口
    result = (*slPlayerItf)->GetInterface(slPlayerItf, SL_IID_BUFFERQUEUE, &pcmQueue);
    if (result != SL_RESULT_SUCCESS) {
        LOGE("player get SL_IID_BUFFERQUEUE failed");
    } else {
        LOGI("player get SL_IID_BUFFERQUEUE success");
    }
    /************* 4 创建播放器 ****************/

    //设置回调函数
    (*pcmQueue)->RegisterCallback(pcmQueue, CallBack, 0);
    //设置播放状态
    (*slPlayItf)->SetPlayState(slPlayItf, SL_PLAYSTATE_PLAYING);
    //启动队列
    (*pcmQueue)->Enqueue(pcmQueue, "", 1);

    env->ReleaseStringUTFChars(url_, url);
}

/**
 * 定点着色器glsl
 * #x #可以指定x字符串，这样数组中的字符串就不用加“”
 */
#define GET_STR(x) #x
static const char *vertexShader = GET_STR(
        attribute
        vec4 aPosition; //顶点坐标
        attribute
        vec2 aTexCoord; //材质顶点坐标
        varying
        vec2 vTexCoord;   //输出的纹理坐标
        void main() {
            vTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);
            gl_Position = aPosition;
        }
);

//片元着色器,软解码和部分x86硬解码
static const char *fragYUV420P = GET_STR(
        precision
        mediump float;    //精度
        varying
        vec2 vTexCoord;     //顶点着色器传递的坐标
        uniform
        sampler2D yTexture; //输入的材质（不透明灰度，单像素）
        uniform
        sampler2D uTexture;
        uniform
        sampler2D vTexture;
        void main() {
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(yTexture, vTexCoord).r;
            yuv.g = texture2D(uTexture, vTexCoord).r - 0.5;
            yuv.b = texture2D(vTexture, vTexCoord).r - 0.5;
            rgb = mat3(1.0, 1.0, 1.0,
                       0.0, -0.39465, 2.03211,
                       1.13983, -0.58060, 0.0) * yuv;
            //输出像素颜色
            gl_FragColor = vec4(rgb, 1.0);
        }
);

GLint initShader(const char *code, GLint type) {
    //创建shader
    GLint shader = glCreateShader(type);
    if (shader == 0) {
        LOGE("glCreateShader %d failed", type);
        return 0;
    }
    //加载shader
    glShaderSource(shader,
                   1,//shader数量
                   &code,
                   0//代码长度
    );

    //编译shader
    glCompileShader(shader);

    //获取编译情况
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        LOGE("glCompileShader %d failed", type);
        return 0;
    }
    LOGI("glCompileShader %d success", type);
    return shader;
}


/**
 * OpenGLES (Open Graphics Library for Embedded Systems)适用于手持嵌入式设备，是精简版的OpenGL
 *
 * OpenGL- ES 是免授权费的，跨平台的，功能完善的2D和3D图形应用程序接口API，主要针对多种嵌入式系统专门设计 -
 * 包括控制台、移动电话、手持设备、家电设备和汽车。它由精心定义的桌面OpenGL子集组成，创造了软件与图形加速间
 * 灵活强大的底层交互接口。 OpenGL ES 包含浮点运算和定点运算系统描述以及 EGL针对便携设备的本地视窗系统规范。
 * OpenGL ES 1.X 面向功能固定的硬件所设计并提供加速支持、图形质量及性能标准。OpenGL ES 2.X 则提供包括遮盖器
 * 技术在内的全可编程3D图形算法。OpenGL ES-SC 专为有高安全性需求的特殊市场精心打造
 *
 * 一般EGL和OpenGL ES使用时都会先利用egl函数(egl开头)创建opengl本地环境，然后再利用opengl函数(gl开头)去画图
 *
 * EGL (Embedded Graphics Library):EGL是连接OpenGL ES和本地窗口系统的桥梁,由于OpenGL ES是跨平台的，
 * 引入EGL就是为了屏蔽不同平台上的区别。本地窗口相关的API提供了访问本地窗口系统的接口，EGL提供了创建渲染表面，
 * 接下来OpenGL ES就可以在这个渲染表面上绘制，同时提供了图形上下文，用来进行状态管理
 *
 * OpenGL实现跨平台的功能，在不同的操作系统上需要不同的类似适配层的内容，比如在Windows操作系统上需要WGL。
 * 同样的，OpenGL ES是一个平台中立的图形库，在它能够工作前，需要与一个实际的窗口关联起来，但是，
 * 与OpenGL不一样的是，OpenGL是每个窗口系统需要一个与之对应的适配层，Windows需要WGL，X-Window需要xgl，
 * Mac OS需要agl。而OpenGL ES的这层，是统一的一个标准。这个标准就是EGL。
 *
 * 1.获取Display eglGetDisplay
 * 2.初始化Display eglInitialize
 * 3.选择Config eglChooseConfig
 * 4.创建Surface eglCreateWindowSurface
 * 5.创建Context eglCreateContext
 * 6.绑定Context eglMakeCurrent
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_rzm_ffmpegplayer_FFmpegPlayer_initOpenGL(JNIEnv *env, jobject instance, jstring url_,
                                                  jobject surface) {
    const char *url = env->GetStringUTFChars(url_, 0);

    FILE *fp = fopen(url, "rb");
    if (!fp) {
        LOGE("open file %s failed!", url);
        return;
    }

    /**************************EGL初始化********************************************/

    //1.创建渲染窗口
    ANativeWindow *aNativeWindow = ANativeWindow_fromSurface(env, surface);

    //2.EGL Display创建
    //EGL提供了平台无关类型EGLDisplay表示窗口。定义EGLNativeDisplayType是为了匹配原生窗口系统的显示类型，
    // 对于Windows，EGLNativeDisplayType被定义为HDC，
    // 对于Linux系统，被定义为Display*类型，
    // 对于Android系统，定义为ANativeWindow *类型，
    // 为了方便的将代码转移到不同的操作系统上，应该传入EGL_DEFAULT_DISPLAY，返回与默认原生窗口的连接。
    // 如果连接不可用，则返回EGL_NO_DISPLAY
    EGLDisplay eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (eglDisplay == EGL_NO_DISPLAY) {
        LOGE("eglGetDisplay failed%d", eglGetError());
        return;
    }

    //3.初始化Display
    //创建与本地原生窗口的连接后需要初始化EGL，使用函数eglInitialize进行初始化操作。如果 EGL 不能初始化，
    // 它将返回EGL_FALSE，并将EGL错误码设置为EGL_BAD_DISPLAY表示制定了不合法的EGLDisplay，
    // 或者EGL_NOT_INITIALIZED表示EGL不能初始化。使用函数eglGetError用来获取最近一次调用EGL函数出错的错误代码

    //参数
    //EGLDisplay display 创建的EGL连接
    //EGLint *majorVersion 返回EGL主板版本号
    //EGLint *minorVersion 返回EGL次版本号
    if (EGL_TRUE != eglInitialize(eglDisplay, 0, 0)) {
        LOGE("eglInitialize failed");
        return;
    }

    //3. surface窗口配置
    //窗口配置有两种方式:一种方式是使用eglGetConfigs函数获取底层窗口系统支持的所有EGL表面配置(Config)，然后再使用
    // eglGetConfigAttrib依次查询每个EGLConfig相关的信息,Config有众多的Attribute，这些Attribute决定FrameBuffer
    // 的格式和能力，通过eglGetConfigAttrib ()来读取，但不能修改。EGLConfig包含了渲染表面的所有信息，包括可用颜色、
    // 缓冲区等其他特性。
    // 另一种方式是指定我们需要的渲染表面配置，让EGL自己选择一个符合条件的EGLConfig配置。eglChooseChofig
    // 调用成功返回EGL_TRUE，失败时返回EGL_FALSE，如果attribList包含了未定义的EGL属性，或者属性值不合法，
    // EGL代码被设置为EGL_BAD_ATTRIBUTR

    //Config实际就是FrameBuffer的参数
    EGLConfig eglConfig;

    //attribList参数在EGL函数中可以为null或者指向一组以EGL_NONE结尾的键对值
    //通常以id,value依次存放，对于个别标识性的属性可以只有id，没有value
    EGLint configAttr[] = {
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_SURFACE_TYPE,
            EGL_WINDOW_BIT,
            EGL_NONE
    };
    EGLint eglConfigNum;

    //参数
    //EGLDisplay dpy 创建的和本地窗口系统的连接
    //const EGLint *attrib_list, 指定渲染表面的参数列表，可以为null
    //EGLConfig *configs  调用成功，返会符合条件的EGLConfig列表
    //EGLint config_size, 最多返回的符合条件的EGLConfig个数
    //EGLint *num_config 实际返回的符合条件的EGLConfig个数
    if (EGL_TRUE != eglChooseConfig(eglDisplay, configAttr, &eglConfig, 1, &eglConfigNum)) {
        LOGE("eglChooseConfig failed");
        return;
    }

    //4. 创建surface

    // 有了符合条件的EGLConfig后，就可以通过eglCreateWindowSurface函数创建渲染表面。使用这个函数的前提是要使
    // 用原生窗口系统提供的API创建一个窗口。eglCreateWindowSurface中attribList一般可以使用null即可。
    // 函数调用失败会返回EGL_NO_SURFACE，并设置对应的错误码

    //使用eglCreateWindowSurface函数创建在窗口上的渲染表面，此外还可以使用eglCreatePbufferSurface创建
    // 屏幕外渲染表面（Pixel Buffer 像素缓冲区）。使用Pbuffer一般用于生成纹理贴图，不过该功能已经被
    // FrameBuffer替代了，使用帧缓冲对象的好处是所有的操作都由OpenGL ES来控制。使用Pbuffer的方法和前面创建
    // 窗口渲染表面一样，需要改动的地方是在选取EGLConfig时，增加EGL_SURFACE_TYPE参数使其值包含
    // EGL_PBUFFER_BIT。而该参数默认值为EGL_WINDOW_BIT。
    //EGLSurface eglCreatePbufferSurface( EGLDisplay display, EGLConfig config, EGLint const * attrib_list // 指定像素缓冲区属性列表 );

    //Surface实际上就是一个FrameBuffer
    EGLSurface eglSurface = eglCreateWindowSurface(eglDisplay, eglConfig, aNativeWindow, 0);
    if (eglSurface == EGL_NO_SURFACE) {
        LOGE("eglCreateWindowSurface failed");
        return;
    }

    //4 创建关联的上下文

    //使用eglCreateContext为当前的渲染API创建EGL渲染上下文，返回一个上下文，当前的渲染API是由函数eglBindAPI
    // 设置的。OpenGL ES是一个状态机，用一系列变量描述OpenGL ES当前的状态如何运行，我们通常使用如下途径去更改
    // OpenGL状态：设置选项，操作缓冲。最后，我们使用当前OpenGL上下文来渲染。比如我想告诉OpenGL ES接下来要绘制
    // 三角形，可以通过一些上下文变量来改变OpenGL ES的状态，一旦改变了OpenGL ES的状态为绘制三角形，下一个命令
    // 就会画出三角形。通过这些状态设置函数就会改变上下文，接下来的操作总会根据当前上下文的状态来执行，除非再次
    // 重新改变状态。

    const EGLint ctxAttr[] = {
            EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
    };
    EGLContext eglContext = eglCreateContext(eglDisplay, eglConfig, EGL_NO_CONTEXT, ctxAttr);
    if (eglContext == EGL_NO_CONTEXT) {
        LOGE("eglCreateContext failed!");
        return;
    }

    //5.指定某个EGLContext为当前上下文。使用eglMakeCurrent函数进行当前上下文的绑定。一个程序可能创建多个EGLContext，
    // 所以需要关联特定的EGLContext和渲染表面，一般情况下两个EGLSurface参数设置成一样的。
    if (EGL_TRUE != eglMakeCurrent(eglDisplay, eglSurface, eglSurface, eglContext)) {
        LOGE("eglMakeCurrent failed!");
        return;
    }

    LOGI("EGL Init Success!");

    /**************************EGL初始化********************************************/

    /**************************shader初始化********************************************/
    //定点shader初始化
    GLint vshader = initShader(vertexShader, GL_VERTEX_SHADER);

    //片元yuv420 shader初始化
    GLint fshader = initShader(fragYUV420P, GL_FRAGMENT_SHADER);
    /**************************shader初始化********************************************/

    /**************************渲染程序初始化********************************************/
    //7.使用OpenGL相关的API进行绘制操作。.....

    //创建渲染程序
    GLint program = glCreateProgram();
    if (program == 0) {
        LOGE("glCreateProgram failed!");
        return;
    }
    //渲染程序中加入着色器代码
    glAttachShader(program, vshader);
    glAttachShader(program, fshader);

    //链接程序
    glLinkProgram(program);
    GLint status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        LOGE("glLinkProgram failed!");
        return;
    }
    glUseProgram(program);
    LOGI("glLinkProgram success!");
    /////////////////////////////////////////////////////////////


    //加入三维顶点数据 两个三角形组成正方形
    //顶点坐标系描述了GopenGL的绘制范围，他以绘制中心为原点，在2D图形下，左边界为到x -1，右边界到x 1，上边界到y 1
    //下边界到y -1,3D下同样道理。定点坐标系就是OpenGL的绘制区间
    static float vers[] = {
            1.0f, -1.0f, 0.0f,  //右下
            -1.0f, -1.0f, 0.0f, //左下
            1.0f, 1.0f, 0.0f,   //右上
            -1.0f, 1.0f, 0.0f,  //左上
    };
    GLuint apos = (GLuint) glGetAttribLocation(program, "aPosition");
    glEnableVertexAttribArray(apos);
    //传递顶点 取3个数据,跳转12个字节位(3个数据)再取另外3个数据，这是实现块状数据存储的关键，很多函数里都有这个参数，通常写作int stride
    glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 12, vers);

    //加入纹理坐标数据
    //纹理坐标的坐标系以纹理左下角为坐标原点，向右为x正轴方向，向上为y轴正轴方向。他的总长度是1。即纹理图片的
    // 四个角的坐标分别是：(0,0)、(1,0)、(0,1)、(1,1)，分别对应左下、右下、左上、右上四个顶点。
    static float txts[] = {
            1.0f, 0.0f, //右下
            0.0f, 0.0f, //左下
            1.0f, 1.0f, //右上
            0.0, 1.0    //左上
    };
    GLuint atex = (GLuint) glGetAttribLocation(program, "aTexCoord");
    glEnableVertexAttribArray(atex);
    glVertexAttribPointer(atex, 2, GL_FLOAT, GL_FALSE, 8, txts);

    LOGI("glVertexAttribPointer success");
    /**************************渲染程序传递数据********************************************/

    /**************************纹理设置********************************************/
    int width = 176;
    int height = 144;

    width = 352;
    height = 288;
    //材质纹理初始化
    //设置纹理层

    //对于纹理第1层
    glUniform1i(glGetUniformLocation(program, "yTexture"), 0);
    //对于纹理第2层
    glUniform1i(glGetUniformLocation(program, "uTexture"), 1);
    //对于纹理第3层
    glUniform1i(glGetUniformLocation(program, "vTexture"), 2);

    //创建opengl纹理
    GLuint texts[3] = {0};
    //创建三个纹理对象
    //在纹理资源使用完毕后(一般是程序退出或场景转换时)，一定要删除纹理对象，释放资源。
    //glDeleteTextures(Count:Integer;TexObj:Pointer);
    glGenTextures(3, texts);



    //使用glBindTexture将创建的纹理绑定到当前纹理。这样所有的纹理函数都将针对当前纹理。
    glBindTexture(GL_TEXTURE_2D, texts[0]);
    //设置缩小滤镜
    /**
     * 第一个参数表明是针对何种纹理进行设置
     * 第二个参数表示要设置放大滤镜还是缩小滤镜
     *
     * 在纹理映射的过程中，如果图元的大小不等于纹理的大小，OpenGL便会对纹理进行缩放以适应图元的尺寸。
     * 我们可以通过设置纹理滤镜来决定OpenGL对某个纹理采用的放大、缩小的算法。
     *
     * 第三个参数表示使用的滤镜
     *
     * 第三个参数可选项如下：
     *
     * GL_NEAREST 	取最邻近像素
     * GL_LINEAR 	线性内部插值
     * GL_NEAREST_MIPMAP_NEAREST 	最近多贴图等级的最邻近像素
     * GL_NEAREST_MIPMAP_LINEAR 	在最近多贴图等级的内部线性插值
     * GL_LINEAR_MIPMAP_NEAREST 	在最近多贴图等级的外部线性插值
     * GL_LINEAR_MIPMAP_LINEAR 	在最近多贴图等级的外部和内部线性插值
     *
     */

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //设置放大滤镜
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexImage2D函数将Pixels数组中的像素值传给当前绑定的纹理对象，于是便创建了纹理，Pixels是最后一个参数
    glTexImage2D(
            //纹理的类型
            GL_TEXTURE_2D,
            //纹理的等级 0默认 级的分辨率最大
            0,
            //gpu内部格式 亮度，灰度图
            GL_LUMINANCE,
            //纹理图像的宽度和高度 拉升到全屏
            width, height,
            //边框大小
            0,
            //像素数据的格式 亮度，灰度图 要与上面一致
            GL_LUMINANCE,
            //像素值的数据类型
            GL_UNSIGNED_BYTE,
            //纹理的数据(像素数据)
            NULL
    );

    //使用glBindTexture将创建的纹理绑定到当前纹理。这样所有的纹理函数都将针对当前纹理。
    glBindTexture(GL_TEXTURE_2D, texts[1]);
    //调用glTexParameter来设置纹理滤镜
    //设置缩小滤镜
    /**
     * 第一个参数表明是针对何种纹理进行设置
     * 第二个参数表示要设置放大滤镜还是缩小滤镜
     * 第三个参数表示使用的滤镜
     *
     * 第三个参数可选项如下：
     *
     * GL_NEAREST 	取最邻近像素
     * GL_LINEAR 	线性内部插值
     * GL_NEAREST_MIPMAP_NEAREST 	最近多贴图等级的最邻近像素
     * GL_NEAREST_MIPMAP_LINEAR 	在最近多贴图等级的内部线性插值
     * GL_LINEAR_MIPMAP_NEAREST 	在最近多贴图等级的外部线性插值
     * GL_LINEAR_MIPMAP_LINEAR 	在最近多贴图等级的外部和内部线性插值
     *
     * 多贴图纹理(Mip Mapping)为一个纹理对象生成不同尺寸的图像。在需要时，根据绘制图形的大小来决定采用的纹理
     * 等级或者在不同的纹理等级之间进行线性内插。使用多贴图纹理的好处在于消除纹理躁动。这种情况在所绘制的景物
     * 离观察者较远时常常发生(如图6.6-1和6.6-2)。由于多贴图纹理现在的渲染速度已经很快，以至于和普通纹理没有
     * 什么区别，我们现在一般都使用多贴图纹理。
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    //设置放大滤镜
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexImage2D函数将Pixels数组中的像素值传给当前绑定的纹理对象，于是便创建了纹理，Pixels是最后一个参数
    glTexImage2D(
            //纹理的类型
            GL_TEXTURE_2D,
            //纹理的等级 0默认 级的分辨率最大
            0,
            //gpu内部格式 亮度，灰度图
            GL_LUMINANCE,
            //纹理图像的宽度和高度 拉升到全屏
            width, height,
            //边框大小
            0,
            //像素数据的格式 亮度，灰度图 要与上面一致
            GL_LUMINANCE,
            //像素值的数据类型
            GL_UNSIGNED_BYTE,
            //纹理的数据(像素数据)
            NULL
    );

    //使用glBindTexture将创建的纹理绑定到当前纹理。这样所有的纹理函数都将针对当前纹理。
    glBindTexture(GL_TEXTURE_2D, texts[2]);
    //缩小的过滤器
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    //glTexImage2D函数将Pixels数组中的像素值传给当前绑定的纹理对象，于是便创建了纹理，Pixels是最后一个参数
    glTexImage2D(
            //纹理的类型
            GL_TEXTURE_2D,
            //纹理的等级 0默认 级的分辨率最大
            0,
            //gpu内部格式 亮度，灰度图
            GL_LUMINANCE,
            //纹理图像的宽度和高度 拉升到全屏
            width, height,
            //边框大小
            0,
            //像素数据的格式 亮度，灰度图 要与上面一致
            GL_LUMINANCE,
            //像素值的数据类型
            GL_UNSIGNED_BYTE,
            //纹理的数据(像素数据)
            NULL
    );
    LOGI("glTexImage2D success");


    /**************************纹理设置********************************************/
    unsigned char *buf[3] = {0};
    buf[0] = new unsigned char[width * height];
    buf[1] = new unsigned char[width * height / 4];
    buf[2] = new unsigned char[width * height / 4];

    for (int i = 0; i < 10000; i++) {
        //memset(buf[0],i,width*height);
        // memset(buf[1],i,width*height/4);
        //memset(buf[2],i,width*height/4);

        //420p   yyyyyyyy uu vv
        if (feof(fp) == 0) {
            //yyyyyyyy
            fread(buf[0], 1, width * height, fp);
            fread(buf[1], 1, width * height / 4, fp);
            fread(buf[2], 1, width * height / 4, fp);
        }




        //激活第1层纹理,绑定到创建的opengl纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texts[0]);
        //替换纹理内容
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                        buf[0]);


        //激活第2层纹理,绑定到创建的opengl纹理
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, texts[1]);
        //替换纹理内容
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
                        GL_UNSIGNED_BYTE, buf[1]);

        //激活第2层纹理,绑定到创建的opengl纹理
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, texts[2]);
        //替换纹理内容
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width / 2, height / 2, GL_LUMINANCE,
                        GL_UNSIGNED_BYTE, buf[2]);



        //三维绘制
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        //窗口显示
        eglSwapBuffers(eglDisplay, eglSurface);

    }
    /**************************纹理显示********************************************/
    LOGI("eglSwapBuffers success");
    env->ReleaseStringUTFChars(url_, url);
}
