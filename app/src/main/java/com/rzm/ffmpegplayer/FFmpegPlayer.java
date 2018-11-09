package com.rzm.ffmpegplayer;

import android.content.Context;
import android.media.MediaPlayer;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class FFmpegPlayer extends GLSurfaceView implements SurfaceHolder.Callback,GLSurfaceView.Renderer{

    static {
        System.loadLibrary("ffmpeg");
    }

    public FFmpegPlayer(Context context, AttributeSet attrs) {
        super(context, attrs);

        //android 8.0 需要设置,不设置会显示白屏
        //只有在绘制数据改变时才绘制view，可以防止GLSurfaceView帧重绘
        //setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
        setRenderer( this );
    }

    /**
     * 获取当前播放进度百分比，不是具体的播放时间点
     * @return
     */
    public native static double getCurrentPosition();

    /**
     * 暂停或开始播放，如果当前正在播放，则暂停，如果当前已经暂停，则重新开始播放
     */
    public native static void pause();

    /**
     * 拖到一定的位置播放，这个位置是指当前播放进度的百分比
     * @param v
     */
    public native static void seekTo(double v);

    /**
     * 开始播放
     * @param path 视频路径，可以是本地视频或者网络视频
     */
    public native static void start(String path);

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        initView(holder.getSurface());
    }

    private native void initView(Surface surface);

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

        MediaPlayer player = new MediaPlayer();
        player.start();
        player.seekTo(1);
        player.pause();
        player.getCurrentPosition();
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

    /**
     * 播放视频
     */
    public native void playVideo(String url,Surface surface);
    public native void playAudio(String url);
    public native void initOpenGL(String url,Surface surface);

}
