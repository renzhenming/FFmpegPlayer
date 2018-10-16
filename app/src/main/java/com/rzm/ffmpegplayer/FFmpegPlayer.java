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

    @Override
    public void run() {
        String path = Environment.getExternalStorageDirectory().getAbsolutePath()+"/1080.mp4";
        playVideo(path,getHolder().getSurface());
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        new Thread( this ).start();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    }
}
