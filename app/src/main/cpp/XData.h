#ifndef FFMPEG_XDATA_H
#define FFMPEG_XDATA_H

struct XData {
    unsigned char *data = 0;
    int size = 0;
    bool isAudio = false;
    bool isVideo = false;
    void Drop();
};

#endif