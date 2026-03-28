#ifdef PN_PLATFORM_WINDOWS
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
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Components/cEntity.h"
#include "Systems/Transform/sysTransform.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {

#define PN_ECS_SERVICE      services->get<ECS::Controller>()
#define PN_METADATA_SERVICE services->get<MetaData::Service>()
#define PN_SERI_SERVICE     services->get<Serialization::Service>()
#define PN_PATH_SERVICE     services->get<Path::Path>()
#define PN_SCENE_SERVICE    services->get<Scene::SceneManager>()
#define PN_ASSET_SERVICE    services->get<Assets::Manager>()

			Tools::Tools() {
				// Set panel name
				name = "##ToolsPanel";

				// Set panel flags
				flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
					ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground |
					ImGuiWindowFlags_MenuBar;

				registerPopUp("New Scene", createNewScenePopUp("New Scene"));
				registerPopUp("Load Scene", loadScenePopUp("Load Scene"));
				registerPopUp("Save As...", saveAsPopUp("Save As..."));
				registerPopUp("Settings", settingsPopUp("Settings"));
				registerPopUp("Unsaved Changes", unsavedChangesPopUp("Unsaved Changes"));
				registerPopUp("Unsaved Scene", unsavedScenePopUp("Unsaved Scene"));
				registerPopUp("Create Entity", createEmptyPopUp("Create Entity"));
				registerPopUp("Add Component", addComponentPopUp("Add Component"));
				registerPopUp("Info", defPopUp("Info"));
			}

			void Tools::nextWindowSettings() {
				ImGuiViewport* vp = ImGui::GetMainViewport();
				ImGui::SetNextWindowPos(vp->Pos);
				ImGui::SetNextWindowSize(vp->Size);
				ImGui::SetNextWindowViewport(vp->ID);
			}

			// ----------------------------------------------------------------
			// New Scene
			// ----------------------------------------------------------------
			std::function<void(std::any const&)> Tools::createNewScenePopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					static char scene_name[128] = "";

					ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Create New Scene");
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::Text("Scene Name:");
					ImGui::SetNextItemWidth(250);
					ImGui::InputText("##NewSceneName", scene_name, sizeof(scene_name));

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					const bool valid = strlen(scene_name) > 0;

					float button_width = 120.0f;
					float spacing = ImGui::GetStyle().ItemSpacing.x;
					float total = (button_width * 2) + spacing;
					float offset = (ImGui::GetContentRegionAvail().x - total) * 0.5f;
					if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

					ImGui::BeginDisabled(!valid);
					if (ImGui::Button("Create", ImVec2(button_width, 0)) ||
						(valid && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
						PN_SCENE_SERVICE->createScene(std::string(scene_name));
						scene_name[0] = '\0';
						closePopUp(popup_id);
					}
					ImGui::EndDisabled();

					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(button_width, 0))) {
						scene_name[0] = '\0';
						closePopUp(popup_id);
					}
					};
			}

			// ----------------------------------------------------------------
			// Load Scene
			// ----------------------------------------------------------------
			std::function<void(std::any const&)> Tools::loadScenePopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					auto asset_service = PN_ASSET_SERVICE;
					auto scn_service = PN_SCENE_SERVICE;

					ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), "Load Scene");
					ImGui::Separator();
					ImGui::Spacing();

					if (!asset_service || !scn_service) {
						ImGui::TextDisabled("Services unavailable");
						return;
					}

					auto scenes = asset_service->getAllAssetDataOfType(Assets::Type::Scenes);
					Assets::GUID currID = scn_service->getCurrScnID();

					static int          selected_idx = -1;
					static Assets::GUID selected_guid = {};

					if (scenes.empty()) {
						ImGui::TextDisabled("No scenes available");
					}
					else {
						ImGui::BeginChild("##SceneList", ImVec2(300, 200), true);

						for (int i = 0; i < static_cast<int>(scenes.size()); ++i) {
							// Strip .scn extension for display
							std::string display = scenes[i]->name;
							const auto  dot = display.rfind('.');
							if (dot != std::string::npos) display = display.substr(0, dot);

							const bool isCurrent = (scenes[i]->guid == currID);
							const bool isSelected = (selected_idx == i);

							std::string label = isCurrent ? (display + "  [active]") : display;

							if (ImGui::Selectable(label.c_str(), isSelected)) {
								selected_idx = i;
								selected_guid = scenes[i]->guid;
							}
						}

						ImGui::EndChild();
					}

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					const bool canLoad = selected_idx >= 0 &&
						selected_guid.IsValid() &&
						selected_guid != currID;

					float button_width = 120.0f;
					float spacing = ImGui::GetStyle().ItemSpacing.x;
					float total = (button_width * 2) + spacing;
					float offset = (ImGui::GetContentRegionAvail().x - total) * 0.5f;
					if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

					ImGui::BeginDisabled(!canLoad);
					if (ImGui::Button("Load", ImVec2(button_width, 0))) {
						scn_service->loadScene(selected_guid);
						selected_idx = -1;
						selected_guid = {};
						closePopUp(popup_id);
					}
					ImGui::EndDisabled();

					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(button_width, 0))) {
						selected_idx = -1;
						selected_guid = {};
						closePopUp(popup_id);
					}
					};
			}

			// ----------------------------------------------------------------
			// Save As
			// ----------------------------------------------------------------
			std::function<void(std::any const&)> Tools::saveAsPopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					static char scene_name[128] = "";

					ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Save Scene As");
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::Text("New Scene Name:");
					ImGui::SetNextItemWidth(250);
					ImGui::InputText("##SaveAsName", scene_name, sizeof(scene_name));

					ImGui::Spacing();
					ImGui::TextWrapped("This will create a new scene file with the specified name.");
					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					const bool valid = strlen(scene_name) > 0;

					float button_width = 120.0f;
					float spacing = ImGui::GetStyle().ItemSpacing.x;
					float total = (button_width * 2) + spacing;
					float offset = (ImGui::GetContentRegionAvail().x - total) * 0.5f;
					if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

					ImGui::BeginDisabled(!valid);
					if (ImGui::Button("Save As", ImVec2(button_width, 0)) ||
						(valid && ImGui::IsKeyPressed(ImGuiKey_Enter))) {
						auto scn = PN_SCENE_SERVICE;
						scn->saveActiveScene(scn->getCurrScnID(), std::string(scene_name));
						scene_name[0] = '\0';
						closePopUp(popup_id);
						openPopUp("Info", std::make_shared<std::string>("Scene Saved!")); // <-- ADDED
					}
					ImGui::EndDisabled();

					ImGui::SameLine();
					if (ImGui::Button("Cancel", ImVec2(button_width, 0))) {
						scene_name[0] = '\0';
						closePopUp(popup_id);
					}
					};
			}

			// ----------------------------------------------------------------
			// Settings
			// ----------------------------------------------------------------
			std::function<void(std::any const&)> Tools::settingsPopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					auto& gfx = GraphicsSettings::get();

					// Shadow Type
					const char* shadow_types[] = { "Softest", "Soft", "Hard" };
					int current_shadow = static_cast<int>(gfx.shadow_type);
					if (ImGui::Combo("Shadow Type", &current_shadow, shadow_types, IM_ARRAYSIZE(shadow_types))) {
						gfx.shadow_type = static_cast<GraphicsSettings::SHADOW_TYPES>(current_shadow);
					}
					ImGui::Text("Shadow Map Width: %d", gfx.getShadowMapWidth());
					ImGui::Separator();

					if (ImGui::Checkbox("Gamma Correction", &gfx.gamma_correction)) {}
					if (gfx.gamma_correction) {
						ImGui::SliderFloat("Gamma", &gfx.gamma_value, 1.0f, 3.0f);
					}

					float ambient_light[3] = { gfx.AMBIENT_LIGHT.r, gfx.AMBIENT_LIGHT.g, gfx.AMBIENT_LIGHT.b };
					if (ImGui::ColorEdit3("Ambient Light", ambient_light)) {
						gfx.AMBIENT_LIGHT = glm::vec3(ambient_light[0], ambient_light[1], ambient_light[2]);
					}

					if (ImGui::Checkbox("Daytime", &gfx.world_light)) {}
					if (ImGui::Checkbox("Draw Floor", &gfx.draw_floor)) {}
					if (ImGui::SliderFloat("FOV", &gfx.fov, 30.0f, 120.0f)) {}
					ImGui::Separator();

					if (ImGui::Checkbox("Ambient Occlusion", &gfx.DEBUG_USE_AO_MAP)) {}
					if (ImGui::SliderInt("Blur Quality", &gfx.blur_quality, 2, 10)) {}
					if (ImGui::SliderFloat("Blur Strength", &gfx.blur_strength, 0.0f, 10.0f)) {}
					ImGui::Separator();

					if (ImGui::Checkbox("Bloom", &gfx.bloom)) {}
					if (ImGui::SliderFloat("Bloom Threshold", &gfx.bloom_threshold, 0.0f, 5.0f)) {}
					if (ImGui::SliderFloat("Bloom Blur Strength", &gfx.bloom_blur_strength, 0.1f, 10.0f)) {}
					if (ImGui::SliderFloat("Bloom Strength", &gfx.bloom_strength, 0.0f, 5.0f)) {}
					if (ImGui::SliderInt("Bloom Quality", &gfx.bloom_quality, 2, 10)) {}
					ImGui::Separator();

					const char* tone_mapping_modes[] = { "None", "ACES", "Reinhard", "Uncharted2" };
					int current_tone = static_cast<int>(gfx.tone_mapping_mode);
					if (ImGui::Combo("Tone Mapping", &current_tone, tone_mapping_modes, IM_ARRAYSIZE(tone_mapping_modes))) {
						gfx.tone_mapping_mode = static_cast<GraphicsSettings::TONE_MAPPING_TYPES>(current_tone);
					}
					if (ImGui::SliderFloat("Tone Exposure", &gfx.tone_mapping_exposure, 0.0f, 5.0f)) {}
					if (ImGui::Checkbox("Image Based Lighting (IBL)", &gfx.ibl)) {}
					if (gfx.ibl) {
						if (ImGui::SliderFloat("IBL Diffuse", &gfx.ibl_diffuse_strength, 0.0f, 2.0f)) {}
						if (ImGui::SliderFloat("IBL Specular", &gfx.ibl_specular_strength, 0.0f, 2.0f)) {}
						if (ImGui::SliderFloat("IBL Max Reflection LOD", &gfx.ibl_max_reflection_lod, 0.0f, 4.0f)) {}
					}

					ImGui::Spacing();
					if (ImGui::Button("Close Settings")) {
						closePopUp(popup_id);
					}
					};
			}

			// ----------------------------------------------------------------
			// Unsaved Changes (on exit)
			// ----------------------------------------------------------------
			std::function<void(std::any const&)> Tools::unsavedChangesPopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					auto ser = PN_SERI_SERVICE;
					auto win = services->get<Window::Window>();

					ImGui::Text("You have unsaved changes. Save before exit?");
					ImGui::Spacing();

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

			// ----------------------------------------------------------------
			// Unsaved Scene (on exit with no saved scene)
			// ----------------------------------------------------------------
			std::function<void(std::any const&)> Tools::unsavedScenePopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					auto win = services->get<Window::Window>();

					ImGui::Text("Scene not saved. Exit without saving?");
					ImGui::Spacing();

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

			// ----------------------------------------------------------------
			// Add Component
			// ----------------------------------------------------------------
			std::function<void(std::any const&)> Tools::addComponentPopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					auto ecs = PN_ECS_SERVICE;
					auto entity_panel = entities_panel.lock();
					auto component_panel = components_panel.lock();

					if (!entity_panel) {
						auto editor = services->get<PAIN::Editor::Editor>();
						if (editor) {
							auto ep = editor->getPanel<Panel::EntityPanel>();
							if (ep) { entities_panel = ep; entity_panel = ep; }
						}
					}

					if (!component_panel) {
						auto editor = services->get<PAIN::Editor::Editor>();
						if (editor) {
							auto cp = editor->getPanel<Panel::ComponentsPanel>();
							if (cp) { components_panel = cp; component_panel = cp; }
						}
					}

					if (!entity_panel) {
						ImGui::Text("EntityPanel not available");
						if (ImGui::Button("Close", ImVec2(-1, 0))) closePopUp(popup_id);
						return;
					}

					entt::entity selected_entity = entity_panel->getSelectedEntity();

					if (!ecs->checkEntity(selected_entity, component_panel->getCurrentRegistry())) {
						ImGui::Text("No valid entity selected");
						if (ImGui::Button("Close", ImVec2(-1, 0))) closePopUp(popup_id);
						return;
					}

					ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Add Component");
					ImGui::Separator();
					ImGui::Spacing();

					static char search_filter[256] = "";
					ImGui::SetNextItemWidth(-1);
					ImGui::InputTextWithHint("##Search", "Search components...", search_filter, 256);
					ImGui::Spacing();

					ImGui::BeginChild("##ComponentList", ImVec2(400, 350), true,
						ImGuiWindowFlags_AlwaysVerticalScrollbar);

					bool found_any = false;

					for (const auto& [comp_name, factory_func] : ecs->getComponentFactories()) {
						if (comp_name == getComponentName<Entity::Name>() ||
							comp_name == getComponentName<Entity::Layer>() ||
							comp_name == getComponentName<MetaData::EditorVisible>()) {
							continue;
						}

						if (ecs->hasComponentByName(selected_entity, comp_name,
							component_panel->getCurrentRegistry())) {
							continue;
						}

						if (strlen(search_filter) > 0) {
							std::string comp_lower = comp_name;
							std::string search_lower = search_filter;
							std::transform(comp_lower.begin(), comp_lower.end(),
								comp_lower.begin(), ::tolower);
							std::transform(search_lower.begin(), search_lower.end(),
								search_lower.begin(), ::tolower);
							if (comp_lower.find(search_lower) == std::string::npos) continue;
						}

						found_any = true;

						if (ImGui::Selectable(comp_name.c_str(), false)) {
							if (comp_name == "RigidBody3D") {
								openPopUp("AddRigidBody3DConfig");
							}
							else {
								ecs->addComponentByName(selected_entity, comp_name,
									component_panel->getCurrentRegistry());
							}
							search_filter[0] = '\0';
							closePopUp(popup_id);
						}
					}

					if (!found_any) ImGui::TextDisabled("No available components");

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

			// ----------------------------------------------------------------
			// Create Entity
			// ----------------------------------------------------------------

			std::function<void(std::any const&)> Tools::createEmptyPopUp(std::string const& popup_id)
			{
				return [this, popup_id](std::any const& data) {
					static char entity_name[128] = "";

					ImGui::Text("Enter a name for the new entity:");
					ImGui::InputText("##Entity Name", entity_name, sizeof(entity_name));


					if (strlen(entity_name) > 0 && (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
						std::string final_name = std::string(entity_name);

						command_manager->executeAction(Action{
							[this, final_name]() {
								auto ecs = PN_ECS_SERVICE;
								auto scene = services->get<Scene::SceneManager>();
								ECS::RegistryID currentRegistryID = ECS::MAIN_REGISTRY_ID;

								entt::entity entity = ecs->createEntity(currentRegistryID); // Auto-assigns GUID

								// Add core components
								ecs->addEntityComponent(entity, Entity::Name{ final_name }, currentRegistryID);
								ecs->addEntityComponent(entity, LocalTransform{}, currentRegistryID);
								ecs->addEntityComponent(entity, WorldTransform{}, currentRegistryID);

								// Mark transform dirty
								auto transformSystem = ecs->getSystem<Transform::System>();
								if (transformSystem) {
									transformSystem->markDirty(entity, ecs->getRegistry(currentRegistryID));
								}
							},
							[this, final_name]() {
								ECS::RegistryID currentRegistryID = ECS::MAIN_REGISTRY_ID;
								// Undo: find and delete by name
								auto ecs = PN_ECS_SERVICE;
								auto& registry = ecs->getRegistry(currentRegistryID);
								auto view = registry.view<Entity::Name>();

								for (auto entity : view) {
									if (view.get<Entity::Name>(entity).name == final_name) {
										ecs->destroyEntity(entity, currentRegistryID);
										break;
									}
								}
							},
							"Create Entity: " + final_name
							});

						entity_name[0] = '\0';
						closePopUp(popup_id);
					}

					ImGui::SameLine();
					if (ImGui::Button("Cancel")) {
						entity_name[0] = '\0';
						closePopUp(popup_id);
					}
					};
			}


			// ----------------------------------------------------------------
			// onAttach
			// ----------------------------------------------------------------
			void Tools::onAttach() {}

			// ----------------------------------------------------------------
			// onUpdate - menu bar + toolbar
			// ----------------------------------------------------------------
			void Tools::onUpdate(AppTiming timing) {

				// ---- Keyboard shortcuts (Ctrl+Z / Ctrl+Y) ----
				// Only fire when no ImGui widget is actively capturing keyboard input
				if (!ImGui::GetIO().WantTextInput) {
					bool ctrl = ImGui::GetIO().KeyCtrl;

					if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
						command_manager->undo();
					}
					if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
						command_manager->redo();
					}
					if (ctrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) {
						auto scn = PN_SCENE_SERVICE;
						if (scn) {
							if (scn->isPlaying()) {
								openPopUp("Info", std::make_shared<std::string>("Cannot save scene while Game is Playing! Please Stop first."));
							}
							else {
								scn->saveActiveScene(scn->getCurrScnID());
								openPopUp("Info", std::make_shared<std::string>("Scene Saved!"));
							}
						}
					}
				}


				if (ImGui::BeginMenuBar()) {

					// ---- File ----
					if (ImGui::BeginMenu("File")) {
						if (ImGui::MenuItem("New Scene", "Ctrl+N")) { openPopUp("New Scene"); }
						if (ImGui::MenuItem("Load Scene", "Ctrl+O")) { openPopUp("Load Scene"); }
						ImGui::Separator();
						if (ImGui::MenuItem("Save", "Ctrl+S")) {
							auto scn = PN_SCENE_SERVICE;
							if (scn) {
								if (scn->isPlaying()) {
									openPopUp("Info", std::make_shared<std::string>("Cannot save scene while Game is Playing! Please Stop first.")); // <-- ADDED
								}
								else {
									scn->saveActiveScene(scn->getCurrScnID());
									openPopUp("Info", std::make_shared<std::string>("Scene Saved!")); // <-- ADDED
								}
							}
						}
						if (ImGui::MenuItem("Save As...")) { openPopUp("Save As..."); }
						ImGui::Separator();
						if (ImGui::MenuItem("Settings")) { openPopUp("Settings"); }
						ImGui::Separator();
						if (ImGui::MenuItem("Exit")) {
							auto win = services->get<Window::Window>();
							win->safeShutdown();
						}
						ImGui::EndMenu();
					}

					// ---- Edit ----
					if (ImGui::BeginMenu("Edit")) {
						bool canUndo = command_manager->canUndo();
						bool canRedo = command_manager->canRedo();

						// Undo
						ImGui::BeginDisabled(!canUndo);
						if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
							command_manager->undo();
						}
						if (canUndo && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
							ImGui::SetTooltip("Undo: %s", command_manager->getNextUndoDescription().c_str());
						}
						ImGui::EndDisabled();

						// Redo
						ImGui::BeginDisabled(!canRedo);
						if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
							command_manager->redo();
						}
						if (canRedo && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
							ImGui::SetTooltip("Redo: %s", command_manager->getNextRedoDescription().c_str());
						}
						ImGui::EndDisabled();

						ImGui::Separator();
						ImGui::MenuItem("Preferences...");
						ImGui::EndMenu();
					}

					// ---- Assets ----
					if (ImGui::BeginMenu("Assets")) {
						ImGui::MenuItem("Create");
						ImGui::EndMenu();
					}

					// ---- GameObject ----
					if (ImGui::BeginMenu("GameObject")) {
						if (ImGui::MenuItem("Create Empty")) {
							openPopUp("Create Entity");
						}
						ImGui::EndMenu();
					}

					// ---- Component ----
					if (ImGui::BeginMenu("Component")) {
						if (ImGui::MenuItem("Add...")) { openPopUp("Add Component"); }
						ImGui::EndMenu();
					}

					ImGui::Separator();

					// ---- Center: Scene name ----
					{
						std::string scn_name = "No Scene";
						Assets::GUID curr_id = PN_SCENE_SERVICE ? PN_SCENE_SERVICE->getCurrScnID() : Assets::GUID{};
						if (curr_id.IsValid()) {
							auto scn_data = PN_ASSET_SERVICE->getAssetData(curr_id);
							if (scn_data) {
								scn_name = scn_data->name;
								auto dot = scn_name.rfind('.');
								if (dot != std::string::npos) scn_name = scn_name.substr(0, dot);
							}
						}

						float text_width = ImGui::CalcTextSize(scn_name.c_str()).x;
						float center_x = (ImGui::GetWindowWidth() - text_width) * 0.5f;
						float current_x = ImGui::GetCursorPosX();
						if (center_x > current_x) ImGui::SetCursorPosX(center_x);

						ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", scn_name.c_str());
					}

					ImGui::EndMenuBar();
				}

				// Toolbar strip below the menu bar
				float menu_h = ImGui::GetFrameHeight();
				ImGui::SetCursorPosY(menu_h);

				ImGuiWindowFlags toolbar_flags =
					ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
					ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
					ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoScrollbar;

				ImGui::BeginChild("##TopToolbar", ImVec2(0, 20.f), false, toolbar_flags);
				ImGui::EndChild();

				renderPopUps(); // <-- MOVED: out of child window so popups anchor correctly
			}

			// ----------------------------------------------------------------
			// onEvent
			// ----------------------------------------------------------------
			void Tools::onEvent(Event::Event& event) {
				Event::Dispatcher dispatcher(event);

				dispatcher.Dispatch<Event::WindowClosed>([&](Event::WindowClosed& e) -> bool {
					auto ser = PN_SERI_SERVICE;
					auto scn = PN_SCENE_SERVICE;
					closeAllPopUps();

					// In CI or headless environments, skip save prompts entirely
					const bool is_ci = std::getenv("CI") != nullptr;

					// If the game is actively playing, don't halt shutdown for saving prompts
					if (is_ci || (scn && scn->isPlaying())) {
						auto win = services->get<Window::Window>();
						win->safeShutdown();
						return true;
					}

					if (ser->getIsModifiedScene()) {
						openPopUp("Unsaved Changes");
						return true;
					}
					else if (ser->getIsCurSceneEmpty()) {
						openPopUp("Unsaved Scene");
						return true;
					}

					auto win = services->get<Window::Window>();
					win->safeShutdown();
					return true;
					});
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
#endif
