package com.pain.engine;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import android.content.Context;
import android.content.res.AssetManager;
import android.opengl.GLSurfaceView;

public class MyGLRenderer implements GLSurfaceView.Renderer {

    public native void onSurfaceCreated(AssetManager assetManager);
    public native void onSurfaceChanged(int width, int height);
    public native void onDrawFrame();

    private AssetManager mAssetManager;

    public MyGLRenderer(Context context) {
        mAssetManager = context.getAssets();
    }

    @Override
    public void onSurfaceCreated(GL10 unused, EGLConfig config) {
        onSurfaceCreated(mAssetManager);
    }

    @Override
    public void onSurfaceChanged(GL10 unused, int width, int height) {
        onSurfaceChanged(width, height);
    }

    @Override
    public void onDrawFrame(GL10 unused) {
        onDrawFrame();
    }
}