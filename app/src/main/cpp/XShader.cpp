//
// Created by renzhenming on 2018/10/24.
//
#include "XShader.h"
#include <GLES2/gl2.h>
#include <malloc.h>
#include "XLog.h"

#define GET_STR(x) #x

/**
 * 顶点着色器glsl
 *
 * 顶点着色器被适用于传统的基于顶点的操作，例如位移矩阵，计算光照方程，产生贴图坐标，顶点着色器被应用指定，
 * 应用于客户的顶点转化，顶点着色器能绘制的几何图形包括三角形，直线 ，点
 *
 * 几个重要的名词
 * 基元装配：基元是能被OpenGLES绘制的几何物体，基元装配就是将基元输入片段着色器的
 * 光栅化：转化图元为二维片段的过程，被片段着色器执行，二维的片段像素可以被绘制到屏幕上
 */
static const char *vertexShader = GET_STR(

//attribute 可以看做顶点着色器的输入
//varying就是顶点着色器的输出

//顶点坐标，定义输入顶点矩阵
        attribute
        vec4 aPosition;
        //材质顶点坐标，定义输入顶点属性 颜色
        attribute
        vec2 aTexCoord;

        //输出的材质坐标
        //定点着色器的输出叫做varying变量
        varying
        vec2 vTexCoord;

        void main() {
            vTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);

            //编译时，gl_Position被编译器自动定义，读入顶点矩阵aPosition
            //输出到gl_Position上，（通常会有一系列的矩阵变换），这里直接赋值
            gl_Position = aPosition;
        }
);

/**
 * 片元着色器（也叫片段着色器）,软解码和部分x86硬解码
 * 片段着色器也能丢弃片段或者产生一些颜色值像gl_FragColor，光栅化阶段产生的颜色、深度、模板、屏幕坐标（Xw，Yw）
 * 变成OpenGL ES 2.0管线输入
 *
 */
static const char *fragYUV420P = GET_STR(
//精度,设置显示质量
        precision
        mediump float;
        //顶点着色器传递的坐标
        varying
        vec2 vTexCoord;
        //输入的材质，Uniforms-常量数据
        uniform
        sampler2D yTexture;
        uniform
        sampler2D uTexture;
        uniform
        sampler2D vTexture;

        void main() {
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(yTexture, vTexCoord).r;
            yuv.g = texture2D(uTexture, vTexCoord).r - 0.5;
            yuv.b = texture2D(vTexture, vTexCoord).r - 0.5;

            rgb = mat3(
                    1.0, 1.0, 1.0,
                    0.0, -0.39465, 2.03211,
                    1.13983, -0.58060, 0.0
            ) * yuv;

            //输出像素颜色
            gl_FragColor = vec4(rgb, 1.0);
        }

);

//片元着色器,软解码和部分x86硬解码
static const char *fragNV12 = GET_STR(
        precision
        mediump float;    //精度
        varying
        vec2 vTexCoord;     //顶点着色器传递的坐标
        uniform
        sampler2D yTexture; //输入的材质（不透明灰度，单像素）
        uniform
        sampler2D uvTexture;
        void main() {
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(yTexture, vTexCoord).r;
            yuv.g = texture2D(uvTexture, vTexCoord).r - 0.5;
            yuv.b = texture2D(uvTexture, vTexCoord).a - 0.5;
            rgb = mat3(1.0, 1.0, 1.0,
                       0.0, -0.39465, 2.03211,
                       1.13983, -0.58060, 0.0) * yuv;
            //输出像素颜色
            gl_FragColor = vec4(rgb, 1.0);
        }
);

//片元着色器,软解码和部分x86硬解码
static const char *fragNV21 = GET_STR(
        precision
        mediump float;    //精度
        varying
        vec2 vTexCoord;     //顶点着色器传递的坐标
        uniform
        sampler2D yTexture; //输入的材质（不透明灰度，单像素）
        uniform
        sampler2D uvTexture;
        void main() {
            vec3 yuv;
            vec3 rgb;
            yuv.r = texture2D(yTexture, vTexCoord).r;
            yuv.g = texture2D(uvTexture, vTexCoord).a - 0.5;
            yuv.b = texture2D(uvTexture, vTexCoord).r - 0.5;
            rgb = mat3(1.0, 1.0, 1.0,
                       0.0, -0.39465, 2.03211,
                       1.13983, -0.58060, 0.0) * yuv;
            //输出像素颜色
            gl_FragColor = vec4(rgb, 1.0);
        }
);

static GLuint InitShader(const char *code, GLint type) {
    //创建shader
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        XLOGI("glCreateShader %d failed!", type);
        return 0;
    }

    //加载shader 1 shader数量 code shader代码 0代码长度
    //使用glShaderSource装载着色器源码
    glShaderSource(shader, 1, &code, 0);

    //编译shader
    //使用glCompileShader装载着色器对象
    glCompileShader(shader);

    //获取编译情况
    GLint status;

    //装载着色器后，装载状态和产生的错误被打印出来
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        XLOGI("glCompileShader failed!");
        /*GLint infoLen = 0;
        glGetShaderiv(shader,GL_INFO_LOG_LENGTH,&infoLen);
        if (infoLen>1){
            char* infoLog = (char *)malloc(sizeof(char)*infoLen);
            glGetShaderInfoLog(shader,infoLen,NULL,infoLog);
            esLogMessage("error compiling shader:%s",infoLog);
            free(infoLog);
        }*/
        //删除
        glDeleteShader(shader);
        return 0;
    }
    XLOGI("glCompileShader success!");
    return shader;
}

