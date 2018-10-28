#ifndef FFMPEG_XDATA_H
#define FFMPEG_XDATA_H

//对音视频数据做一个区分，视频数据解封装得到的时AVPackt，清理的时候可以使用ffmpeg的方法
//av_packet_free清理，但是音频不行，
enum XDataType {
    AVPACKET_TYPE = 0,
    UCHAR_TYPE = 1
};

struct XData {
    int type = AVPACKET_TYPE;
    unsigned char *data = 0;
    unsigned char *datas[8] = {0};
    int size = 0;
    bool isAudio = false;
    bool isVideo = false;
    int width = 0;
    int height = 0;
    void Drop();
    bool Alloc(int size,const char *data=0);
};

#endif