//
// Created by renzhenming on 2018/10/24.
//

#include "GLVideoView.h"
#include "XTexture.h"
#include "XLog.h"


/**
重复打开报错：
E/libOpenSLES: frameworks/wilhelm/src/itf/IPlay.c:38: pthread_mutex_lock_timeout_np returned 110
E/XPlay: IDecode ::Main 从解码器种读取解码后的数据失败  frame.size=
E/libEGL: eglCreateWindowSurface: native_window_api_connect (win=0xcbd7e008) failed (0xffffffed) (already connected to another API?)
eglCreateWindowSurface:481 error 3003 (EGL_BAD_ALLOC)
E/XPlay: eglCreateWindowSurface failed!
XTexture init -- > EGL init failed
call to OpenGL ES API with no current context (logged once per thread)

原因分析
再次打开视频的时候我们是从另一个页面退回到视频播放页面，此时surfaceCreated方法会再次回调，传递surface过来，但是要知道，传递
surface和打开视频播放是两个线程进行的，当第二次点击播放按钮的时候，会立即重新调用Java_com_rzm_ffmpegplayer_OpenUrl_Open
播放视频，但此时，surfaceCreated方法未必已经在打开之前传递了surface，导致，第二次播放视频使用了第一次传递的surface，大概原因就是这样

所以这就需要对传递的surface（window）进行清理，但是这个清理不能放在Close方法中，原因是，surface的传递和视频界面的绘制这两个线程
执行完成的顺序有两种，要么先传递了surface，此时播放正常，不会有问题，但是另一种情况就是先播放，后传递了surface，导致上边的问题发生
如果我们在close的时候清理window,那么会纯在这样一种情况，正确的surface已经传递过来了，但是此时被close清理掉了，也会发生问题，所以
window在close中清理
*/
void GLVideoView::SetRender(void *window) {
XLOGI("GLVideoView::SetRender");
    view = window;
}

void GLVideoView::Close() {
    mutex.lock();
    if(texture){
        texture->Drop();
        texture = 0;
    }
    mutex.unlock();
}

void GLVideoView::Render(XData data) {
    if (!view) return;
    if (!texture) {
        texture = XTexture::Create();
        XLOGI("GLVideoView::Render");
        texture->Init(view, (XTextureType) data.format);
    }
    //XLOGI("GLVideoView begin draw data.width=%d data.height=%d",data.width,data.height);
    texture->Draw(data.datas, data.width, data.height);
}