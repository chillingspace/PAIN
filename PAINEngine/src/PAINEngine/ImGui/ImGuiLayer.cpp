#include "pch.h"
#include "ImGuiLayer.h"
#include <GLFW/glfw3.h>

namespace PAIN {

    ImGuiLayer::ImGuiLayer()
        : m_Time(0.0f), m_Window(nullptr), m_BlockEvents(true) {
    }

    ImGuiLayer::~ImGuiLayer() {}

    void ImGuiLayer::onAttach() {
        if (m_Initialized) return;

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui::StyleColorsDark();

        // Get the window from the current OpenGL context
        m_Window = glfwGetCurrentContext();

        ImGui_ImplGlfw_InitForOpenGL(m_Window, true);
        ImGui_ImplOpenGL3_Init("#version 450");

        m_Initialized = true;
    }

    void ImGuiLayer::onDetach() {
        if (!m_Initialized) return;
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        m_Initialized = false;
    }

    void ImGuiLayer::onUpdate() {
        if (!m_Initialized) onAttach();   // <— ensure initialized

        BeginFrame();

        static bool show_demo = true;
        if (show_demo) ImGui::ShowDemoWindow(&show_demo);

        ImGui::Begin("PAIN Engine Debug");
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
            1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::Checkbox("Show Demo Window", &show_demo);
        ImGui::End();

        EndFrame();
    }

    void ImGuiLayer::BeginFrame() {
        if (!m_Initialized) onAttach();   // <— guard (extra safety)
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiLayer::EndFrame() {
        ImGuiIO& io = ImGui::GetIO();
        if (m_Window) {
            int w, h;
            glfwGetFramebufferSize(m_Window, &w, &h);
            io.DisplaySize = ImVec2((float)w, (float)h);
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void ImGuiLayer::onEvent(Event::Event& event) {
        if (m_BlockEvents) {
            ImGuiIO& io = ImGui::GetIO();
            // hook into your event system if desired
        }
    }

} // namespace PAIN
