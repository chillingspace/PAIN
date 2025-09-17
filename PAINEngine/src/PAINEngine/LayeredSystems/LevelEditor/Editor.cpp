#include "pch.h"
#include "Editor.h"

#ifdef _DEBUG
#include <GLFW/glfw3.h>

#include "CoreSystems/Events/WindowEvents.h"
#include "CoreSystems/Events/KeyEvents.h"
#include "CoreSystems/Events/MouseEvents.h"
#include "CoreSystems/Events/AssetEvents.h"
#include "PAINEngine/Audio/AudioManager.h"

namespace PAIN {

    namespace Editor {

        int Editor::imguiKeyMapping(int code) {

            //Code within 0 - 9 range
            if (code >= GLFW_KEY_0 && code <= GLFW_KEY_9) {
                return ImGuiKey_0 + (code - GLFW_KEY_0);
            }
            //Code within A - Z range
            else if (code >= GLFW_KEY_A && code <= GLFW_KEY_Z) {
                return ImGuiKey_A + (code - GLFW_KEY_A);
            }
            //Code within F1 - F24 range
            else if (code >= GLFW_KEY_F1 && code <= GLFW_KEY_F24) {
                return ImGuiKey_F1 + (code - GLFW_KEY_F1);
            }
            //Code within KeyPad_0 - KeyPad_Equal range
            else if (code >= GLFW_KEY_KP_0 && code <= GLFW_KEY_KP_EQUAL) {
                return ImGuiKey_Keypad0 + (code - GLFW_KEY_KP_0);
            }
            //Code within Apostrophe - Slash range
            else if (code >= GLFW_KEY_APOSTROPHE && code <= GLFW_KEY_SLASH) {
                return ImGuiKey_Apostrophe + (code - GLFW_KEY_APOSTROPHE);
            }
            //Code within left bracket & grave accent
            else if (code >= GLFW_KEY_LEFT_BRACKET && code <= GLFW_KEY_GRAVE_ACCENT) {
                return ImGuiKey_LeftBracket + (code - GLFW_KEY_LEFT_BRACKET);
            }
            //Code within caps lock & pause
            else if (code >= GLFW_KEY_CAPS_LOCK && code <= GLFW_KEY_PAUSE) {
                return ImGuiKey_CapsLock + (code - GLFW_KEY_CAPS_LOCK);
            }
            //Code within caps lock & pause
            else if (code >= GLFW_KEY_PAGE_UP && code <= GLFW_KEY_END) {
                return ImGuiKey_PageUp + (code - GLFW_KEY_PAGE_UP);
            }

            //Mouse button codes
            switch (code) {
            case GLFW_MOUSE_BUTTON_LEFT: return ImGuiKey_MouseLeft;
            case GLFW_MOUSE_BUTTON_RIGHT: return ImGuiKey_MouseRight;
            case GLFW_MOUSE_BUTTON_MIDDLE: return ImGuiKey_MouseMiddle;
            case GLFW_MOUSE_BUTTON_4: return ImGuiKey_MouseX1;
            case GLFW_MOUSE_BUTTON_5: return ImGuiKey_MouseX2;
            }

            //Remaing key codes that are not in order
            switch (code) {
            case GLFW_KEY_SPACE: return ImGuiKey_Space;
            case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
            case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
            case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
            case GLFW_KEY_ENTER: return ImGuiKey_Enter;
            case GLFW_KEY_TAB: return ImGuiKey_Tab;
            case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
            case GLFW_KEY_INSERT: return ImGuiKey_Insert;
            case GLFW_KEY_DELETE: return ImGuiKey_Delete;
            case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
            case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
            case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
            case GLFW_KEY_UP: return ImGuiKey_UpArrow;
            case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
            case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
            case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
            case GLFW_KEY_MENU: return ImGuiKey_Menu;
            }

            return ImGuiKey_None;
        }

        Editor::Editor() {
        }

        Editor::~Editor() {}

        void Editor::onAttach() {

            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;

            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            ImGui::StyleColorsDark();

            ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
            ImGui_ImplOpenGL3_Init("#version 450");

            ImGui::LoadIniSettingsFromDisk(io.IniFilename);

            //Construct command manager
            command_manager = std::make_shared<CommandManager>();

            //Register panels
            //panels[CLASS_STR(IPanel)] = std::make_shared<IPanel>();
            //PN_CORE_INFO(panels[CLASS_STR(IPanel)]->getPanelName());
        }

        void Editor::onDetach() {
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SaveIniSettingsToDisk(io.IniFilename);

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }

