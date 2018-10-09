package com.rzm.ffmpegplayer;

import android.view.Surface;

public class FFmpegPlayer {

    static {
        System.loadLibrary("ffmpeg");
    }

    /**
     * 播放视频
     */
    public native void play(String url,Surface handle);

    public native void playVideo(String url,Surface surface);

}
