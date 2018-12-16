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

        //定点着色器被调用的时候，在aPosition（有的书中写作a_Position）
        //中接收当前顶点的位置，
        //attribute用于经常修改的信息，只能在定点着色器中使用
        attribute vec4 aPosition;
        //材质顶点坐标，定义输入顶点属性 颜色
        attribute vec2 aTexCoord;

        //输出的材质坐标
        //定点着色器的输出叫做varying变量
        //在顶点着色器中定义vTexCoord代表纹理的坐标点，并对它赋值
        varying vec2 vTexCoord;

        void main() {
            //计算纹理坐标
            vTexCoord = vec2(aTexCoord.x, 1.0 - aTexCoord.y);

            //main方法执行时，会把上边定义的定点位置复制到输出变量gl_Position
            //OPenGL会把gl_Position中存储的值当作顶点的最终位置
            //vec4 gl_Position：是顶点着色器的内置变量，用于设置顶点转换到屏幕坐标的位置
            //注意这个gl_Position有的书上写的是gl_position,这种写法是错误的
            //float gl_pointSize:也是内置变量，粒子效果的场景下，为粒子设置大小
            gl_Position = aPosition;
        }
);

/**
 * 片元着色器（也叫片段着色器）,软解码和部分x86硬解码
 * 通常一个片段可以看作屏幕上的一个像素，但并不总是准确，在一些超高分辨率的
 * 的设备上可能需要使用较大的片段
 * 片段着色器的目的就是告诉GPU每个片段的颜色应该是什么
 */