        void Editor::onUpdate() {

            //Update shortcuts
            updateShortCuts();

            //Begin IMGUI Frame
            BeginFrame();

            //Update all panels
            for (auto const& panel : panels) {
                panel.second->drawWindow();
            }

            static bool show_demo = true;
            if (show_demo) ImGui::ShowDemoWindow(&show_demo);

            ImGui::Begin("PAIN Engine Debug");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Checkbox("Show Demo Window", &show_demo);
            ImGui::End();

            // Functional Audio Controls
            ImGui::Begin("Audio Controls");
            AudioManager& audio = Application::Get().GetAudioManager();

            static char soundPath[256] = "assets/audio/SFX/MovingSFX/Footstep_Metal_01.wav";
            static float volume = 0.0f;
            static bool loop = false;
            static bool is3D = true;
            static float posX = 0.0f, posY = 0.0f, posZ = 0.0f;

            ImGui::InputText("Sound Path", soundPath, IM_ARRAYSIZE(soundPath));
            ImGui::SliderFloat("Volume (dB)", &volume, -80.0f, 10.0f, "%.2f");
            ImGui::Checkbox("Loop", &loop);
            ImGui::Checkbox("3D", &is3D);

            if (is3D) {
                ImGui::SliderFloat("X", &posX, -10.0f, 10.0f);
                ImGui::SliderFloat("Y", &posY, -10.0f, 10.0f);
                ImGui::SliderFloat("Z", &posZ, -10.0f, 10.0f);
            }

            if (ImGui::Button("Load Sound")) {
                audio.LoadSound(soundPath, is3D, loop);
            }

            ImGui::SameLine();

            if (ImGui::Button("Play Sound")) {
                audio.PlaySound(soundPath, { posX, posY, posZ }, volume);
            }

            ImGui::Separator();
            ImGui::Text("Playlist Controls");
            if (ImGui::Button("Play Random Footstep"))
            {
                audio.PlayRandomFromPlaylist("FootstepsGrass", { posX, posY, posZ }, volume);
            }

            ImGui::Separator();
            if (ImGui::Button("Stop All Sounds")) {
                audio.StopAllChannels();
            }

            ImGui::End();

            EndFrame();
        }

        void Editor::updateShortCuts() {
            //Get IO
            ImGuiIO& io = ImGui::GetIO();

            //Undo
            static bool z_triggered = false;
            if (io.KeyCtrl && io.KeysDown[ImGuiKey_Z]) {
                if (!z_triggered) {
                    command_manager->undo();
                    z_triggered = true;
                }
            }
            else {
                z_triggered = false;
            }

            //Redo
            static bool y_triggered = false;
            if (io.KeyCtrl && io.KeysDown[ImGuiKey_Y]) {
                if (!y_triggered) {
                    command_manager->redo();
                    y_triggered = true;
                }
            }
            else {
                y_triggered = false;
            }
        }

        void Editor::BeginFrame() {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
        }

        void Editor::EndFrame() {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }

        void Editor::onEvent(Event::Event& event) {

            ImGuiIO& io = ImGui::GetIO();
            if (event.isInCategory(Event::Keyboard) && io.WantCaptureKeyboard) {
                handleKeyEvents(io, event);
            }
            if (event.isInCategory(Event::Mouse) && io.WantCaptureMouse) {
                handleMouseEvents(io, event);
            }
            if (event.isInCategory(Event::Application)) {
                handleWindowEvents(io, event);
            }
        }

        void Editor::handleKeyEvents(ImGuiIO& io, Event::Event& event) {
            Event::Dispatcher dispatcher(event);
            dispatcher.Dispatch<Event::KeyTriggered>([&](Event::KeyTriggered& e) -> bool {
                io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
                return true;
                });
            dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {
                io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
                return true;
                });
            dispatcher.Dispatch<Event::KeyRepeated>([&](Event::KeyRepeated& e) -> bool {
                io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
                return true;
                });
            dispatcher.Dispatch<Event::KeyReleased>([&](Event::KeyReleased& e) -> bool {
                io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), false);
                return true;
                });
        }

        void Editor::handleMouseEvents(ImGuiIO& io, Event::Event& event) {
            Event::Dispatcher dispatcher(event);
            dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
                io.AddMouseButtonEvent(e.getBtnCode(), true);
                return true;
                });
            dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {
                io.AddMouseButtonEvent(e.getBtnCode(), false);
                return true;
                });
            dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {
                io.AddMousePosEvent(e.getWindowPos().x, e.getWindowPos().y);
                return false;
                });
            dispatcher.Dispatch<Event::MouseScrolled>([&](Event::MouseScrolled& e) -> bool {
                io.AddMouseWheelEvent(e.getOffset().x, e.getOffset().y);
                return false;
                });
        }

        void Editor::handleWindowEvents(ImGuiIO& io, Event::Event& event) {
            Event::Dispatcher dispatcher(event);
            dispatcher.Dispatch<Event::WindowResized>([&](Event::WindowResized& e) -> bool {
                io.DisplaySize.x = static_cast<float>(e.getFrameBuffer().x);
                io.DisplaySize.y = static_cast<float>(e.getFrameBuffer().y);
                return false;
                });
        }
    }

}
#endif