package com.pain.engine;

import androidx.appcompat.app.AppCompatActivity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import org.fmod.FMOD;

public class MainActivity extends AppCompatActivity {
    private GLSurfaceView mGLView;

    static {
        System.loadLibrary("fmod"); // Load FMOD Core ONLY
        System.loadLibrary("Game");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        FMOD.init(this);

        mGLView = new GLSurfaceView(this);
        mGLView.setEGLContextClientVersion(3);
        mGLView.setRenderer(new MyGLRenderer(this));
        setContentView(mGLView);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mGLView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mGLView.onResume();
    }

    @Override
    protected void onDestroy() {
        FMOD.close();
        super.onDestroy();
    }
}