static const char *fragYUV420P = GET_STR(
        //精度,设置显示质量（lowp mudiump highp）
        //顶点着色器也有精度，默认值是highp,因为定点位置精确度很重要
        //对于片段着色器，出于兼容性考虑，使用mediump,因为紧度越高，
        //对性能影响越大，这是基于速度和质量的权衡
        precision mediump float;
        //顶点着色器传递的坐标 二维向量
        varying vec2 vTexCoord;
        //声明二维纹理类型
        uniform sampler2D yTexture;
        uniform sampler2D uTexture;
        uniform sampler2D vTexture;

        void main() {
            vec3 yuv;
            vec3 rgb;
            //texture2D取出二维纹理(vTexCoord)中该纹理坐标点上的纹理像素(yTexture)的值
            yuv.r = texture2D(yTexture, vTexCoord).r;
            //texture2D取出二维纹理(vTexCoord)中该纹理坐标点上的纹理像素(uTexture)的值
            yuv.g = texture2D(uTexture, vTexCoord).r - 0.5;
            //texture2D取出二维纹理(vTexCoord)中该纹理坐标点上的纹理像素(vTexture)的值
            yuv.b = texture2D(vTexture, vTexCoord).r - 0.5;

            //取出像数值后可进行像素变换

            //转换成rgb类型
            rgb = mat3(
                    1.0, 1.0, 1.0,
                    0.0, -0.39465, 2.03211,
                    1.13983, -0.58060, 0.0
            ) * yuv;

            //将uniform中定义的颜色最终复制到特殊的输出变量gl_FragColor，OpenGL
            //会把这个颜色作为当前片段的最终颜色
            //vec4 gl_FragColor：片元着色器的内置变量，指定当前纹理坐标所代表的像素点最终颜色值
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
    //创建shader对象，作为shader的容器，返回容器的句柄
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        XLOGI("glCreateShader %d failed!", type);
        return 0;
    }

    //加载shader 1 shader数量 code shader代码 0代码长度，设置为0自动检测长度
    //使用glShaderSource装载着色器源码，将着色器加载到着色器句柄所关联的内存中
    glShaderSource(shader, 1, &code, 0);

    //编译shader
    glCompileShader(shader);

    //获取编译情况
    GLint status;

    //获取是否编译成功的信息，返回status为1表示成功0表示失败
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == 0) {
        XLOGI("glCompileShader failed!");
        GLint infoLen = 0;
        //获取错误信息的长度，根据这个长度分配出一个buffer
        glGetShaderiv(shader,GL_INFO_LOG_LENGTH,&infoLen);
        if (infoLen>1){
            char* infoLog = (char *)malloc(sizeof(char)*infoLen);
            glGetShaderInfoLog(shader,infoLen,NULL,infoLog);
            XLOGE("init GL_VERTEX_SHADER failed!",infoLog);
            free(infoLog);
        }
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

    if(fsh){
        glDeleteShader(fsh);
    }

    if(vsh){
        glDeleteShader(vsh);
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

    //创建渲染程序对象，作为程序的容器，返回容器的句柄
    program = glCreateProgram();
    if (program == 0) {
        mutex.unlock();
        XLOGE("glCreateProgram failed!");
        return false;
    }
    XLOGI("glCreateProgram success!");

    //渲染程序种加入顶点着色器代码
    glAttachShader(program, vsh);
    //渲染程序种加入片元着色器代码
    glAttachShader(program, fsh);

    //链接程序
    glLinkProgram(program);

    //判断链接结果
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        mutex.unlock();
        XLOGE("glLinkProgram failed!");
        GLint infoLen = 0;
        //获取错误信息的长度，根据这个长度分配出一个buffer
        glGetProgramiv(program,GL_INFO_LOG_LENGTH,&infoLen);
        if (infoLen>1){
            char* infoLog = (char *)malloc(sizeof(char)*infoLen);
            glGetProgramInfoLog(program,infoLen,NULL,infoLog);
            XLOGE("glLinkProgram failed!",infoLog);
            free(infoLog);
        }
        //删除
        glDeleteProgram(program);
        return false;
    }
    XLOGI("glLinkProgram success!");
    //使用该程序
    glUseProgram(program);
    XLOGI("glUseProgram success!");


    //这里需要理解一下openGL的坐标系，在openGL中，
    //对应屏幕显示区域的右上角为（1，1） 右下角为（1，-1）
    //对应屏幕显示区域的左上角为（-1，1）左下角为（-1，-1）

    //下边这个数组包含的四个坐标位置，分别对应屏幕的四个顶点，
    //因为openGL只能绘制三角形而不能绘制矩形，所以这四个顶点就是
    //两个三角形组合在一起形成的一个矩形，假设四个顶点标记分别是1234
    //那么123是一个三角形，234是一个三角形你设置坐标的时候一定要保证
    //23两个坐标是两个三角形公用的两个点，这样才能做成一个矩形
    //但是这样还不能正确的显示，顶点坐标必须和材质坐标搭配合理才行

    //经过我大量测试，有这两个结论：
    //1.数组坐标中第二个和第三个坐标元素一定要是矩形对角线上的两个点，这样可以
    //保证两个三角形公用这两个点组成一个矩形

    //2.顶点坐标数组和纹理坐标数组元素的排序需要一致，既，如果顶点坐标数组是
    //右下，左下，右上，左上，那么纹理坐标同样需要保持这个顺序才能正确显示画面
    static float vers[] = {
        //右下
        1.0f, -1.0f,
        //左下
        -1.0f, -1.0f,
        //右上
        1.0f, 1.0f,
        //左上
        -1.0f, 1.0f
    };



    //设置物体坐标
    GLuint apos = glGetAttribLocation(program, "aPosition");
    glEnableVertexAttribArray(apos);
    //两个参数传0也可以
    //glVertexAttribPointer(apos, 3, GL_FLOAT, GL_FALSE, 12, vers);
    glVertexAttribPointer(apos, 2, GL_FLOAT, 0, 0, vers);

    //设置纹理坐标
    //这个坐标的作用是将计算机坐标系和OpenGl坐标系进行转换
    static float txts[]{
        //右下
        1.0f, 0.0f,
        //左下
        0.0f, 0.0f,
        //右上
        1.0f, 1.0f,
        //左上
        0.0, 1.0
    };
    GLuint atex = glGetAttribLocation(program, "aTexCoord");
    glEnableVertexAttribArray(atex);

    //两个参数传0也可以
    //glVertexAttribPointer(atex, 2, GL_FLOAT, GL_FALSE, 8, txts);
    glVertexAttribPointer(atex, 2, GL_FLOAT, 0, 0, txts);

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
        //创建一个纹理对象，第一个参数表示创建纹理对象的数量是1，创建成功把纹理对象的句柄放在第二个参数中
        //此时，就已经再显卡中创建了一个纹理对象
        glGenTextures(1, &texts[index]);

        //绑定纹理对象，绑定之后OpenGLES才直到操作哪一个对象
        glBindTexture(GL_TEXTURE_2D, texts[index]);

        //绑定纹理对象之后。下面的操作就是针对texts[index]这个纹理对象了

        //纹理被渲染的时候，有可能需要放大或缩小，设置放大缩小过滤器，确定每个像素如何被填充
        //视频渲染一般使用GL_LINEAR线性过滤，可使用双线性插值平滑像素之间的过渡，使用四个临近的纹理元素
        //并再他们之间用一个先行插值算法做插值，该过滤方式是最主要的过滤方式
        //给纹理对象设置参数，缩小的过滤器
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        //给纹理对象设置参数，放大的过滤器
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        //
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
                //纹理数据（像素数据），一般传RGBA格式数据 uint8_t 数组类型
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














