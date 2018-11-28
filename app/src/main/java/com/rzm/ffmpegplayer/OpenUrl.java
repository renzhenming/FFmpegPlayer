
package com.rzm.ffmpegplayer;

import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;

public class OpenUrl extends AppCompatActivity {
    private Button btfile;
    private Button btrtmp;

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

        setContentView(R.layout.openurl);
        btfile = (Button) findViewById(R.id.playvideo);
        btfile.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        EditText t = (EditText) findViewById(R.id.fileurl);
                        //用户输入的URL，打开视频
                        FFmpegPlayer.start(t.getText().toString());
                        //关闭当前窗口
                        finish();
                    }
                }
        );
        btrtmp = (Button) findViewById(R.id.playrtmp);
        btrtmp.setOnClickListener(
                new View.OnClickListener() {
                    @Override
                    public void onClick(View view) {
                        EditText t = (EditText) findViewById(R.id.rtmpurl);
                        //用户输入的URL，打开视频
                        FFmpegPlayer.start(t.getText().toString());
                        //关闭当前窗口
                        finish();
                    }
                }
        );
    }

}
