#include "pch.h"
#include "ToolsPanel.h"
#include "EntityPanel.h"
#include "ComponentsPanel.h"
#include "ECS/Components/AllComponents.h"
#include "../Editor.h"


#ifdef _DEBUG
#include "ECS/sMetaData.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "CoreSystems/Renderer/GraphicsSettings.h"
#include "CoreSystems/Windows/Window.h"

#include "ECS/Components/cEntity.h"

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
                registerPopUp("Settings", settingsPopUp("Settings"));
                registerPopUp("Unsaved Changes", unsavedChangesPopUp("Unsaved Changes"));
                registerPopUp("Unsaved Scene", unsavedScenePopUp("Unsaved Scene"));

                registerPopUp("Add Component", addComponentPopUp("Add Component"));
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

            std::function<void(std::any const&)> Tools::settingsPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
                         auto& gfx = GraphicsSettings::get();

                        // Shadow Type Dropdown
                        const char* shadow_types[] = { "Softest", "Soft", "Hard" };
                        int current_shadow = static_cast<int>(gfx.shadow_type);
                        if (ImGui::Combo("Shadow Type", &current_shadow, shadow_types, IM_ARRAYSIZE(shadow_types))) {
                            gfx.shadow_type = static_cast<GraphicsSettings::SHADOW_TYPES>(current_shadow);
                        }
                        ImGui::Text("Shadow Map Width: %d", gfx.getShadowMapWidth());

                        ImGui::Separator();

                        // Gamma Correction Checkbox
                        if (ImGui::Checkbox("Gamma Correction", &gfx.gamma_correction)) {}

                        // Ambient Light Color (using glm::vec3)
                        float ambient_light[3] = { gfx.AMBIENT_LIGHT.r, gfx.AMBIENT_LIGHT.g, gfx.AMBIENT_LIGHT.b };
                        if (ImGui::ColorEdit3("Ambient Light", ambient_light)) {
                            gfx.AMBIENT_LIGHT = glm::vec3(ambient_light[0], ambient_light[1], ambient_light[2]);
                        }

                        // Daytime Toggle
                        if (ImGui::Checkbox("Daytime", &gfx.world_light)) {}
                        
                        // Draw Floor
                        if (ImGui::Checkbox("Draw Floor", &gfx.draw_floor)) {}

                        // Field of View slider
                        if (ImGui::SliderFloat("FOV", &gfx.fov, 30.0f, 120.0f)) {}

                        ImGui::Separator();

                        // Ambient Occlusion toggle
                        if (ImGui::Checkbox("Ambient Occlusion", &gfx.DEBUG_USE_AO_MAP)) {}

                        // Blur Quality slider (integer)
                        if (ImGui::SliderInt("Blur Quality", &gfx.blur_quality, 2, 10)) {}

                        // Blur Strength slider
                        if (ImGui::SliderFloat("Blur Strength", &gfx.blur_strength, 0.0f, 10.0f)) {}

                        ImGui::Separator();

                        // Bloom Enable checkbox
                        if (ImGui::Checkbox("Bloom", &gfx.bloom)) {}

                        // Bloom threshold slider
                        if (ImGui::SliderFloat("Bloom Threshold", &gfx.bloom_threshold, 0.0f, 5.0f)) {}

                        // Bloom blur strength slider
                        if (ImGui::SliderFloat("Bloom Blur Strength", &gfx.bloom_blur_strength, 0.1f, 10.0f)) {}

                        // Bloom strength slider
                        if (ImGui::SliderFloat("Bloom Strength", &gfx.bloom_strength, 0.0f, 5.0f)) {}

                        // Bloom quality slider (integer)
                        if (ImGui::SliderInt("Bloom Quality", &gfx.bloom_quality, 2, 10)) {}

                        ImGui::Separator();

                        // Tone mapping mode Combo
                        const char* tone_mapping_modes[] = { "None", "ACES", "Reinhard", "Uncharted2" };
                        int current_tone = static_cast<int>(gfx.tone_mapping_mode);
                        if (ImGui::Combo("Tone Mapping", &current_tone, tone_mapping_modes, IM_ARRAYSIZE(tone_mapping_modes)))
                        {
                            gfx.tone_mapping_mode = static_cast<GraphicsSettings::TONE_MAPPING_TYPES>(current_tone);
                        }

                        // Tone mapping exposure slider
                        if (ImGui::SliderFloat("Tone Exposure", &gfx.tone_mapping_exposure, 0.0f, 5.0f)) {}

                        // Image based lighting toggle
                        if (ImGui::Checkbox("Image Based Lighting (IBL)", &gfx.ibl)) {}
                    
                        if (ImGui::Button("Close Settings")) {

                            closePopUp(popup_id);
                        }
                   
                };
            }

            std::function<void(std::any const&)> Tools::unsavedChangesPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {

                    auto ser = services->get<Serialization::Service>();
                    auto win = services->get<Window::Window>();

                    ImGui::Text("You have unsaved changes. Save before exit?");
                    if (ImGui::Button("Save and Exit")) {
                        ser->saveCurrentScene();
                        win->safeShutdown();

                        closePopUp(popup_id);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Exit Without Saving")) {
                        win->safeShutdown();

                        closePopUp(popup_id);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {

                        closePopUp(popup_id);
                    }
          
                };
            }

            std::function<void(std::any const&)> Tools::unsavedScenePopUp(std::string const& popup_id)
            {
                return [this, popup_id](std::any const& data) {

                    auto ser = services->get<Serialization::Service>();
                    auto win = services->get<Window::Window>();

                    ImGui::Text("Scene Not Saved! Save before exit?");
                    if (ImGui::Button("Save scene as (HAVEN'T IMPLEMENTED SAVE SCENE AS)")) {

                        closePopUp(popup_id);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Exit Without Saving")) {
                        win->safeShutdown();

                        closePopUp(popup_id);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {

                        closePopUp(popup_id);
                    }

                };
            }

            std::function<void(std::any const&)> Tools::addComponentPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
                    auto ecs = services->get<ECS::Controller>();
                    auto entity_panel = entities_panel.lock();
                    auto component_panel = components_panel.lock();

                    if (!entity_panel) {
                        auto editor = services->get<PAIN::Editor::Editor>();
                        if (editor) {
                            auto ep = editor->getPanel<Panel::EntityPanel>();
                            if (ep) {
                                entities_panel = ep;
                                entity_panel = ep;
                            }
                        }
                    }

                    if (!component_panel) {
                        auto editor = services->get<PAIN::Editor::Editor>();
                        if (editor) {
                            auto cp = editor->getPanel<Panel::ComponentsPanel>();
                            if (cp) {
                                components_panel = cp;
                                component_panel = cp;
                            }
                        }
                    }

                    if (!entity_panel) {
                        ImGui::Text("EntityPanel not available");
                        if (ImGui::Button("Close", ImVec2(-1, 0))) {
                            closePopUp(popup_id);
                        }
                        return;
                    }

                    entt::entity selected_entity = entity_panel->getSelectedEntity();

                    if (!ecs->checkEntity(selected_entity, component_panel->getCurrentRegistry())) {
                        ImGui::Text("No valid entity selected");
                        if (ImGui::Button("Close", ImVec2(-1, 0))) {
                            closePopUp(popup_id);
                        }
                        return;
                    }

                    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Add Component");
                    ImGui::Separator();
                    ImGui::Spacing();

                    static char search_filter[256] = "";
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputTextWithHint("##Search", "Search components...", search_filter, 256);
                    ImGui::Spacing();

                    ImGui::BeginChild("##ComponentList", ImVec2(400, 350), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    bool found_any = false;

                    // Iterate registered component factories
                    for (const auto& [comp_name, factory_func] : ecs->getComponentFactories()) {

                        if (comp_name == getComponentName<Entity::Name>() ||
                            comp_name == getComponentName<Entity::Layer>() ||
                            comp_name == getComponentName<MetaData::EditorVisible>()) {
                            continue;
                        }

                        // Skip if entity already has this component
                        if (ecs->hasComponentByName(selected_entity, comp_name, component_panel->getCurrentRegistry())) {
                            continue;
                        }

                        // Search filter
                        if (strlen(search_filter) > 0) {
                            std::string comp_lower = comp_name;
                            std::string search_lower = search_filter;
                            std::transform(comp_lower.begin(), comp_lower.end(), comp_lower.begin(), ::tolower);
                            std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);
                            if (comp_lower.find(search_lower) == std::string::npos) {
                                continue;
                            }
                        }

                        found_any = true;

                        if (ImGui::Selectable(comp_name.c_str(), false)) {
                            if (comp_name == "RigidBody3D") {
                                openPopUp("AddRigidBody3DConfig");
                                search_filter[0] = '\0';
                                closePopUp(popup_id);
                            }
                            else {
                                ecs->addComponentByName(selected_entity, comp_name, component_panel->getCurrentRegistry());
                                search_filter[0] = '\0';
                                closePopUp(popup_id);
                            }
                        }
                    }

                    if (!found_any) {
                        ImGui::TextDisabled("No available components");
                    }

                    ImGui::EndChild();
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
                        search_filter[0] = '\0';
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
                        if (ImGui::MenuItem("Settings")) { openPopUp("Settings"); }
                        if (ImGui::MenuItem("Exit")) {
                        
                            auto win = services->get<Window::Window>();
                            win->safeShutdown();

                            // TO DO:
                            //unsavedChangesPopUp("Unsaved Changes");
                            //unsavedScenePopUp("Unsaved Scene");
                        }
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
                                auto scene = services->get<Scene::SceneManager>();

                                glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f);
                                glm::quat rot = { 1.f, 0.f, 0.f, 0.f };
                                glm::vec3 scale = { 1.f, 1.f, 1.f };

                                entt::entity entity = ecs->createEntity();
                                ecs->addEntityComponent(entity, Entity::Name{ *shared_id });
                                ecs->addEntityComponent(entity, LocalTransform{ pos, rot, scale });
                                ecs->addEntityComponent(entity, WorldTransform{});
                                ecs->addEntityComponent(entity, Entity::Hierarchy{});
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

                    if (ImGui::BeginMenu("Component")) { 

                        if (ImGui::MenuItem("Add...")) {
                            openPopUp("Add Component");
                        }
                        ImGui::EndMenu(); 
                    }

                    /*if (ImGui::BeginMenu("Services")) { ImGui::MenuItem("Cloud"); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Window")) { ImGui::MenuItem("Layouts"); ImGui::EndMenu(); }
                    if (ImGui::BeginMenu("Help")) { ImGui::MenuItem("About"); ImGui::EndMenu(); }*/

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
            void Tools::onEvent(Event::Event& event) {
#ifdef PN_PLATFORM_WINDOWS
                // Handle closing of application tool panel
                Event::Dispatcher dispatcher(event);

                dispatcher.Dispatch<Event::WindowClosed>([&](Event::WindowClosed& e) -> bool {
                    auto ser = services->get<PAIN::Serialization::Service>();

                    closeAllPopUps();

                    // Is modified scene doesnt work currently
                    if (ser->getIsModifiedScene()) {
                        openPopUp("Unsaved Changes");
                        return true; // handled
                    }
                    else if (ser->getIsCurSceneEmpty()) {
                        /*showCloseNoScenePopup = true;*/
                        openPopUp("Unsaved Scene");
                        return true;
                    }


                    auto win = services->get<Window::Window>();
                    win->safeShutdown();
                    return true;

                    });
#endif
		}

        }
	}
}

#endif
