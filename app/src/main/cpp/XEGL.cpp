//
// Created by renzhenming on 2018/10/24.
//

#include <android/native_window.h>
#include <EGL/egl.h>
#include "XEGL.h"
#include "XLog.h"

class CXEGL:public XEGL{
public:

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;

    virtual bool Init(void *win){
        ANativeWindow *window = (ANativeWindow *)win;

        //初始化EGL

        //获取Display eglGetDisplay
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if(display == EGL_NO_DISPLAY){
            XLOGE("eglGetDisplay failed");
            return false;
        }
        XLOGI("eglGetDisplay success");

        //初始化Display eglInitialize
        if(EGL_TRUE != eglInitialize(display,0,0)){
            XLOGE("eglInitialize failed");
            return false;
        }
        XLOGI("eglInitialize success");

        //选择Config eglChooseConfig
        EGLint configAttrList[]{
                EGL_RED_SIZE,8,
                EGL_GREEN_SIZE,8,
                EGL_BLUE_SIZE,8,
                EGL_SURFACE_TYPE,
                EGL_WINDOW_BIT,
                EGL_NONE
        };
        EGLConfig config = 0;
        EGLint numConfigs = 0;
        if (EGL_TRUE != eglChooseConfig(display,configAttrList,&config,1,&numConfigs)){
            XLOGE("eglChooseConfig false");
            return false;
        }
        XLOGI("eglChooseConfig success");

        //创建Surface eglCreateWindowSurface /*EGL_NONE: Attrib list terminator */
        surface = eglCreateWindowSurface(display,config,window,NULL);
        if(surface == EGL_NO_SURFACE) {
            XLOGE("eglCreateWindowSurface failed!");
            return false;
        }
        XLOGI("eglCreateWindowSurface success!");


        //创建Context eglCreateContext
        const EGLint contextAttrList[] ={EGL_CONTEXT_CLIENT_VERSION,2,EGL_NONE};
        context = eglCreateContext(display,config,EGL_NO_CONTEXT,contextAttrList);
        if(context == EGL_NO_CONTEXT) {
            XLOGE("eglCreateContext failed!");
            return false;
        }
        XLOGI("eglCreateContext success!");

        //绑定Context eglMakeCurrent
        if(EGL_TRUE != eglMakeCurrent(display,surface,surface,context)){
            XLOGE("eglMakeCurrent failed!");
            return false;
        }
        XLOGI("eglMakeCurrent success!");
        return true;
    }

    virtual void Draw() {
        if(display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE)
        {
            return;
        }
        eglSwapBuffers(display,surface);
    }
};

//创建对象的单利模式
XEGL* XEGL::Get() {
    static CXEGL egl;
    return &egl;
}