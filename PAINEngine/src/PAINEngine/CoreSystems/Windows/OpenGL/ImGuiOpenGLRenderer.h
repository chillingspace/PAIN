#pragma once

struct GLFWwindow;

class ImGuiOpenGLRenderer
{
public:
    static bool Init(GLFWwindow* window);
    static void Shutdown();
    static void BeginFrame();
    static void EndFrame();

    static double lastTime; // last fps update
    static int frames; // frames rendered since lastTime
    static float FPS;
};