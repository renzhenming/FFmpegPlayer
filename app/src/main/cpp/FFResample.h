//
// Created by renzhenming on 2018/10/26.
//

#ifndef FFMPEGPLAYER_FFRESAMPLE_H
#define FFMPEGPLAYER_FFRESAMPLE_H

#include "IResample.h"

struct SwrContext;

class FFResample : public IResample {
public:
    virtual bool Open(XParameter in, XParameter out = XParameter());

    virtual void Close();

    virtual XData Resample(XData inData);

protected:
    SwrContext *swrContext = 0;
    std::mutex mutex;
};

#endif //FFMPEGPLAYER_FFRESAMPLE_H
