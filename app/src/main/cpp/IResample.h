//
// Created by renzhenming on 2018/10/26.
//

#ifndef FFMPEGPLAYER_IRESAMPLE_H
#define FFMPEGPLAYER_IRESAMPLE_H

#include "XParameter.h"
#include "XData.h"
#include "IObserver.h"

class IResample : public IObserver {
public:
    virtual bool Open(XParameter in, XParameter out = XParameter()) = 0;

    virtual XData Resample(XData inData) = 0;

    virtual void Update(XData data);

    int outChannels = 2;
    int outFormat = 1;
};

#endif //FFMPEGPLAYER_IRESAMPLE_H
