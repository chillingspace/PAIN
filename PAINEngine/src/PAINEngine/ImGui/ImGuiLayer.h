#pragma once
#ifndef PDEBUG
#ifndef IMGUI_LAYER_HPP
#define IMGUI_LAYER_HPP

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include <ImGui/headers/imgui.h>
#include <ImGui/headers/imgui_impl_glfw.h>
#include <ImGui/headers/imgui_impl_opengl3.h>
#include "Managers/Windows/Window.h"

struct GLFWwindow; // forward declaration

namespace PAIN {

    class ImGuiLayer : public AppSystem {
    public:
        ImGuiLayer();
        ~ImGuiLayer();                       // no override

        void onAttach() override;
        void onDetach() override;
        void onUpdate() override;
        void onEvent(Event::Event& event) override;

        void BeginFrame();
        void EndFrame();

    private:
        float       m_Time;
        GLFWwindow* m_Window;
        bool        m_BlockEvents;
        bool        m_Initialized = false;   // <— add this
    };


} // namespace PAIN
#endif // IMGUI_LAYER_HPP
#endif // PDEBUG
