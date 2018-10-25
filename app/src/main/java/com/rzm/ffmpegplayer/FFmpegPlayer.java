package com.rzm.ffmpegplayer;

import android.content.Context;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;

public class FFmpegPlayer extends GLSurfaceView implements Runnable,SurfaceHolder.Callback{

    static {
        System.loadLibrary("ffmpeg");
    }

    public FFmpegPlayer(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    /**
     * 播放视频
     */
    public native void playVideo(String url,Surface surface);
    public native void playAudio(String url);
    public native void initOpenGL(String url,Surface surface);
    public native void stringFromJNI();

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

        stringFromJNI();
        initView(holder.getSurface());

    }

    private native void initView(Surface surface);

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    }
}
