package com.rzm.ffmpegplayer;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;

public class MainActivity extends AppCompatActivity {

    private SurfaceView surfaceView;
    private Button play;
    private FFmpegPlayer fFmpegPlayer;
    private String path;
    static {
        System.loadLibrary("ffmpeg");
    }

    public native void stringFromJNI();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
        setContentView(R.layout.activity_main);

        stringFromJNI();


        /*FFmpegPlayer player = (FFmpegPlayer) findViewById(R.id.surface);*/

        //目前在红米note5上不加render无法播放，在360n5s上可以正常播放
        /*player.setRenderer(new GLSurfaceView.Renderer() {
            @Override
            public void onSurfaceCreated(GL10 gl, EGLConfig config) {

            }

            @Override
            public void onSurfaceChanged(GL10 gl, int width, int height) {

            }

            @Override
            public void onDrawFrame(GL10 gl) {

            }
        });*/

        /*fFmpegPlayer = new FFmpegPlayer();
        surfaceView = (SurfaceView) findViewById(R.id.surface);
        play = (Button) findViewById(R.id.play);
        path = Environment.getExternalStorageDirectory().getAbsolutePath()+"/1080.mp4";*/
    }

    /*public void play(View view) {
        fFmpegPlayer.playVideo(path,surfaceView.getHolder().getSurface());
    }*/
}
