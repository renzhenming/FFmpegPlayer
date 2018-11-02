//
// Created by renzhenming on 2018/10/24.
//

#include "XTexture.h"
#include "XShader.h"
#include "XLog.h"
#include "XEGL.h"

class CXTexture : public XTexture {
public:
    XShader shader;
    XTextureType type;

    virtual void Drop(){
        mutex.lock();
        XEGL::Get()->Close();
        shader.Close();
        mutex.unlock();
        delete this;
    }

    virtual bool Init(void *window, XTextureType type) {

        mutex.lock();

        XEGL::Get()->Close();
        shader.Close();

        if (!window) {
            mutex.unlock();
            XLOGE("XTexture init failed,window is NULL");
            return false;
        }

        if (!XEGL::Get()->Init(window)) {
            mutex.unlock();
            XLOGE("XTexture init -- > EGL init failed");
            return false;
        }
        XLOGI("EGL init success");
        this->type = type;
        if (!shader.Init((XShaderType) type)) {
            mutex.unlock();
            XLOGE("XTexture init -- > shader init failed");
            return false;
        }
        XLOGI("XTexture init -- > shader init success");
        mutex.unlock();
        return true;
    }

    virtual void Draw(unsigned char *data[], int width, int height) {
        mutex.lock();
        shader.GetTexture(0, width, height, data[0]); //Y
        if (type == XTEXTURE_YUV420P) {
            shader.GetTexture(1, width / 2, height / 2, data[1]);  //U
            shader.GetTexture(2, width / 2, height / 2, data[2]);  //V
        } else {
            shader.GetTexture(1, width / 2, height / 2, data[1], true);  //UV
        }
        shader.Draw();
        XEGL::Get()->Draw();
        mutex.unlock();
    }

};


XTexture *XTexture::Create() {
    return new CXTexture();
}