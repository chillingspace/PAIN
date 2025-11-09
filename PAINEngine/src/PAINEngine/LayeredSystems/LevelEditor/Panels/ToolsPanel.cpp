#include "pch.h"
#include "ToolsPanel.h"


#ifdef _DEBUG
#include "ECS/sMetaData.h"
#include "CoreSystems/Serialization/sSerialization.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {

#define PN_ECS_SERVICE services->get<ECS::Controller>()
#define PN_METADATA_SERVICE services->get<MetaData::Service>()
#define PN_SERI_SERVICE services->get<Serialization::Service>()
#define PN_PATH_SERVICE services->get<Path::Path>()

			Tools::Tools() {

				//Set panel name
				name = "##ToolsPanel";

				//Set panel flag
                flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
                    ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
                    ImGuiWindowFlags_MenuBar;

                registerPopUp("New Scene", createNewScenePopUp("New Scene"));
                registerPopUp("Save As...", saveAsPopUp("Save As..."));
			}
            
			void Tools::nextWindowSettings() {
				//Fullscreen dockspace (content sits under the bars)
				ImGuiViewport* vp = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(vp->Pos);
				ImGui::SetNextWindowSize(vp->Size);
				ImGui::SetNextWindowViewport(vp->ID);
			}

            std::function<void(std::any const&)> Tools::createNewScenePopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {

                    static char scene_name[128] = "";

                    ImGui::Text("Enter a new name for the scene:");
                    if (ImGui::InputText("##Scene Name", scene_name, sizeof(scene_name))) {
                    }

                    if (strlen(scene_name) > 0 && (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {

                        PN_SERI_SERVICE->saveSceneAs(scene_name);
                        PN_SERI_SERVICE->markSceneChanged();

                        scene_name[0] = '\0';
                        closePopUp(popup_id);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        scene_name[0] = '\0';
                        closePopUp(popup_id);
                    }

                    };
            }

            std::function<void(std::any const&)> Tools::saveAsPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {

                    static char scene_name[128] = "";

                    ImGui::Text("Enter a name for the new scene:");
                    if (ImGui::InputText("##Scene Name", scene_name, sizeof(scene_name))) {
                    }

                    if (strlen(scene_name) > 0 && (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {

                        PN_SERI_SERVICE->createNewScene(scene_name);

                        const std::string id = std::string{ scene_name } + ".scn";
                        PN_SERI_SERVICE->loadSceneById(id);

                        scene_name[0] = '\0';
                        closePopUp(popup_id);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        scene_name[0] = '\0';
                        closePopUp(popup_id);
                    }

                    };
            }

            void Tools::onAttach()
            {
            }

            void Tools::onUpdate(AppTiming timing) {
                //Begin menubar
                if (ImGui::BeginMenuBar())
                {
                    if (ImGui::BeginMenu("File")) {
                        if (ImGui::MenuItem("New Scene", "Ctrl+N")) { openPopUp("New Scene"); }
                        if (ImGui::MenuItem("Open Scene", "Ctrl+O")) {

#ifdef PN_PLATFORM_WINDOWS
                            std::string path = PN_SERI_SERVICE->OpenSceneFileDialog();
                            if (!path.empty()) {
                                std::string id = PN_SERI_SERVICE->getSceneId(path);
                                PN_SERI_SERVICE->loadSceneById(id);
                            }
#endif

                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Save", "Ctrl+S")) { PN_SERI_SERVICE->saveCurrentScene(); }
                        if (ImGui::MenuItem("Save As...")) { openPopUp("Save As..."); }
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
                    
                    if (ImGui::BeginMenu("GameObject")) { 
                        if (ImGui::MenuItem("Create Empty")){

                            Action create;
                            std::shared_ptr<std::string> shared_id = std::make_shared<std::string>("Empty");

                            create.do_action = [&, shared_id]() {
                                auto ecs = services->get<ECS::Controller>();
                                auto scene = services->get<Scene>();

                                glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f);
                                glm::quat rot = { 1.f, 0.f, 0.f, 0.f };
                                glm::vec3 scale = { 1.f, 1.f, 1.f };

                                entt::entity entity = ecs->createEntity();
                                ecs->addEntityComponent(entity, MetaData::EntityName{ *shared_id });
                                ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
                                //ecs->addEntityComponent(entity, Hierarchy{});
                                //if (scene) {
                                //    ecs->addEntityComponent(entity, MeshRenderer{ scene->getMeshId("") });
                                //}
                                };

                            create.undo_action = [&, shared_id]() {
                                auto entity = PN_METADATA_SERVICE->getEntityByName(*shared_id);
                                if (entity.has_value()) {
                                    PN_ECS_SERVICE->destroyEntity(entity.value());
                                }
                                };

                            command_manager->executeAction(std::move(create));
                        }

                        ImGui::EndMenu(); 
                    }

                    if (ImGui::BeginMenu("Component")) { ImGui::MenuItem("Add..."); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Services")) { ImGui::MenuItem("Cloud"); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Window")) { ImGui::MenuItem("Layouts"); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Help")) { ImGui::MenuItem("About"); ImGui::EndMenu(); }
                    // Display FPS on the right side of the menu bar
                    float fps = 1.0f / timing.dt;
                    float text_width = 150.0f; // Approximate width for the FPS text
                    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - text_width);
                    ImGui::Text("FPS: %.1f (%.2f ms)", fps, timing.dt * 1000.0f);
                    ImGui::EndMenuBar();
                }



                // Reserve space right under the main menu for the toolbar (#2)
                float menu_h = ImGui::GetFrameHeight();

                ImGui::SetCursorPosY(menu_h); // place toolbar directly below menu

                ImGuiWindowFlags toolbar_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
                    ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;
                ImGui::BeginChild("##TopToolbar", ImVec2(0, 20.f), false, toolbar_flags);

                //// Center: Play / Pause / Step
                //{
                //    ImGui::SameLine(0, 800);
                //    if (ImGui::Button("Play")) {
                //    }
                //    ImGui::SameLine();
                //    if (ImGui::Button("Pause")) {
                //    }
                //    ImGui::SameLine();
                //    if (ImGui::Button("Stop")) {
                //    }
                //}

                renderPopUps();

                ImGui::EndChild();
			}
		}
	}
}

#endif
