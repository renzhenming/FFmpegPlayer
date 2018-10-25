//
// Created by renzhenming on 2018/10/24.
//

#ifndef FFMPEGPLAYER_IVIDEOVIEW_H
#define FFMPEGPLAYER_IVIDEOVIEW_H

#include "XData.h"
#include "IObserver.h"

class IVideoView : public IObserver{
public:
    virtual void SetRender(void *window) = 0;
    virtual void Render(XData data) = 0;
    virtual void Update(XData data);
};

#endif //FFMPEGPLAYER_IVIDEOVIEW_H
