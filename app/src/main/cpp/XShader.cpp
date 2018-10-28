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

//片元着色器,软解码和部分x86硬解码
static const char *fragNV12 = GET_STR(
        precision mediump float;    //精度
        varying vec2 vTexCoord;     //顶点着色器传递的坐标
        uniform sampler2D yTexture; //输入的材质（不透明灰度，单像素）
        uniform sampler2D uvTexture;
        void main(){
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(yTexture,vTexCoord).r;
            yuv.g = texture2D(uvTexture,vTexCoord).r - 0.5;
            yuv.b = texture2D(uvTexture,vTexCoord).a - 0.5;
            rgb = mat3(1.0,     1.0,    1.0,
                       0.0,-0.39465,2.03211,
                       1.13983,-0.58060,0.0)*yuv;
            //输出像素颜色
            gl_FragColor = vec4(rgb,1.0);
        }
);

//片元着色器,软解码和部分x86硬解码
static const char *fragNV21 = GET_STR(
        precision mediump float;    //精度
        varying vec2 vTexCoord;     //顶点着色器传递的坐标
        uniform sampler2D yTexture; //输入的材质（不透明灰度，单像素）
        uniform sampler2D uvTexture;
        void main(){
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(yTexture,vTexCoord).r;
            yuv.g = texture2D(uvTexture,vTexCoord).a - 0.5;
            yuv.b = texture2D(uvTexture,vTexCoord).r - 0.5;
            rgb = mat3(1.0,     1.0,    1.0,
                       0.0,-0.39465,2.03211,
                       1.13983,-0.58060,0.0)*yuv;
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

bool XShader::Init(XShaderType type){
    //初始化顶点着色器
    vsh = InitShader(vertexShader,GL_VERTEX_SHADER);
    if(vsh == 0){
        XLOGE("init GL_VERTEX_SHADER failed!");
        return false;
    }
    XLOGI("init GL_VERTEX_SHADER success!");

    //初始化片元着色器yuv420
    switch(type){
        case XSHADER_YUV420P:
            fsh = InitShader(fragYUV420P,GL_FRAGMENT_SHADER);
            break;
        case XSHADER_NV12:
            fsh = InitShader(fragNV12,GL_FRAGMENT_SHADER);
            break;
        case XSHADER_NV21:
            fsh = InitShader(fragNV21,GL_FRAGMENT_SHADER);
            break;
        default:
            XLOGE("XSHADER format is error");
            return false;
    }

    if(fsh == 0){
        XLOGE("init GL_FRAGMENT_SHADER failed!");
        return false;
    }
    XLOGI("init GL_FRAGMENT_SHADER success!");

    //创建渲染程序
    program = glCreateProgram();
    if(program == 0){
        XLOGE("glCreateProgram failed!");
        return false;
    }
    XLOGI("glCreateProgram success!");

    //渲染程序种加入着色器代码
    glAttachShader(program,vsh);
    glAttachShader(program,fsh);

    //链接程序
    glLinkProgram(program);

    //判断链接结果
    GLint status;
    glGetProgramiv(program,GL_LINK_STATUS,&status);
    if(status != GL_TRUE){
        XLOGE("glLinkProgram failed!");
        return false;
    }
    glUseProgram(program);
    XLOGI("glLinkProgram success!");

    //加入三维顶点数据 两个三角形组成正方形(数组元素的顺序无影响)
    static float vers[]={
            /*1.0f,-1.0f,0.0f,
            -1.0f,-1.0f,0.0f,
            -1.0f,1.0f,0.0f,
            1.0f,1.0f,0.0f,*/

            1.0f,-1.0f,0.0f,
            -1.0f,-1.0f,0.0f,
            1.0f,1.0f,0.0f,
            -1.0f,1.0f,0.0f,
    };

    GLuint apos = glGetAttribLocation(program,"aPosition");
    glEnableVertexAttribArray(apos);
    //传递顶点
    glVertexAttribPointer(apos,3,GL_FLOAT,GL_FALSE,12,vers);

    //加入材质坐标数据
    static float txts[]{
            /*1.0f,0.0f,
            0.0f,0.0f,
            0.0f,1.0f,
            1.0f,1.0f*/
            1.0f,0.0f , //右下
            0.0f,0.0f,
            1.0f,1.0f,
            0.0,1.0
    };
    GLuint atex = glGetAttribLocation(program,"aTexCoord");
    glEnableVertexAttribArray(atex);
    glVertexAttribPointer(atex,2,GL_FLOAT,GL_FALSE,8,txts);

    //初始化材质纹理
    glUniform1i(glGetUniformLocation(program,"yTexture"),0);
    switch (type) {
        case XSHADER_YUV420P:
            glUniform1i(glGetUniformLocation(program, "uTexture"), 1); //对于纹理第2层
            glUniform1i(glGetUniformLocation(program, "vTexture"), 2); //对于纹理第3层
            break;
        case XSHADER_NV21:
        case XSHADER_NV12:
            glUniform1i(glGetUniformLocation(program, "uvTexture"), 1); //对于纹理第2层
            break;
    }


    XLOGI("初始化shader成功");
    return true;
}

void XShader::GetTexture(unsigned int index,int width,int height, unsigned char *buf,bool isa){

    //默认格式时灰度图
    unsigned int format = GL_LUMINANCE;
    if(isa){
        //如果带透明通道
        format = GL_LUMINANCE_ALPHA;
    }
    if (texts[index] == 0){
        //材质初始化
        glGenTextures(1,&texts[index]);

        //设置纹理属性
        glBindTexture(GL_TEXTURE_2D,texts[index]);

        //缩小的过滤器
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
        //放大的过滤器
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);

        //设置纹理格式和大小
        glTexImage2D(
                //纹理类型
                GL_TEXTURE_2D,
                //纹理的等级，0最大，默认0
                0,
                //gpu内部格式 亮度 灰度图
                format,
                //纹理图像的宽度和高度
                width,
                height,
                //边框大小
                0,
                //像素数据的格式 亮度，灰度图 要与上面一致
                format,
                //像素值的数据类型
                GL_UNSIGNED_BYTE,
                //纹理数据（像素数据）
                NULL
        );
    }
    //激活第一层纹理，绑定到创建的opengl
    glActiveTexture(GL_TEXTURE0+index);
    glBindTexture(GL_TEXTURE_2D,texts[index]);
    //替换纹理内容
    glTexSubImage2D(GL_TEXTURE_2D,0,0,0,width,height,format,GL_UNSIGNED_BYTE,buf);
    XLOGI("XShader::GetTexture->success texts[index] ==%d",texts[index]);
}

void XShader::Draw() {
    if(!program){
        return;
    }

    //三维绘制
    glDrawArrays(GL_TRIANGLE_STRIP,0,4);
    XLOGI("XShader::Draw->success");
}














