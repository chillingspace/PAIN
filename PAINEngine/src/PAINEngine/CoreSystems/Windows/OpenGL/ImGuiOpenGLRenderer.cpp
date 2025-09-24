#include "pch.h"

#ifndef PLATFORM_ANDROID
#include <GL/glew.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "ImGuiOpenGLRenderer.h"

double ImGuiOpenGLRenderer::lastTime = 0.0;
int ImGuiOpenGLRenderer::frames = 0;
float ImGuiOpenGLRenderer::FPS = 0.0f;


bool ImGuiOpenGLRenderer::Init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    return true;
}

void ImGuiOpenGLRenderer::Shutdown()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiOpenGLRenderer::BeginFrame()
{
    double currentTime = glfwGetTime();
    frames++;
    if (currentTime - lastTime >= 1.0) { // update once per second
        FPS = (float)frames / (float)(currentTime - lastTime);
        frames = 0;
        lastTime = currentTime;
    }


    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiOpenGLRenderer::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

#endif