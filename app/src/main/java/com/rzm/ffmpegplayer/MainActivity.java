package com.rzm.ffmpegplayer;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.SeekBar;

public class MainActivity extends AppCompatActivity implements Runnable {

    private View bt;
    private SeekBar seek;
    private Thread th;
    private FFmpegPlayer player;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        //去掉标题栏
        supportRequestWindowFeature( Window.FEATURE_NO_TITLE);
        //全屏，隐藏状态
        getWindow().setFlags( WindowManager.LayoutParams.FLAG_FULLSCREEN ,
                WindowManager.LayoutParams.FLAG_FULLSCREEN
        );
        //屏幕为横屏
        setRequestedOrientation( ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE );


        setContentView( R.layout.activity_main );
        player = (FFmpegPlayer) findViewById( R.id.player );
        player.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                FFmpegPlayer.pauseOrPlay();
            }
        });
        bt = findViewById( R.id.open_button );
        bt.setOnClickListener( new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.e("XPlay","open button click!");
                //打开选择路径窗口
                Intent intent = new Intent();
                intent.setClass( MainActivity.this ,OpenUrl.class);
                startActivity( intent );


            }
        } );

        seek = (SeekBar) findViewById( R.id.aplayseek );
        seek.setMax(1000);

        //启动播放进度线程
        th = new Thread(this);
        th.start();
    }



    @Override
    public void run() {
        for(;;)
        {
            seek.setProgress((int)(FFmpegPlayer.getCurrentPosition()*1000));
            try {
                Thread.sleep( 40 );
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }
    }
}
