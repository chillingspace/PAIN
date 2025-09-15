#include "pch.h"
#include <GL/glew.h>
#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM

#include "ImGui/headers/imgui.h"
#include "ImGui/headers/imgui_impl_glfw.h"
#include "ImGui/headers/imgui_impl_opengl3.h"
#include "ImGuiOpenGLRenderer.h"

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
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiOpenGLRenderer::EndFrame()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}