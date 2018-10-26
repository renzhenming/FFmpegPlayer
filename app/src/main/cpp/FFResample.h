//
// Created by renzhenming on 2018/10/26.
//

#ifndef FFMPEGPLAYER_FFRESAMPLE_H
#define FFMPEGPLAYER_FFRESAMPLE_H

#include "IResample.h"

struct SwrContext;

class FFResample: public IResample{
public:
    virtual bool Open(XParameter in,XParameter out = XParameter());
    virtual XData Resample(XData inData);

protected:
    SwrContext *swrContext;
};

#endif //FFMPEGPLAYER_FFRESAMPLE_H
