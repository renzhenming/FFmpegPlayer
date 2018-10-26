#include "XData.h"

extern "C"{
    #include "libavformat/avformat.h"
}

void XData::Drop(){
    if(!data) return;
    av_packet_free((AVPacket **)&data);
    data = 0;
    size = 0;
}

bool XData::Alloc(int size, const char *d) {
    Drop();
    type = UCHAR_TYPE;
    if(size <= 0){
        return false;
    }
    this->data = new unsigned char[size];
    if(!this->data) return false;
    if(d){
        memcpy(this->data,d,size);
    }
    return true;
}