#ifndef FFMPEG_XDATA_H
#define FFMPEG_XDATA_H

struct XData {
    unsigned char *data = 0;
    unsigned char *datas[8] = {0};
    int size = 0;
    bool isAudio = false;
    bool isVideo = false;
    int width = 0;
    int height = 0;
    void Drop();
};

#endif