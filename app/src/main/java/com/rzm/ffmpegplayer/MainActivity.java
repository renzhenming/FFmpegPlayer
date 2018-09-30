package com.rzm.ffmpegplayer;

import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    private SurfaceView surfaceView;
    private Button play;
    private FFmpegPlayer fFmpegPlayer;
    private String path;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        fFmpegPlayer = new FFmpegPlayer();
        surfaceView = (SurfaceView) findViewById(R.id.surface);
        play = (Button) findViewById(R.id.play);
        path = Environment.getExternalStorageDirectory().getAbsolutePath()+"/1080.mp4";
    }

    public void play(View view) {
        fFmpegPlayer.play(path,surfaceView.getHolder().getSurface());
    }
}
