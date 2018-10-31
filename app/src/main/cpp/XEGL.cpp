//
// Created by renzhenming on 2018/10/24.
//

#include <android/native_window.h>
#include <EGL/egl.h>
#include "XEGL.h"
#include "XLog.h"

/**
 * EGL是Khronos组织创造的渲染API（OpenGL ES）和操作系统窗口之间的接口。当提供OpenGL ES时，不是必需的提供EGL，
 * 开发者可以根据平台厂商公布的文档决定使 用何种接口.EGL API也有另外的功能，例如电力管理，支持多种渲染上下文，
 * 分享贴图或者缓冲区等等，通过OpenGL ES扩展行为能够通过工具查询获得。
 *
 * EGL命名语法:
 * 所有的EGL函数以egl前缀开始（例如eglCreateWindowSurface），EGL数据类型以EGL前缀开始例如EGLint和EGLenum。
 *
 * 同样OpenGL ES数据类型也以前缀GL开始
 */
class CXEGL : public XEGL {
public:

    EGLDisplay display = EGL_NO_DISPLAY;
    EGLContext context = EGL_NO_CONTEXT;
    EGLSurface surface = EGL_NO_SURFACE;

    virtual bool Init(void *win) {
        ANativeWindow *window = (ANativeWindow *) win;

        //初始化EGL

        //获取Display eglGetDisplay

        /**
         * OpenGL ES命令出错会产生一个错误码，错误码被记录，能够使用glGetError函数查询，在第一个被记录的错误码
         * 被查询前，不会记录新的错误码。一旦错误码被查询，当前错误码将变成GL_NO_ERROR。除了GL_OUT_OF_MEMORY
         * 错误码以外，其它的错误码将被忽略，不影响程序运行状态。
         */
        display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display == EGL_NO_DISPLAY) {
            XLOGE("eglGetDisplay failed%d", eglGetError());
            return false;
        }
        XLOGI("eglGetDisplay success");

        //初始化Display eglInitialize
        if (EGL_TRUE != eglInitialize(display, 0, 0)) {
            XLOGE("eglInitialize failed");
            return false;
        }
        XLOGI("eglInitialize success");

        //选择Config eglChooseConfig
        EGLint configAttrList[]{
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_SURFACE_TYPE,
                EGL_WINDOW_BIT,
                EGL_NONE
        };
        EGLConfig config = 0;
        EGLint numConfigs = 0;
        if (EGL_TRUE != eglChooseConfig(display, configAttrList, &config, 1, &numConfigs)) {
            XLOGE("eglChooseConfig false");
            return false;
        }
        XLOGI("eglChooseConfig success");

        //创建Surface eglCreateWindowSurface /*EGL_NONE: Attrib list terminator */
        surface = eglCreateWindowSurface(display, config, window, NULL);
        if (surface == EGL_NO_SURFACE) {
            XLOGE("eglCreateWindowSurface failed!");
            return false;
        }
        XLOGI("eglCreateWindowSurface success!");


        //创建Context eglCreateContext
        const EGLint contextAttrList[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
        context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttrList);
        if (context == EGL_NO_CONTEXT) {
            XLOGE("eglCreateContext failed!");
            return false;
        }
        XLOGI("eglCreateContext success!");

        //绑定Context eglMakeCurrent
        if (EGL_TRUE != eglMakeCurrent(display, surface, surface, context)) {
            XLOGE("eglMakeCurrent failed!");
            return false;
        }
        XLOGI("eglMakeCurrent success!");
        return true;
    }

    virtual void Draw() {
        if (display == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE) {
            return;
        }
        eglSwapBuffers(display, surface);
    }
};

//创建对象的单利模式
XEGL *XEGL::Get() {
    static CXEGL egl;
    return &egl;
}