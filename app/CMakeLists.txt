
cmake_minimum_required(VERSION 3.4.1)

#设置头文件路径
include_directories(src/main/cpp/include)

#设置FFmpeg库路径变量
#CMAKE_CURRENT_SOURCE_DIR,指的是当前处理的 CMakeLists.txt 所在的路径，CMAKE_SOURCE_DIR，不论采用何种编译方式,都是工程顶层目录

set(FFMPEG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/${ANDROID_ABI})


add_library( ffmpeg
             SHARED
             ./src/main/cpp/native-lib.cpp )
find_library( log-lib log )


# 编解码(最重要的库)
add_library( avcodec
             SHARED
             IMPORTED )
# 指定库的路径
set_target_properties( # Specifies the target library.
                       avcodec
                       # Specifies the parameter you want to define.
                       PROPERTIES IMPORTED_LOCATION
                       # Provides the path to the library you want to import.
                       ${FFMPEG_DIR}/libavcodec.so )

#滤镜特效处理库
add_library( avfilter
             SHARED
             IMPORTED )
# 指定库的路径
set_target_properties(
                       avfilter
                       PROPERTIES IMPORTED_LOCATION
                       ${FFMPEG_DIR}/libavfilter.so )

#封装格式处理库
add_library( avformat
             SHARED
             IMPORTED )
# 指定库的路径
set_target_properties(
                       avformat
                       PROPERTIES IMPORTED_LOCATION
                       ${FFMPEG_DIR}/libavformat.so)

#工具库(大部分库都需要这个库的支持)
add_library( avutil
             SHARED
             IMPORTED )
# 指定库的路径
set_target_properties(
                       avutil
                       PROPERTIES IMPORTED_LOCATION
                       ${FFMPEG_DIR}/libavutil.so)

#音频采样数据格式转换库
add_library( swresample
             SHARED
             IMPORTED )
# 指定库的路径
set_target_properties(
                       swresample
                       PROPERTIES IMPORTED_LOCATION
                       ${FFMPEG_DIR}/libswresample.so)

#视频像素数据格式转换
add_library( swscale
             SHARED
             IMPORTED )
# 指定库的路径
set_target_properties(
                       swscale
                       PROPERTIES IMPORTED_LOCATION
                       ${FFMPEG_DIR}/libswscale.so)

# 将预构建库与本地库相连
target_link_libraries(
                       ffmpeg avcodec avfilter avformat avutil swresample swscale
                       ${log-lib} )