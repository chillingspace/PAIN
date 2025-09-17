#pragma once

struct GLFWwindow;

class ImGuiOpenGLRenderer
{
public:
    static bool Init(GLFWwindow* window);
    static void Shutdown();
    static void BeginFrame();
    static void EndFrame();
};