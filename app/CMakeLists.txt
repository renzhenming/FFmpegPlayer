
cmake_minimum_required(VERSION 3.4.1)

#设置头文件路径
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/include)

#设置FFmpeg库路径变量
#CMAKE_CURRENT_SOURCE_DIR,指的是当前处理的 CMakeLists.txt 所在的路径，CMAKE_SOURCE_DIR，不论采用何种编译方式,都是工程顶层目录

set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI})

#添加avcodec
add_library(avcodec SHARED IMPORTED)
set_target_properties(avcodec PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavcodec.so)

#添加avfilter
add_library(avfilter SHARED IMPORTED)
set_target_properties(avfilter PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavfilter.so)

#添加avformat
add_library(avformat SHARED IMPORTED)
set_target_properties(avformat PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavformat.so)

#添加avutil
add_library(avutil SHARED IMPORTED)
set_target_properties(avutil PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libavutil.so)

#添加swresample
add_library(swresample SHARED IMPORTED)
set_target_properties(swresample PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libswresample.so)

#添加swscale
add_library(swscale SHARED IMPORTED)
set_target_properties(swscale PROPERTIES IMPORTED_LOCATION ${FFMPEG_DIR}/libswscale.so)

add_library( ffmpeg
             SHARED
             ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/XThread.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/ffmpeg_player.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/IDemux.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/FFDemux.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/XData.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/XLog.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/IObserver.cpp
             ${CMAKE_CURRENT_SOURCE_DIR}/src/main/cpp/FFDecode.cpp

)

find_library( log-lib log )

target_link_libraries( ffmpeg
                       #播放音频
                       OpenSLES
                       #视频shader播放
                       EGL
                       GLESv2
                       android
                       avcodec
                       avfilter
                       avformat
                       avutil
                       swresample
                       swscale
                       ${log-lib} )