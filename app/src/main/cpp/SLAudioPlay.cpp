#include "SLAudioPlay.h"
#include "XLog.h"
#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"

SLObjectItf engineObject = NULL;
SLEngineItf engineInterface = NULL;
SLObjectItf mixture = NULL;
SLObjectItf playerItf = NULL;
SLPlayItf playerInterface = NULL;
SLAndroidSimpleBufferQueueItf pcmQueue = NULL;

SLAudioPlay::SLAudioPlay() {
    buf = new unsigned char[1024 * 1024];
}

SLAudioPlay::~SLAudioPlay() {
    delete buf;
    buf = 0;
}

void SLAudioPlay::PlayCall(void *buf_queue) {
    if (!buf_queue) return;
    SLAndroidSimpleBufferQueueItf bf = (SLAndroidSimpleBufferQueueItf) buf_queue;
    //阻塞
    XData d = GetData();
    if (d.size <= 0) {
        XLOGE("GetData() size is 0");
        return;
    }
    if (!buf) return;
    memcpy(buf, d.data, d.size);
    mutex.lock();

    //注意这里有一个bug，当打开一个视频之后，再重复打开一次，导致程序崩溃，原因是重复打开时，音频
    //播放队列被清理，但是有可能这里还在进行enqueue,这个时候其实队列已经不在了，所以在这里做一层判断
    if(pcmQueue && (*pcmQueue))
        (*bf)->Enqueue(bf, buf, d.size);
    mutex.unlock();
    d.Drop();
}

static void PcmCall(SLAndroidSimpleBufferQueueItf bf, void *context) {
    SLAudioPlay *ap = (SLAudioPlay *) context;
    if (!ap) {
        XLOGE("SLAudioPlay PcmCall failed,context is null");
        return;
    }
    ap->PlayCall((void *) bf);
}

bool SLAudioPlay::StartPlay(XParameter out) {
    XLOGI("SLAudioPlay begin");
    Close();
    XLOGI("SLAudioPlay close finish");
    mutex.lock();
    /****************创建OpenSLES 引擎*******************/

    //Object是一个资源的抽象集合,可以通过它获取各种资源,所有的Object在OpenSL里面我们拿到的都是一个SLObjectItf
    SLresult result = slCreateEngine(&engineObject, 0, 0, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS) {
        mutex.unlock();
        XLOGE("SLAudioPlay slCreateEngine failed");
        return false;
    }
    XLOGI("SLAudioPlay slCreateEngine success");

    //初始化引擎对象
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        mutex.unlock();
        XLOGE("SLAudioPlay engineObject Realize failed");
        return false;
    }
    XLOGI("SLAudioPlay engineObject Realize success");

    //获取引擎接口
    //一个Object里面可能包含了多个Interface,所以GetInterface方法有个SLInterfaceID参数来指定到的需要获取
    //Object里面的哪个Interface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineInterface);
    if (result != SL_RESULT_SUCCESS) {
        mutex.unlock();
        XLOGE("SLAudioPlay engineObject GetInterface failed");
        return false;
    }
    XLOGI("SLAudioPlay engineObject GetInterface success");

    /****************创建混音器*******************/
    result = (*engineInterface)->CreateOutputMix(engineInterface, &mixture, 0, 0, 0);
    if (result != SL_RESULT_SUCCESS) {
        mutex.unlock();
        XLOGE("SLAudioPlay engineInterface CreateOutputMix failed");
        return false;
    }
    XLOGI("SLAudioPlay engineInterface CreateOutputMix success");

    //初始化混音器
    result = (*mixture)->Realize(mixture, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
        mutex.unlock();
        XLOGE("SLAudioPlay mixture Realize failed");
        return false;
    }
    XLOGI("SLAudioPlay mixture Realize success");

    //创建播放器
    SLDataLocator_AndroidSimpleBufferQueue queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 10};
    SLDataFormat_PCM pcm = {
            SL_DATAFORMAT_PCM,
            (SLuint32) out.channels,
            (SLuint32) out.sample_rate * 1000,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_PCMSAMPLEFORMAT_FIXED_16,
            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
            SL_BYTEORDER_LITTLEENDIAN
    };
    SLDataLocator_OutputMix outmix = {SL_DATALOCATOR_OUTPUTMIX, mixture};

    SLDataSource ds = {&queue, &pcm};
    SLDataSink audioSink = {&outmix, 0};
    const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[] = {SL_BOOLEAN_TRUE};

    result = (*engineInterface)->CreateAudioPlayer(
            engineInterface,
            &playerItf,
            &ds,
            &audioSink,
            sizeof(ids) / sizeof(SLInterfaceID),
            ids,
            req
    );
    if (result != SL_RESULT_SUCCESS) {
        mutex.unlock();
        XLOGE("SLAudioPlay engineInterface CreateAudioPlayer failed");
        return false;
    }
    XLOGI("SLAudioPlay engineInterface CreateAudioPlayer success");
    //实例化播放器
    (*playerItf)->Realize(playerItf, SL_BOOLEAN_FALSE);
    //获取播放器接口
    result = (*playerItf)->GetInterface(playerItf, SL_IID_PLAY, &playerInterface);
    if (result != SL_RESULT_SUCCESS) {
        mutex.unlock();
        XLOGE("SLAudioPlay playerItf GetInterface SL_IID_PLAY failed");
        return false;
    }
    XLOGI("SLAudioPlay playerItf GetInterface SL_IID_PLAY success");
    //获取播放队列接口
    result = (*playerItf)->GetInterface(playerItf, SL_IID_BUFFERQUEUE, &pcmQueue);
    if (result != SL_RESULT_SUCCESS) {
        mutex.unlock();
        XLOGE("SLAudioPlay playerItf GetInterface SL_IID_BUFFERQUEUE failed");
        return false;
    }
    XLOGI("SLAudioPlay playerItf GetInterface SL_IID_BUFFERQUEUE success");
    //设置回调函数
    (*pcmQueue)->RegisterCallback(pcmQueue, PcmCall, this);

    //设置为播放状态
    (*playerInterface)->SetPlayState(playerInterface, SL_PLAYSTATE_PLAYING);

    //启动队列
    (*pcmQueue)->Enqueue(pcmQueue, "", 1);
    XLOGI("SLAudioPlay play success");
    mutex.unlock();
    return true;
}

void SLAudioPlay::Close(){
    IAudioPlay::Clear();
    XLOGI("SLAudioPlay 开始关闭");
    mutex.lock();

    //停止播放
    if(playerInterface && (*playerInterface)){
        (*playerInterface)->SetPlayState(playerInterface,SL_PLAYSTATE_STOPPED);
    }
    XLOGI("SLAudioPlay 停止播放");
    //清理播放队列
    if(pcmQueue && (*pcmQueue)){
        (*pcmQueue)->Clear(pcmQueue);
    }
    XLOGI("SLAudioPlay 清理播放队列");
    //销毁player对象
    if(playerItf &&(*playerItf)){
        (*playerItf)->Destroy(playerItf);
    }
    XLOGI("SLAudioPlay 销毁player对象");
    //销毁混音器
    if(mixture && (*mixture)){
        (*mixture)->Destroy(mixture);
    }
    XLOGI("SLAudioPlay 销毁混音器");
    //销毁播放引擎
    if(engineObject && (*engineObject)){
        (*engineObject)->Destroy(engineObject);
    }
    XLOGI("SLAudioPlay 销毁播放引擎 finish");

    engineObject = NULL;
    engineInterface = NULL;
    mixture = NULL;
    playerItf = NULL;
    playerInterface = NULL;
    pcmQueue = NULL;

    mutex.unlock();
}

























