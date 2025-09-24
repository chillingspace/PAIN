#include "pch.h"
#include "Editor.h"

#ifdef _DEBUG

//#include "CoreSystems/Events/GLFW/WindowEvents.h"
//#include "CoreSystems/Events/GLFW/KeyEvents.h"
//#include "CoreSystems/Events/GLFW/MouseEvents.h"
//#include "CoreSystems/Events/GLFW/AssetEvents.h"
#include "PAINEngine/Audio/AudioManager.h"

// Panels
#include "Panels/ToolsPanel.h"
#include "Panels/AudioPanel.h"
#include "Panels/ScenesPanel.h"
#include "Panels/ComponentsPanel.h"


namespace PAIN {

    namespace Editor {

        //int Editor::imguiKeyMapping(int code) {

        //    //Code within 0 - 9 range
        //    if (code >= GLFW_KEY_0 && code <= GLFW_KEY_9) {
        //        return ImGuiKey_0 + (code - GLFW_KEY_0);
        //    }
        //    //Code within A - Z range
        //    else if (code >= GLFW_KEY_A && code <= GLFW_KEY_Z) {
        //        return ImGuiKey_A + (code - GLFW_KEY_A);
        //    }
        //    //Code within F1 - F24 range
        //    else if (code >= GLFW_KEY_F1 && code <= GLFW_KEY_F24) {
        //        return ImGuiKey_F1 + (code - GLFW_KEY_F1);
        //    }
        //    //Code within KeyPad_0 - KeyPad_Equal range
        //    else if (code >= GLFW_KEY_KP_0 && code <= GLFW_KEY_KP_EQUAL) {
        //        return ImGuiKey_Keypad0 + (code - GLFW_KEY_KP_0);
        //    }
        //    //Code within Apostrophe - Slash range
        //    else if (code >= GLFW_KEY_APOSTROPHE && code <= GLFW_KEY_SLASH) {
        //        return ImGuiKey_Apostrophe + (code - GLFW_KEY_APOSTROPHE);
        //    }
        //    //Code within left bracket & grave accent
        //    else if (code >= GLFW_KEY_LEFT_BRACKET && code <= GLFW_KEY_GRAVE_ACCENT) {
        //        return ImGuiKey_LeftBracket + (code - GLFW_KEY_LEFT_BRACKET);
        //    }
        //    //Code within caps lock & pause
        //    else if (code >= GLFW_KEY_CAPS_LOCK && code <= GLFW_KEY_PAUSE) {
        //        return ImGuiKey_CapsLock + (code - GLFW_KEY_CAPS_LOCK);
        //    }
        //    //Code within caps lock & pause
        //    else if (code >= GLFW_KEY_PAGE_UP && code <= GLFW_KEY_END) {
        //        return ImGuiKey_PageUp + (code - GLFW_KEY_PAGE_UP);
        //    }

        //    //Mouse button codes
        //    switch (code) {
        //    case GLFW_MOUSE_BUTTON_LEFT: return ImGuiKey_MouseLeft;
        //    case GLFW_MOUSE_BUTTON_RIGHT: return ImGuiKey_MouseRight;
        //    case GLFW_MOUSE_BUTTON_MIDDLE: return ImGuiKey_MouseMiddle;
        //    case GLFW_MOUSE_BUTTON_4: return ImGuiKey_MouseX1;
        //    case GLFW_MOUSE_BUTTON_5: return ImGuiKey_MouseX2;
        //    }

        //    //Remaing key codes that are not in order
        //    switch (code) {
        //    case GLFW_KEY_SPACE: return ImGuiKey_Space;
        //    case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        //    case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        //    case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        //    case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        //    case GLFW_KEY_TAB: return ImGuiKey_Tab;
        //    case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        //    case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        //    case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        //    case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        //    case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        //    case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        //    case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        //    case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        //    case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        //    case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        //    case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        //    case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        //    case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        //    case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        //    case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        //    case GLFW_KEY_MENU: return ImGuiKey_Menu;
        //    }

        //    return ImGuiKey_None;
        //}

        Editor::Editor(void* window) {

            //Construct platform
            platform = std::shared_ptr<EditorPlatform>(EditorPlatform::createEditorPlatform(window));
        }

        Editor::~Editor() {}

        void Editor::onAttach() {

            // IMGUI_CHECKVERSION();
            // ImGui::CreateContext();

            // ImGuiIO& io = ImGui::GetIO();
            // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   
            // ImGui::StyleColorsDark();


            // ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
            // ImGui_ImplOpenGL3_Init("#version 450");

            // ImGui::LoadIniSettingsFromDisk(io.IniFilename);

            //Construct command manager
            command_manager = std::make_shared<CommandManager>();

            //Register panels
            //panels[CLASS_STR(Panel::Tools)] = std::make_shared<Panel::Tools>(command_manager);
            //PN_CORE_INFO(panels[CLASS_STR(Panel::Tools)]->getPanelName());

            // panels[CLASS_STR(Panel::DebugAudioPanel)] = std::make_shared<Panel::DebugAudioPanel>(command_manager);

            // panels[CLASS_STR(Panel::ScenesPanel)] = std::make_shared<Panel::ScenesPanel>(command_manager);

            // panels[CLASS_STR(Panel::ComponentsPanel)] =
            //     std::make_shared<Panel::ComponentsPanel>(command_manager);

        }

