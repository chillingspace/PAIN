package com.example.jnicpp;

import androidx.appcompat.app.AppCompatActivity;

import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.widget.Toast;

import com.example.jnicpp.databinding.ActivityMainBinding;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class MainActivity extends AppCompatActivity implements GLSurfaceView.Renderer {

    private static final String TAG = "MainActivity";

    // Used to load the 'jnicpp' library on application startup.
    static {
        try {
            Log.d(TAG, "Attempting to load native library 'jnicpp'");
            System.loadLibrary("jnicpp");
            Log.d(TAG, "Native library loaded successfully");
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Failed to load native library", e);
            Log.e(TAG, "Error details: " + e.getMessage());
        } catch (Exception e) {
            Log.e(TAG, "Unexpected error loading native library", e);
        }
    }

    private ActivityMainBinding binding;
    private GLSurfaceView glSurfaceView;
    private boolean isInitialized = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Log.d(TAG, "onCreate called");

        try {
            binding = ActivityMainBinding.inflate(getLayoutInflater());
            setContentView(binding.getRoot());

            // Setup GLSurfaceView
            glSurfaceView = binding.glSurfaceView;
            glSurfaceView.setEGLContextClientVersion(3); // OpenGL ES 3.0
            glSurfaceView.setRenderer(this);
            glSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

            // Example of a call to a native method
            try {
                Log.d(TAG, "Calling stringFromJNI()");
                String message = stringFromJNI();
                Log.d(TAG, "Native message: " + message);
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "Failed to call stringFromJNI()", e);
                Log.e(TAG, "Error details: " + e.getMessage());
            } catch (Exception e) {
                Log.e(TAG, "Unexpected error calling stringFromJNI()", e);
            }
        } catch (Exception e) {
            Log.e(TAG, "Error in onCreate", e);
            showToast("Error initializing app: " + e.getMessage());
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        Log.d(TAG, "onPause called");
        if (glSurfaceView != null) {
            glSurfaceView.onPause();
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        Log.d(TAG, "onResume called");
        if (glSurfaceView != null) {
            glSurfaceView.onResume();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        Log.d(TAG, "onDestroy called");
        if (isInitialized) {
            try {
                cleanupGL();
            } catch (Exception e) {
                Log.e(TAG, "Error during cleanup", e);
            }
        }
    }

    // Helper method to show toast from any thread
    private void showToast(final String message) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Toast.makeText(MainActivity.this, message, Toast.LENGTH_SHORT).show();
            }
        });
    }

    // GLSurfaceView.Renderer implementation
    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        Log.d(TAG, "Surface created");
        try {
            // Initialize OpenGL in native code with AssetManager
            boolean success = initGL(getAssets());
            if (success) {
                isInitialized = true;
                Log.d(TAG, "OpenGL ES 3.0 initialized successfully");
            } else {
                Log.e(TAG, "Failed to initialize OpenGL ES 3.0");
                showToast("Failed to initialize OpenGL ES 3.0");
            }
        } catch (Exception e) {
            Log.e(TAG, "Error in onSurfaceCreated", e);
            showToast("Error creating surface: " + e.getMessage());
        }
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        Log.d(TAG, "Surface changed: " + width + "x" + height);
        // Set display size for ImGui and recreate device objects
        if (isInitialized) {
            setDisplaySize(width, height);
            
            // Get device DPI and set it for ImGui scaling
            float dpi = getResources().getDisplayMetrics().densityDpi;
            Log.d(TAG, "Device DPI: " + dpi);
            setDPI(dpi);
            
            // Recreate ImGui device objects for new context
            recreateImGuiDeviceObjects();
        }
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        if (isInitialized) {
            try {
                renderFrame();
            } catch (Exception e) {
                Log.e(TAG, "Error in onDrawFrame", e);
            }
        }
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (isInitialized) {
            int action = event.getAction();
            float x = event.getX();
            float y = event.getY();
            
            // Convert action to ImGui format
            int imguiAction = -1;
            switch (action) {
                case MotionEvent.ACTION_DOWN:
                    imguiAction = 0;
                    break;
                case MotionEvent.ACTION_UP:
                    imguiAction = 1;
                    break;
                case MotionEvent.ACTION_MOVE:
                    imguiAction = 2;
                    break;
            }
            
            if (imguiAction >= 0) {
                handleTouchEvent(imguiAction, x, y);
            }
        }
        return super.onTouchEvent(event);
    }

    /**
     * A native method that is implemented by the 'jnicpp' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    /**
     * Initialize OpenGL ES 3.0 renderer
     */
    public native boolean initGL(android.content.res.AssetManager assetManager);

    /**
     * Render a frame
     */
    public native void renderFrame();

    /**
     * Cleanup OpenGL resources
     */
    public native void cleanupGL();

    /**
     * Set display size for ImGui
     */
    public native void setDisplaySize(int width, int height);

    /**
     * Handle touch events for ImGui
     */
    public native void handleTouchEvent(int action, float x, float y);

    /**
     * Handle key events for ImGui
     */
    public native void handleKeyEvent(int key, int action);

    /**
     * Recreate ImGui device objects (useful for context recreation)
     */
    public native boolean recreateImGuiDeviceObjects();

    /**
     * Set device DPI for ImGui scaling
     */
    public native void setDPI(float dpi);
}