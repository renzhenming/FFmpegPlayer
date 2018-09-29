#include <jni.h>

//千万记住，ffmpeg是C语言编写的，在C++中使用必须开启开启混合编译，不然会一直报错，
//undefined reference to 'xxxx'
extern "C"{
#include "libavcodec/avcodec.h"
};
extern "C"
JNIEXPORT jstring JNICALL
Java_com_rzm_ffmpegplayer_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    const char * configure = avcodec_configuration();
    return env->NewStringUTF(configure);
}
