//
// Created by renzhenming on 2018/10/24.
//

#include "GLVideoView.h"
#include "XTexture.h"
#include "XLog.h"

void GLVideoView::SetRender(void *window) {
    view = window;
}

void GLVideoView::Render(XData data) {
    if (!view) return;
    if (!texture) {
        texture = XTexture::Create();
        texture->Init(view, (XTextureType) data.format);
    }
    //XLOGI("GLVideoView begin draw data.width=%d data.height=%d",data.width,data.height);
    texture->Draw(data.datas, data.width, data.height);
}