//
// Created by renzhenming on 2018/10/24.
//
#include "XShader.h"
#include <GLES2/gl2.h>
#include "XLog.h"

#define GET_STR(x) #x

//顶点着色器glsl
static const char *vertexShader = GET_STR(
    //顶点坐标
    attribute vec4 aPosition;
    //材质顶点坐标
    attribute vec2 aTexCoord;
    //输出的材质坐标
    varying vec2 vTexCoord;

    void main(){
        vTexCoord = vec2(aTexCoord.x,1.0-aTexCoord.y);
        gl_Position = aPosition;
    }
);

//片元着色器,软解码和部分x86硬解码
static const char *fragYUV420P = GET_STR(
    //精度
    precision mediump float;
    //顶点着色器传递的坐标
    varying vec2 vTexCoord;
    //输入的材质
    uniform sampler2D yTexture;
    uniform sampler2D uTexture;
    uniform sampler2D vTexture;

    void main(){
        vec3 yuv;
        vec3 rgb;
        yuv.r = texture2D(yTexture,vTexCoord).r;
        yuv.g = texture2D(uTexture,vTexCoord).r - 0.5;
        yuv.b = texture2D(vTexture,vTexCoord).r-0.5;

        rgb = mat3(
            1.0,   1.0,   1.0,
            0.0,-0.39465,2.03211,
            1.13983,-0.58060,0.0
        )*yuv;

        //输出像素颜色
        gl_FragColor = vec4(rgb,1.0);
    }

);


static GLuint InitShader(const char *code,GLint type){
    //创建shader
    GLuint shader = glCreateShader(type);
    if(shader == 0){
        XLOGI("glCreateShader %d failed!",type);
        return 0;
    }

    //加载shader 1 shader数量 code shader代码 0代码长度
    glShaderSource(shader,1,&code,0);

    //编译shader
    glCompileShader(shader);

    //获取编译情况
    GLint status;
    glGetShaderiv(shader,GL_COMPILE_STATUS,&status);
    if(status == 0){
        XLOGI("glCompileShader failed!");
        return 0;
    }
    XLOGI("glCompileShader success!");
    return shader;
}

bool XShader::Init(){
    //初始化顶点着色器
    vsh = InitShader(vertexShader,GL_VERTEX_SHADER);
    if(vertexShader == 0){
        XLOGE("init GL_VERTEX_SHADER failed!");
        return false;
    }
    XLOGI("init GL_VERTEX_SHADER success!");

    //初始化材质着色器yuv420
    fsh = InitShader(fragYUV420P,GL_FRAGMENT_SHADER);
    if(vertexShader == 0){
        XLOGE("init GL_FRAGMENT_SHADER failed!");
        return false;
    }
    XLOGI("init GL_FRAGMENT_SHADER success!");
}














