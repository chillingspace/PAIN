#include "pch.h"
#include "Editor.h"

#ifdef _DEBUG
#include <GLFW/glfw3.h>

#include "CoreSystems/Events/WindowEvents.h"
#include "CoreSystems/Events/KeyEvents.h"
#include "CoreSystems/Events/MouseEvents.h"
#include "CoreSystems/Events/AssetEvents.h"

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

            // Load imgui settings
            ImGui::LoadIniSettingsFromDisk(io.IniFilename);
        }

        void Editor::onDetach() {
            //Save imgui layouts
            ImGuiIO& io = ImGui::GetIO();
            ImGui::SaveIniSettingsToDisk(io.IniFilename);

            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplGlfw_Shutdown();
            ImGui::DestroyContext();
        }

        void Editor::onUpdate() {

            BeginFrame();

            static bool show_demo = true;
            if (show_demo) ImGui::ShowDemoWindow(&show_demo);

            ImGui::Begin("PAIN Engine Debug");
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::Checkbox("Show Demo Window", &show_demo);
            ImGui::End();

            // Placeholder For Audio (no FMOD yet)
            ImGui::Begin("Audio Controls (Placeholder)");

            static char soundId[256] = "";         
            static float volume = 1.0f;
            static bool loop = false;
            static bool isPlaying = false;

            ImGui::TextDisabled("No audio backend integrated yet (FMOD pending).");
            ImGui::Separator();

            ImGui::InputText("Sound", soundId, IM_ARRAYSIZE(soundId));
            ImGui::SameLine();
            if (ImGui::Button("Clear")) {
                soundId[0] = '\0';
            }

            ImGui::SliderFloat("Volume", &volume, 0.0f, 1.0f, "%.2f");
            ImGui::Checkbox("Loop", &loop);

            ImGui::Separator();
            if (ImGui::Button("Play")) {

                // trigger FMOD play for `soundId` with `volume` & `loop`.
                isPlaying = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Pause")) {

                // pause the active sound/instance.
                isPlaying = false;
            }

            ImGui::Separator();
            ImGui::Text("Status: %s", isPlaying ? "Playing (simulated)" : "Paused/Stopped");
            ImGui::Text("Selected: %s", soundId[0] ? soundId : "<none>");
            ImGui::End();
            // --- End placeholder ---


            EndFrame();
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

            //Get imgui input
            ImGuiIO& io = ImGui::GetIO();

            //Check imgui polling for keyboard event
            if (event.isInCategory(Event::Keyboard)) {

                //Check if imgui wants to capture keyboard events
                if (io.WantCaptureKeyboard) {

                    //Handle keyboard events
                    handleKeyEvents(io, event);
                }
            }

            //Check imgui polling for mouse events
            if (event.isInCategory(Event::Mouse)) {

                //Check if imgui wants to capture mouse events
                if (io.WantCaptureMouse) {

                    //Handle mouse events
                    handleMouseEvents(io, event);
                }
            }

            //Check imgui polling for mouse events
            if (event.isInCategory(Event::Application)) {

                //Handle mouse events
                handleWindowEvents(io, event);
            }
        }

        void Editor::handleKeyEvents(ImGuiIO& io, Event::Event& event) {

            //Create event dispatcher
            Event::Dispatcher dispatcher(event);

            //Dispatch key triggered event
            dispatcher.Dispatch<Event::KeyTriggered>([&](Event::KeyTriggered& e) -> bool {

                //Update imgui
                io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);

                //Return false: continue dispatching, true = stop dispatching 
                return true;
                });

            //Dispatch key press event
            dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {

                //Update imgui
                io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);

                //Return false: continue dispatching, true = stop dispatching 
                return true;
                });

            //Dispatch key repeated event
            dispatcher.Dispatch<Event::KeyRepeated>([&](Event::KeyRepeated& e) -> bool {

                //Update imgui
                io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);

                //Return false: continue dispatching, true = stop dispatching 
                return true;
                });

            //Dispatch key released event
            dispatcher.Dispatch<Event::KeyReleased>([&](Event::KeyReleased& e) -> bool {

                //Update imgui
                io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), false);

                //Return false: continue dispatching, true = stop dispatching 
                return true;
                });
        }

        void Editor::handleMouseEvents(ImGuiIO& io, Event::Event& event) {
            //Create event dispatcher
            Event::Dispatcher dispatcher(event);

            //Dispatch Mouse pressed event
            dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {

                //Update imgui
                io.AddMouseButtonEvent(e.getBtnCode(), true);

                //Return false: continue dispatching, true = stop dispatching 
                return true;
                });

            //Dispatch Mouse released event
            dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {

                //Update imgui
                io.AddMouseButtonEvent(e.getBtnCode(), false);

                //Return false: continue dispatching, true = stop dispatching 
                return true;
                });

            //Dispatch Mouse released event
            dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {

                //Update imgui mouse position
                io.AddMousePosEvent(e.getWindowPos().x, e.getWindowPos().y);

                //Return false: continue dispatching, true = stop dispatching 
                return false;
                });

            //Dispatch Mouse scroll event
            dispatcher.Dispatch<Event::MouseScrolled>([&](Event::MouseScrolled& e) -> bool {

                //Update scroll offset
                io.AddMouseWheelEvent(e.getOffset().x, e.getOffset().y);

                //Return false: continue dispatching, true = stop dispatching 
                return false;
                });
        }

        void Editor::handleWindowEvents(ImGuiIO& io, Event::Event& event) {

            //Create event dispatcher
            Event::Dispatcher dispatcher(event);

            //Dispatch key triggered event
            dispatcher.Dispatch<Event::WindowResized>([&](Event::WindowResized& e) -> bool {

                //Update imgui that windows is resized
                io.DisplaySize.x = static_cast<float>(e.getFrameBuffer().x);
                io.DisplaySize.y = static_cast<float>(e.getFrameBuffer().y);

                //Return false: continue dispatching, true = stop dispatching 
                return false;
                });
        }
    }

} // namespace PAIN

#endif