void XShader::Close(){
    mutex.lock();
    //释放shader
    if(program){
        glDeleteProgram(program);
    }

    if(vsh){
        glDeleteShader(vsh);
    }

    if(fsh){
        glDeleteShader(fsh);
    }

    //释放材质
    for(int i = 0; i< sizeof(texts)/sizeof(unsigned int);i++){
        if(texts[i]){
            glDeleteTextures(1,&texts[i]);
        }
        texts[i] = 0;
    }
    mutex.unlock();
}

bool XShader::Init(XShaderType type) {

    Close();

    mutex.lock();

    //初始化顶点着色器
    vsh = InitShader(vertexShader, GL_VERTEX_SHADER);
    if (vsh == 0) {
        mutex.unlock();
        XLOGE("init GL_VERTEX_SHADER failed!");
        return false;
    }
    XLOGI("init GL_VERTEX_SHADER success!");

    //初始化片元着色器yuv420
    switch (type) {
        case XSHADER_YUV420P:
            fsh = InitShader(fragYUV420P, GL_FRAGMENT_SHADER);
            break;
        case XSHADER_NV12:
            fsh = InitShader(fragNV12, GL_FRAGMENT_SHADER);
            break;
        case XSHADER_NV21:
            fsh = InitShader(fragNV21, GL_FRAGMENT_SHADER);
            break;
        default:
            mutex.unlock();
            XLOGE("XSHADER format is error");
            return false;
    }

    if (fsh == 0) {
        mutex.unlock();
        XLOGE("init GL_FRAGMENT_SHADER failed!");
        return false;
    }
    XLOGI("init GL_FRAGMENT_SHADER success!");

    //创建项目对象，链接顶点着色器和片段着色器
    //创建渲染程序
    program = glCreateProgram();
    if (program == 0) {
        mutex.unlock();
        XLOGE("glCreateProgram failed!");
        return false;
    }
    XLOGI("glCreateProgram success!");

    //渲染程序种加入着色器代码
    glAttachShader(program, vsh);
    glAttachShader(program, fsh);

    //链接程序
    glLinkProgram(program);

    //判断链接结果
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        mutex.unlock();
        XLOGE("glLinkProgram failed!");
        glDeleteProgram(program);
        return false;
    }
    glUseProgram(program);
    XLOGI("glLinkProgram success!");

    //加入三维顶点数据 两个三角形组成正方形(数组元素的顺序无影响)
    static float vers[] = {
            /*1.0f,-1.0f,0.0f,
            -1.0f,-1.0f,0.0f,
            -1.0f,1.0f,0.0f,
            1.0f,1.0f,0.0f,*/

            1.0f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f,
    };

    GLuint apos = glGetAttribLocation(program, "aPosition");
    glEnableVertexAttribArray(apos);
    //传递顶点
    glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 12, vers);

    //加入材质坐标数据
    static float txts[]{
            /*1.0f,0.0f,
            0.0f,0.0f,
            0.0f,1.0f,
            1.0f,1.0f*/
            1.0f, 0.0f, //右下
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0, 1.0
    };
    GLuint atex = glGetAttribLocation(program, "aTexCoord");
    glEnableVertexAttribArray(atex);
    glVertexAttribPointer(atex, 2, GL_FLOAT, GL_FALSE, 8, txts);

    //初始化材质纹理
    //下面两个命令是相同的，只是输入参数一个是浮点数，另一个是整型数
    //glUniform2f(location, 1.0f, 0.0f)
    //glUniform2i(location, 1, 0)
    //下面的命令也是相同，只是输入参数一个是矢量，另一个不是
    //glUniform4fv(location, coord);
    //glUniform4f(location, coord[0], coord[1], coord[2], coord[3])
    glUniform1i(glGetUniformLocation(program, "yTexture"), 0);
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
    mutex.unlock();
    return true;
}

void XShader::GetTexture(unsigned int index, int width, int height, unsigned char *buf, bool isa) {

    //默认格式时灰度图
    unsigned int format = GL_LUMINANCE;
    if (isa) {
        //如果带透明通道
        format = GL_LUMINANCE_ALPHA;
    }
    mutex.lock();
    if (texts[index] == 0) {
        //材质初始化
        glGenTextures(1, &texts[index]);

        //设置纹理属性
        glBindTexture(GL_TEXTURE_2D, texts[index]);

        //缩小的过滤器
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //放大的过滤器
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
    glActiveTexture(GL_TEXTURE0 + index);
    glBindTexture(GL_TEXTURE_2D, texts[index]);
    //替换纹理内容
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, buf);
    XLOGI("XShader::GetTexture->success texts[index] ==%d", texts[index]);

    mutex.unlock();
}

void XShader::Draw() {
    mutex.lock();
    if (!program) {
        mutex.unlock();
        return;
    }

    //三维绘制
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    XLOGI("XShader::Draw->success");
    mutex.unlock();
}














