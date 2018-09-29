#include <jni.h>
#include "libavcodec/avcodec.h"
extern "C"
JNIEXPORT jstring JNICALL
Java_com_rzm_ffmpegplayer_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    const char * configure = avcodec_configuration();
    return env->NewStringUTF(configure);
}
