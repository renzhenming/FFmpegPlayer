#ifndef FFMPEG_XLOG_H
#define FFMPEG_XLOG_H

#ifdef ANDROID
#include <android/log.h>
#define XLOGD(...) __android_log_print(ANDROID_LOG_DEBUG,"XPlay",__VA_ARGS__)
#define XLOGI(...) __android_log_print(ANDROID_LOG_INFO,"XPlay",__VA_ARGS__)
#define XLOGE(...) __android_log_print(ANDROID_LOG_ERROR,"XPlay",__VA_ARGS__)
#else
#define XLOGD(...) printf("XPlay",__VA_ARGS__)
#define XLOGI(...) printf("XPlay",__VA_ARGS__)
#define XLOGE(...) printf("XPlay",__VA_ARGS__)

#endif
//类后要加;error: expected ';' after class
class XLog{

};

#endif