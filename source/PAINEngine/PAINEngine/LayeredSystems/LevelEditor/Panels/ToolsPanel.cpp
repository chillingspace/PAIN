#include "pch.h"
#include "ToolsPanel.h"

#ifdef _DEBUG

namespace PAIN {
	namespace Editor {
		namespace Panel {
			Tools::Tools(std::shared_ptr<CommandManager> command_manager) : IPanel(command_manager) {

				//Set panel name
				name = "##ToolsPanel";

				//Set panel flag
                flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_MenuBar;
			}

			void Tools::nextWindowSettings() {
				//Fullscreen dockspace (content sits under the bars)
				ImGuiViewport* vp = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(vp->Pos);
				ImGui::SetNextWindowSize(vp->Size);
				ImGui::SetNextWindowViewport(vp->ID);
			}

			void Tools::onUpdate() {
                //Begin menubar
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("File")) {
                        if (ImGui::MenuItem("New Scene", "Ctrl+N")) {/*TODO*/ }
                        if (ImGui::MenuItem("Open...", "Ctrl+O")) {/*TODO*/ }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Save", "Ctrl+S")) {/*TODO*/ }
                        if (ImGui::MenuItem("Save As...")) {/*TODO*/ }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Exit")) {/*TODO*/ }
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Edit")) {
                        ImGui::MenuItem("Undo", "Ctrl+Z");
                        ImGui::MenuItem("Redo", "Ctrl+Y");
                        ImGui::Separator();
                        ImGui::MenuItem("Preferences...");
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu("Assets")) { ImGui::MenuItem("Create"); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("GameObject")) { ImGui::MenuItem("Create Empty"); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Component")) { ImGui::MenuItem("Add..."); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Services")) { ImGui::MenuItem("Cloud"); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Window")) { ImGui::MenuItem("Layouts"); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Help")) { ImGui::MenuItem("About"); ImGui::EndMenu(); }
                    ImGui::EndMenuBar();
                }

                // Reserve space right under the main menu for the toolbar (#2)
                float menu_h = ImGui::GetFrameHeight();
                float toolbar_h = 34.0f;
                ImGui::SetCursorPosY(menu_h); // place toolbar directly below menu

                ImGuiWindowFlags toolbar_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
                    ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
                ImGui::BeginChild("##TopToolbar", ImVec2(0, toolbar_h), false, toolbar_flags);

                // Center: Play / Pause / Step
                {
                    ImGui::SameLine(0, 800);
                    if (ImGui::Button("Play")) {
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Pause")) {
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Stop")) {
                    }
                }

                ImGui::EndChild();
			}
		}
	}
}

#endif
