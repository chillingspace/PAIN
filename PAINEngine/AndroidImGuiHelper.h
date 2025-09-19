#ifndef ANDROIDIMGUIHELPER_H
#define ANDROIDIMGUIHELPER_H

#include "Platform.h"

/**
 * AndroidImGuiHelper - Handles Android-specific ImGui functionality
 * 
 * DPI Scaling:
 * This class automatically handles DPI scaling for ImGui widgets on Android devices.
 * Call setDPI() with the device's DPI value, then applyDPIScaling() to scale all
 * ImGui widgets, fonts, and UI elements appropriately for the device's screen density.
 * 
 * The scaling is based on Android's mdpi baseline (160dpi), so:
 * - mdpi (160dpi): 1.0x scaling
 * - hdpi (240dpi): 1.5x scaling  
 * - xhdpi (320dpi): 2.0x scaling
 * - xxhdpi (480dpi): 3.0x scaling
 * - xxxhdpi (640dpi): 4.0x scaling
 */
class AndroidImGuiHelper {
public:
    static void setDisplaySize(int width, int height);
    static void handleTouchEvent(int action, float x, float y);
    static void handleKeyEvent(int key, int action);
    static void updateImGuiIO();
    static void setDPI(float dpi);
    static void applyDPIScaling();
    static float getDPI() { return deviceDPI; }

private:
    static int displayWidth;
    static int displayHeight;
    static bool touchDown;
    static float lastTouchX;
    static float lastTouchY;
    static float deviceDPI;
    static bool dpiScalingApplied;
};

#endif // ANDROIDIMGUIHELPER_H