        void Editor::onDetach() {
        }

        void Editor::onUpdate() {

            //Update shortcuts
            //updateShortCuts();

            //Begin IMGUI Frame
            platform->beginFrame();

            BuildDockspace();

            //Update all panels
            //for (auto const& panel : panels) {
            //    panel.second->drawWindow();
            //}



            static bool show_demo = true;
            if (show_demo) ImGui::ShowDemoWindow(&show_demo);

            ImGui::Begin("PAIN Engine Debug");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Checkbox("Show Demo Window", &show_demo);

            auto& window = Application::Get().GetWindow();
            bool vsync = window.is_Vsync();
            if (ImGui::Checkbox("VSync", &vsync)) {
                window.set_Vsync(vsync);
            }

            ImGui::End();

            //Signal end of frame for imgui
            platform->endFrame();
        }

        //void Editor::updateShortCuts() {
        //    //Get IO
        //    ImGuiIO& io = ImGui::GetIO();

        //    //Undo
        //    static bool z_triggered = false;
        //    if (io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_Z)) {
        //        if (!z_triggered) {
        //            command_manager->undo();
        //            z_triggered = true;
        //        }
        //    }
        //    else {
        //        z_triggered = false;
        //    }

        //    //Redo
        //    static bool y_triggered = false;
        //    if (io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_Y)) {
        //        if (!y_triggered) {
        //            command_manager->redo();
        //            y_triggered = true;
        //        }
        //    }
        //    else {
        //        y_triggered = false;
        //    }
        //}

        void Editor::onEvent(Event::Event& event) {

            //Pass down events to platform for handling
            platform->handleEvents(event);

            //ImGuiIO& io = ImGui::GetIO();
            //if (event.isInCategory(Event::Keyboard) && io.WantCaptureKeyboard) {
            //    handleKeyEvents(io, event);
            //}
            //if (event.isInCategory(Event::Mouse) && io.WantCaptureMouse) {
            //    handleMouseEvents(io, event);
            //}
            //if (event.isInCategory(Event::Application)) {
            //    handleWindowEvents(io, event);
            //}
        }

        //void Editor::handleKeyEvents(ImGuiIO& io, Event::Event& event) {
        //    Event::Dispatcher dispatcher(event);
        //    dispatcher.Dispatch<Event::KeyTriggered>([&](Event::KeyTriggered& e) -> bool {
        //        io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {
        //        io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::KeyRepeated>([&](Event::KeyRepeated& e) -> bool {
        //        io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::KeyReleased>([&](Event::KeyReleased& e) -> bool {
        //        io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), false);
        //        return true;
        //        });
        //}

        //void Editor::handleMouseEvents(ImGuiIO& io, Event::Event& event) {
        //    Event::Dispatcher dispatcher(event);
        //    dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
        //        io.AddMouseButtonEvent(e.getBtnCode(), true);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {
        //        io.AddMouseButtonEvent(e.getBtnCode(), false);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {
        //        io.AddMousePosEvent(e.getWindowPos().x, e.getWindowPos().y);
        //        return false;
        //        });
        //    dispatcher.Dispatch<Event::MouseScrolled>([&](Event::MouseScrolled& e) -> bool {
        //        io.AddMouseWheelEvent(e.getOffset().x, e.getOffset().y);
        //        return false;
        //        });
        //}

        void Editor::BuildDockspace() {
            // ImGuiViewport* vp = ImGui::GetMainViewport();

            // // Reserve vertical space for the fixed Tools panel (menu + toolbar)
            // const float menu_h = ImGui::GetFrameHeight(); // same as Tools
            // const float toolbar_h = .2f;                   // same as Tools
            // const float tools_h = menu_h + toolbar_h;

            // // Position/size the dockspace host BELOW the tools bar
            // ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + tools_h));
            // ImGui::SetNextWindowSize(ImVec2(vp->Size.x, vp->Size.y - tools_h));
            // ImGui::SetNextWindowViewport(vp->ID);

            // ImGuiWindowFlags host_flags =
            //     ImGuiWindowFlags_NoDocking |
            //     ImGuiWindowFlags_NoTitleBar |
            //     ImGuiWindowFlags_NoCollapse |
            //     ImGuiWindowFlags_NoResize |
            //     ImGuiWindowFlags_NoMove |
            //     ImGuiWindowFlags_NoBringToFrontOnFocus |
            //     ImGuiWindowFlags_NoNavFocus |
            //     ImGuiWindowFlags_NoBackground;   // no menubar here; Tools owns the menu

            // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            // ImGui::Begin("##DockSpaceHost", nullptr, host_flags);

            // ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            // ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
            // ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f), dock_flags);

            // ImGui::End();
            // ImGui::PopStyleVar(2);
        }

        //void Editor::handleWindowEvents(ImGuiIO& io, Event::Event& event) {
        //    Event::Dispatcher dispatcher(event);
        //    dispatcher.Dispatch<Event::WindowResized>([&](Event::WindowResized& e) -> bool {
        //        io.DisplaySize.x = static_cast<float>(e.getFrameBuffer().x);
        //        io.DisplaySize.y = static_cast<float>(e.getFrameBuffer().y);
        //        return false;
        //        });
        //}
    }

}

#endif
