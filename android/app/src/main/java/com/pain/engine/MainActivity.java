package com.pain.engine;

import androidx.appcompat.app.AppCompatActivity;
import android.app.ActivityManager;
import android.content.Context;
import android.content.pm.ConfigurationInfo;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;
import android.os.Bundle;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends AppCompatActivity {

    private GLSurfaceView mGLSurfaceView;

    // Load the native library that contains the C++ engine code.
    static {
        System.loadLibrary("Game");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mGLSurfaceView = new GLSurfaceView(this);

        // Check if the system supports OpenGL ES 2.0 or higher.
        final ActivityManager activityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        final ConfigurationInfo configurationInfo = activityManager.getDeviceConfigurationInfo();
        final boolean supportsEs2 = configurationInfo.reqGlEsVersion >= 0x20000;

        if (supportsEs2) {
            // Request an OpenGL ES 2.0 compatible context.
            mGLSurfaceView.setEGLContextClientVersion(2);

            // Set the Renderer for drawing on the GLSurfaceView
            mGLSurfaceView.setRenderer(new Renderer());
        } else {
            // This is where you could fail gracefully.
            return;
        }

        setContentView(mGLSurfaceView);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mGLSurfaceView.onResume();
        nativeOnResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mGLSurfaceView.onPause();
        nativeOnPause();
    }
    
    @Override
    protected void onDestroy() {
        super.onDestroy();
        nativeOnDestroy();
    }

    // --- Native Method Declarations (JNI Bridge) ---
    public static native void nativeInit(AssetManager assetManager);
    public static native void nativeOnSurfaceChanged(int width, int height);
    public static native void nativeOnDrawFrame();
    public static native void nativeOnPause();
    public static native void nativeOnResume();
    public static native void nativeOnDestroy();

    /**
     * Renderer class that handles the OpenGL rendering lifecycle.
     */
    private class Renderer implements GLSurfaceView.Renderer {
        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            // Pass the AssetManager to the native code for initialization.
            nativeInit(getAssets());
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            nativeOnSurfaceChanged(width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl) {
            nativeOnDrawFrame();
        }
    }
}