package com.rzm.ffmpegplayer;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class FFmpegPlayer extends GLSurfaceView implements Runnable,SurfaceHolder.Callback,GLSurfaceView.Renderer{

    static {
        System.loadLibrary("ffmpeg");
    }

    public FFmpegPlayer(Context context, AttributeSet attrs) {
        super(context, attrs);
        //android 8.0 需要设置,不设置会显示白屏
        setRenderer( this );
    }

    /**
     * 播放视频
     */
    public native void playVideo(String url,Surface surface);
    public native void playAudio(String url);
    public native void initOpenGL(String url,Surface surface);

    @Override
    public void run() {
        //String sdPath = Environment.getExternalStorageDirectory().getAbsolutePath();
        //String videoPath = sdPath +"/1080.mp4";
        //String audioPath = sdPath +"/test.pcm";
        //String yuvPath = sdPath +"/out.yuv";
        //String yuvPath2 = sdPath +"/176x144.yuv";
        //String yuvPath3 = sdPath +"/352x288.yuv";
        //String yuvPath4 = sdPath +"/352x288_2.yuv";
        //initOpenGL(yuvPath3,getHolder().getSurface());
        //playVideo(videoPath,getHolder().getSurface());
        //playAudio(audioPath);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        //new Thread( this ).start();
        //初始化opengl egl 显示

        initView(holder.getSurface());

        //android 8.0 需要设置,不设置会显示白屏,放在这里会重复被调用，发生bug,(每次activity onResume都会)
        //java.lang.IllegalStateException: setRenderer has already been called for this instance.
        //所以把他放在构造方法中是最合适的
        //setRenderer( this );


        //只有在绘制数据改变时才绘制view，可以防止GLSurfaceView帧重绘
        //setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);

    }

    private native void initView(Surface surface);

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {

    }

    @Override
    public void onDrawFrame(GL10 gl) {

    }
}
