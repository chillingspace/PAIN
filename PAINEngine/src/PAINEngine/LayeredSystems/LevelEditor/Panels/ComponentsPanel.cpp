#include "pch.h"
#include "ComponentsPanel.h"

#ifdef _DEBUG

#include "../Editor.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/sMetaData.h"
#include "EntityPanel.h"
#include "Systems/Transform/sysTransform.h"

#include "CoreSystems/Prefabs/sPrefab.h"
#include "LayeredSystems/LevelEditor/Panels/ReflectionUI.h"

namespace PAIN {
	namespace Editor {
		namespace Panel {

			ComponentsPanel::ComponentsPanel() {
				name = "Components";
				flags = ImGuiWindowFlags_None;
			}

			void ComponentsPanel::onAttach() {
				// Register component-specific

				// ---- Entity GUID ----
				registerCompUIFunc<PAIN::Entity::GUID>(
					"GUID", [](ComponentsPanel&, PAIN::Entity::GUID& as) {
						DrawWithReflection(as);
					});

				// ---- Entity Name ----
				registerCompUIFunc<PAIN::Entity::Name>(
					"Name", [](ComponentsPanel&, PAIN::Entity::Name& as) {
						DrawWithReflection(as);
					});

				// ---- Entity Hierarchy ----
				registerCompUIFunc<PAIN::Entity::Hierarchy>(
					"Hierarchy", [](ComponentsPanel&, PAIN::Entity::Hierarchy& as) {
						DrawWithReflection(as);
					});

				// ---- Prefab Instance ----
				registerCompUIFunc<PAIN::Prefab::PrefabInstance>(
					"PrefabInstance",
					[](ComponentsPanel&, PAIN::Prefab::PrefabInstance& as) {
						DrawWithReflection(as);
					});

				// ---- Transform ----
				registerCompUIFunc<PAIN::LocalTransform>(
					"LocalTransform",
					[&](ComponentsPanel& panel, PAIN::LocalTransform& transform_ref) {
						static struct {
							entt::entity entity = entt::null;
							LocalTransform original_transform;
							LocalTransform last_frame_transform;
							bool is_editing = false;
							int skip_frames = 0; // NEW: Skip detection for N frames
						} state;

						auto entity_panel = entities_panel.lock();
						if (!entity_panel) {
							DrawWithReflection(transform_ref);
							return;
						}

						entt::entity selected = entity_panel->getSelectedEntity();
						if (selected == entt::null) {
							DrawWithReflection(transform_ref);
							return;
						}

						// NEW: Skip detection if undo/redo is executing
						if (command_manager && command_manager->isExecutingUndoRedo()) {
							state.skip_frames = 2; // Skip next 2 frames
							state.is_editing = false;
							DrawWithReflection(transform_ref);
							return;
						}

						// NEW: Decrement skip counter
						if (state.skip_frames > 0) {
							state.skip_frames--;
							DrawWithReflection(transform_ref);
							return;
						}

						// Start tracking when any ImGui item becomes active
						if (ImGui::IsAnyItemActive() && !state.is_editing) {
							state.entity = selected;
							state.original_transform = transform_ref;
							state.last_frame_transform = transform_ref;
							state.is_editing = true;
						}

						// Draw the reflection UI
						DrawWithReflection(transform_ref);

						// ECS controller
						auto ecs = services->get<ECS::Controller>();
						auto transformSystem = ecs->getSystem<Transform::System>();

						// Detect if transform changed this frame
						if (state.is_editing) {
							if (state.last_frame_transform.position != transform_ref.position ||
								state.last_frame_transform.rotation != transform_ref.rotation ||
								state.last_frame_transform.scale != transform_ref.scale) {
								state.last_frame_transform = transform_ref;

								// Mark dirty
								if (transformSystem) {
									transformSystem->markDirty(selected,
															   ecs->getRegistry(currentRegistryID));
								}
							}
						}

						// When user stops editing
						if (state.is_editing && !ImGui::IsAnyItemActive() &&
							!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
							// Check if anything actually changed
							if (state.original_transform.position != transform_ref.position ||
								state.original_transform.rotation != transform_ref.rotation ||
								state.original_transform.scale != transform_ref.scale) {

								// Create undo/redo action
								LocalTransform final_transform = transform_ref;
								LocalTransform old_transform = state.original_transform;
								entt::entity entity = selected;

								auto metadata = services->get<MetaData::Service>();

								std::string entity_name = "Entity";
								if (metadata) {
									entity_name = metadata->getEntityName(entity);
								}

								// Create temp registry id
								auto registr_id = currentRegistryID;

								command_manager->executeAction(Action{
									[ecs, entity, final_transform, transformSystem, registr_id]() {
										if (ecs->checkEntity(entity, registr_id)) {
											auto transform_opt =
												ecs->getEntityComponent<LocalTransform>(entity,
																						registr_id);
											if (transform_opt.has_value()) {
												transform_opt.value().get() = final_transform;
											}
											// Mark dirty
											if (transformSystem) {
												transformSystem->markDirty(entity,
																		   ecs->getRegistry(registr_id));
											}
										}
									},
									[ecs, entity, old_transform, transformSystem, registr_id]() {
										if (ecs->checkEntity(entity, registr_id)) {
											auto transform_opt =
												ecs->getEntityComponent<LocalTransform>(entity,
																						registr_id);
											if (transform_opt.has_value()) {
												transform_opt.value().get() = old_transform;
											}
											// Mark dirty
											if (transformSystem) {
												transformSystem->markDirty(entity,
																		   ecs->getRegistry(registr_id));
											}
										}
									},
									"Modify Transform: " + entity_name});
							}

							// Reset editing state
							state.is_editing = false;
						}
					});

				// ---- ModelRenderer ----
				registerCompUIFunc<PAIN::ModelRenderer>(
					"ModelRenderer",
					[this](ComponentsPanel& panel, PAIN::ModelRenderer& renderer) {
						// Model GUID selector (using reflection)
						bool changed = false;

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

						// Model Asset Selection
						if (DrawAssetSelectorField("Select A Model", renderer.modelGUID,
												   PAIN::Editor::Attributes::AssetSelector(
													   PAIN::Assets::Type::Model),
												   panel.services)) {
							changed = true;
						}

						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();

						// Rendering Options
						if (ImGui::CollapsingHeader("Rendering Options")) {
							ImGui::Indent(10.0f);
							changed |= ImGui::Checkbox("Visible", &renderer.visible);
							changed |= ImGui::Checkbox("Cast Shadows", &renderer.castShadows);
							changed |=
								ImGui::Checkbox("Receive Shadows", &renderer.receiveShadows);
							ImGui::Unindent(10.0f);
						}

						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();

						// MATERIALS SECTION - This is where the magic happens!
						if (DrawField("Materials", renderer.materials, &panel)) {
							changed = true;
						}

						//if (ImGui::CollapsingHeader("Overrides")) {
						//	ImGui::Indent(10.f);
						//	changed |= ImGui::Checkbox("Override Emissive Map", &renderer.materials[0].useEmissiveOverride);
						//	changed |= ImGui::ColorEdit3("Emissive Color Override", &renderer.materials[0].emissiveOverride.x);
						//	ImGui::Unindent(10.f);
						//}

						ImGui::PopStyleVar();
					});

				// ---- Texture2D ----
				registerCompUIFunc<PAIN::Texture2D>(
					"Texture2D",
					[this](ComponentsPanel& panel, PAIN::Texture2D& texture_comp) {
						// Model GUID selector (using reflection)
						bool changed = false;

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

						// Model Asset Selection
						if (DrawAssetSelectorField("Select A Texture",
												   texture_comp.texture_guid,
												   PAIN::Editor::Attributes::AssetSelector(
													   PAIN::Assets::Type::Texture),
												   panel.services)) {
							changed = true;
						}

						changed |=
							ImGui::DragFloat2("Texture Scale", &texture_comp.texture_scale.x,
											  0.02f, 0.2f, 4.0f, "%.2f");
						changed |= ImGui::DragFloat2("Texture Position", &texture_comp.pos.x);
						
						ImGui::PopStyleVar();

						return changed;
					});

				// ---- Animation ----
				registerCompUIFunc<PAIN::Animation>(
					"Animation", [](ComponentsPanel&, PAIN::Animation& anim) {
						// Basic fields; reflection will handle labels from cAnimation.h
						DrawWithReflection(anim);

						ImGui::Spacing();
						ImGui::Separator();
						ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Runtime Debug Info");

						// Show Status
						if (anim.isPlaying) {
							ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Status: Playing");
						}
						else {
							ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "Status: Paused");
						}

						// Read-only Runtime Values
						ImGui::BeginDisabled(); // Grey out to show they aren't editable settings

						int currIdx = anim.currentAnimationIndex;
						ImGui::InputInt("Current Index", &currIdx);

						float currTime = anim.animationTime;
						ImGui::DragFloat("Time", &currTime);

						int nextIdx = anim.nextAnimationIndex;
						if (nextIdx != -1) {
							ImGui::InputInt("Next Index", &nextIdx);
						}

						ImGui::EndDisabled();

						// 3. Optional: Manual Controls for testing
						if (ImGui::Button(anim.isPlaying ? "Pause##Comp" : "Resume##Comp")) {
							anim.isPlaying = !anim.isPlaying;
						}
					});

				// UItext comp ui
				registerCompUIFunc<PAIN::UIText>("UIText", [this](ComponentsPanel& panel,
																  PAIN::UIText& text) {
					bool changed = false;

					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

					// Editable text string
					char text_buffer[1024];
					std::strncpy(text_buffer, text.display_text.c_str(), sizeof(text_buffer));
					text_buffer[sizeof(text_buffer) - 1] = '\0';
					if (ImGui::InputTextMultiline("Text", text_buffer, sizeof(text_buffer),
												  ImVec2(-1, 0),
												  ImGuiInputTextFlags_AllowTabInput)) {
						text.display_text = text_buffer;
						changed = true;
					}

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					// Font selection
					if (DrawAssetSelectorField(
							"Select A Font", text.font_guid,
							PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Font),
							panel.services)) {
						changed = true;
					}

					// Font position
					ImGui::Text("Text Position: (%.8f, %.8f)", text.text_pos.x,
								text.text_pos.y);

					// Font size
					changed |= ImGui::DragFloat("Font Size", &text.font_size, 0.2f, 6.0f,
												128.0f, "%.1f");

					// Alignment
					static const char* alignment_items[] = {"Left", "Center", "Right"};
					int align_idx = static_cast<int>(text.alignment);
					if (ImGui::Combo("Alignment", &align_idx, alignment_items,
									 IM_ARRAYSIZE(alignment_items))) {
						text.alignment = static_cast<PAIN::TextAlignment>(align_idx);
						changed = true;
					}

					ImGui::Spacing();

					// Color edit
					changed |= ImGui::ColorEdit4("Text Color", &text.color.x);

					// Outline
					ImGui::Separator();
					ImGui::Text("Outline");
					changed |= ImGui::DragFloat("Thickness", &text.outline_thickness, 0.05f,
												0.0f, 16.0f, "%.2f");
					changed |= ImGui::ColorEdit4("Outline Color", &text.outline_color.x);

					// Shadow
					ImGui::Separator();
					ImGui::Text("Shadow");
					changed |= ImGui::DragFloat2("Offset", &text.shadow_offset.x, 1.0f, 0.0f,
												 0.0f, "%.1f");
					changed |= ImGui::ColorEdit4("Shadow Color", &text.shadow_color.x);

					ImGui::Spacing();

					// Word wrap & Rich Text
					changed |= ImGui::Checkbox("Word Wrap", &text.word_wrap);

					changed |= ImGui::DragFloat("Line Height", &text.line_height, 0.02f, 0.2f,
												4.0f, "%.2f");

					changed |= ImGui::DragFloat("Text Wrap", &text.wrap_width, 0.02f, 0.2f,
												4.0f, "%.2f");

					// Max length (for input fields, optional)
					changed |= ImGui::DragInt("Max Length", &text.max_length, 1, 0, 4096);

					ImGui::PopStyleVar();

					return changed;
				});

				registerCompUIFunc<PAIN::UIButton>(
					"UIButton", [this](ComponentsPanel& panel, PAIN::UIButton& button) {
						bool changed = false;

						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

						//// ── Script Callback ──
						//ImGui::SeparatorText("Callback");

						//// Function name input
						//char funcName[128];
						//strncpy(funcName, button.on_click_callback_lua.c_str(),
						//		sizeof(funcName) - 1);
						//funcName[sizeof(funcName) - 1] = '\0';

						//if (ImGui::InputText("Function Name", funcName, sizeof(funcName))) {
						//	button.on_click_callback_lua = funcName;
						//	changed = true;
						//}
						//ImGui::TextDisabled("(e.g., OnPlayButtonClick)");
						//ImGui::TextDisabled("Function must be defined in a loaded script");

						//// ── Button State (Read-only display) ──
						//ImGui::SeparatorText("State");

						//const char* state_names[] = {"Normal", "Highlighted", "Pressed",
						//							 "Disabled"};
						//int current_state = static_cast<int>(button.state);

						//ImGui::BeginDisabled(); // Make it read-only (runtime state)
						//ImGui::Combo("Current State", &current_state, state_names,
						//			 IM_ARRAYSIZE(state_names));
						//ImGui::EndDisabled();

						//ImGui::TextDisabled("(Runtime state - updates automatically)");

						// -- Action --
						ImGui::SeparatorText("Action");

						// must match UIAction order
						static const char* action_names[] = {
							"None",
							"game_Jump",
							"game_Hide",
							"game_Collect",
							"game_Move",

							"pause_Resume",
							"pause_Restart",
							"pause_Settings",
							"pause_ReturnToMainMenu",

							"menu_StartGame",
							"menu_OpenSettings",
							"menu_HowToPlay",
							"menu_Credits",
							"menu_OpenTutorial",
							"menu_QuitGame",

							"menu_BackToMain",

							"quit_Confirm",
							"quit_Cancel",
						};

						int action_idx = static_cast<int>(button.action);

						if (ImGui::Combo("UI Action", &action_idx, action_names, IM_ARRAYSIZE(action_names))) {
							button.action = static_cast<PAIN::UIAction>(action_idx);
							changed = true;

							// if user picked a real action, clear legacy callback to avoid confusion
							if (button.action != PAIN::UIAction::None) {
								button.on_click_callback_lua.clear();
							}
						}

						ImGui::TextDisabled("Pick an action.");

						// -- Payload --
						ImGui::SeparatorText("Payload (Optional)");
						ImGui::TextDisabled("Used by actions like StartGame/Restart/ReturnToMainMenu (e.g. scene path).");

						char payloadBuf[256];
						strncpy(payloadBuf, button.payload.c_str(), sizeof(payloadBuf) - 1);
						payloadBuf[sizeof(payloadBuf) - 1] = '\0';

						if (ImGui::InputText("Payload", payloadBuf, sizeof(payloadBuf))) {
							button.payload = payloadBuf;
							changed = true;
						}

						// ── State Colors ──
						ImGui::SeparatorText("State Colors");

						auto editColor = [&](const char* label, int& color) -> bool {
							// Convert 0xAARRGGBB to ImVec4
							float a = ((color >> 24) & 0xFF) / 255.0f;
							float r = ((color >> 16) & 0xFF) / 255.0f;
							float g = ((color >> 8) & 0xFF) / 255.0f;
							float b = ((color >> 0) & 0xFF) / 255.0f;
							ImVec4 col(r, g, b, a);

							bool color_changed = ImGui::ColorEdit4(
								label, (float*)&col,
								ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview |
									ImGuiColorEditFlags_DisplayHex);

							if (color_changed) {
								int na = static_cast<int>(col.w * 255.0f) & 0xFF;
								int nr = static_cast<int>(col.x * 255.0f) & 0xFF;
								int ng = static_cast<int>(col.y * 255.0f) & 0xFF;
								int nb = static_cast<int>(col.z * 255.0f) & 0xFF;
								color = (na << 24) | (nr << 16) | (ng << 8) | nb;
							}

							return color_changed;
						};

						changed |= editColor("Normal Color##normal", button.normal_color);
						changed |= editColor("Highlighted Color##highlighted",
											 button.highlighted_color);
						changed |= editColor("Pressed Color##pressed", button.pressed_color);
						changed |= editColor("Disabled Color##disabled", button.disabled_color);

						// ── Optional: Preview current state color ──
						ImGui::Spacing();
						ImGui::Text("Current State Preview:");

						int preview_color = button.normal_color;
						switch (button.state) {
						case UIButtonState::Normal:
							preview_color = button.normal_color;
							break;
						case UIButtonState::Highlighted:
							preview_color = button.highlighted_color;
							break;
						case UIButtonState::Pressed:
							preview_color = button.pressed_color;
							break;
						case UIButtonState::Disabled:
							preview_color = button.disabled_color;
							break;
						}

						float pa = ((preview_color >> 24) & 0xFF) / 255.0f;
						float pr = ((preview_color >> 16) & 0xFF) / 255.0f;
						float pg = ((preview_color >> 8) & 0xFF) / 255.0f;
						float pb = ((preview_color >> 0) & 0xFF) / 255.0f;
						ImVec4 preview_col(pr, pg, pb, pa);

						ImGui::ColorButton("##preview", preview_col,
										   ImGuiColorEditFlags_NoTooltip |
											   ImGuiColorEditFlags_NoPicker,
										   ImVec2(ImGui::GetContentRegionAvail().x, 30));

						ImGui::PopStyleVar();

						return changed;
					});

				registerCompUIFunc<PAIN::Cam>("Camera", [](ComponentsPanel&, PAIN::Cam& as) {
					DrawWithReflection(as);
				});

				registerCompUIFunc<PAIN::MetaData::Tag>(
					"Tag", [this](ComponentsPanel& panel, PAIN::MetaData::Tag& tagComp) {
						auto ecs = panel.services->get<ECS::Controller>();
						auto metaSvc = panel.services->get<PAIN::MetaData::Service>();
						auto entityPanel = panel.entities_panel.lock();

						if (!ecs || !metaSvc || !entityPanel) {
							ImGui::TextUnformatted("Metadata/Entity panel not available");
							return;
						}

						entt::entity e = entityPanel->getSelectedEntity();
						if (e == entt::null) {
							ImGui::TextUnformatted("No entity selected");
							return;
						}

						std::string currentTag = "Untagged";
						if (!tagComp.tags.empty())
							currentTag = *tagComp.tags.begin();

						ImGui::Text("Current Tag: %s", currentTag.c_str());

						const auto& allTags = metaSvc->getRegisteredTags();

						if (ImGui::BeginCombo("Tag", currentTag.c_str())) {
							for (auto const& t : allTags) {
								bool selected = (t == currentTag);
								if (ImGui::Selectable(t.c_str(), selected)) {
									// Clears old tags & sets this one
									metaSvc->setEntityTag(e, t);
								}
								if (selected)
									ImGui::SetItemDefaultFocus();
							}
							ImGui::EndCombo();
						}

						// Allow adding a new custom tag
						static char newTagBuf[64] = {};
						ImGui::InputText("New Tag", newTagBuf, sizeof(newTagBuf));
						ImGui::SameLine();
						if (ImGui::Button("Add##Tag") && newTagBuf[0] != '\0') {
							metaSvc->setEntityTag(e, newTagBuf); // also registers it
							newTagBuf[0] = '\0';
						}
					});

				// ---- Light ----
				registerCompUIFunc<PAIN::Lighting>(
					"Lighting", [](ComponentsPanel&, PAIN::Lighting& light) {
						// Draw refl variables
						DrawWithReflection(light);

						// Direction lighting (Only for Directional & Spotlight)
						if (light.light_type != PAIN::TYPES::POINT) {
							ImGui::Separator();
							ImGui::Text("Direction Settings");

							if (ImGui::DragFloat3("Direction", glm::value_ptr(light.direction),
												  0.01f, -1.0f, 1.0f)) {
								if (glm::length(light.direction) > 0.0001f)
									light.direction = glm::normalize(light.direction);
							}
						}

						// Spotlight variables
						if (light.light_type == PAIN::TYPES::SPOTLIGHT) {
							ImGui::Separator();
							ImGui::Text("Spotlight Cone");

							ImGui::DragFloat("Inner Angle", &light.inner_angle, 0.5f, 0.0f, 89.0f,
											 "%.1f deg");

							float minOuter = light.inner_angle + 0.1f;
							if (ImGui::DragFloat("Outer Angle", &light.outer_angle, 0.5f,
												 minOuter, 179.0f, "%.1f deg")) {
								if (light.outer_angle < minOuter)
									light.outer_angle = minOuter;
							}
						}
					});
				// ---- AudioSource ----
				registerCompUIFunc<PAIN::Audio::AudioSource>(
					"AudioSource", [this](ComponentsPanel&, PAIN::Audio::AudioSource& as) {
						DrawWithReflection(as, static_cast<ComponentsPanel*>(this));
					});

				// ---- BoundingVolume ----
				registerCompUIFunc<PAIN::BoundingVolume>(
					"BoundingVolume", [](ComponentsPanel&, PAIN::BoundingVolume& bv) {
						DrawWithReflection(bv);

						// Local AABB
						ImGui::Text("Local AABB");
						// Pass flags for ReadOnly
						ImGui::InputFloat3("Local Min", glm::value_ptr(bv.localAABB.min), "%.3f", ImGuiInputTextFlags_ReadOnly);
						ImGui::InputFloat3("Local Max", glm::value_ptr(bv.localAABB.max), "%.3f", ImGuiInputTextFlags_ReadOnly);

						ImGui::Separator();

						// World AABB
						ImGui::Text("World AABB");
						ImGui::InputFloat3("World Min", glm::value_ptr(bv.worldAABB.min), "%.3f", ImGuiInputTextFlags_ReadOnly);
						ImGui::InputFloat3("World Max", glm::value_ptr(bv.worldAABB.max), "%.3f", ImGuiInputTextFlags_ReadOnly);

						// Size of aabb
						ImGui::Spacing();
						glm::vec3 worldSize = bv.worldAABB.max - bv.worldAABB.min;
						ImGui::TextDisabled("Dimensions: (%.2f, %.2f, %.2f)", worldSize.x, worldSize.y, worldSize.z);
					});

				// ---- Physics ----
				registerCompUIFunc<PAIN::Joint>(
					"Joint",
					[](ComponentsPanel&, PAIN::Joint& as) { DrawWithReflection(as); });

				registerCompUIFunc<Physics::RigidBody3D>(
					"RigidBody3D", [](ComponentsPanel&, Physics::RigidBody3D& rb) {
						DrawWithReflection(rb);
					});

				// ---- CompoundCollider ----
				registerCompUIFunc<PAIN::CompoundCollider>(
					"CompoundCollider", [](ComponentsPanel&, PAIN::CompoundCollider& comp) {
						bool changed = false;

						// Toggle for using compound collider
						changed |=
							ImGui::Checkbox("Use Compound Collider", &comp.useCompoundCollider);

						if (!comp.useCompoundCollider) {
							ImGui::TextDisabled("Enable to define custom collision shapes");
							return;
						}

						ImGui::Separator();
						ImGui::Text("Shapes (%zu)", comp.shapes.size());

						// Add shape buttons
						if (ImGui::Button("+ Box")) {
							comp.addBox(glm::vec3(0.5f));
							changed = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("+ Sphere")) {
							comp.addSphere(0.5f);
							changed = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("+ Capsule")) {
							comp.addCapsule(0.25f, 0.5f);
							changed = true;
						}

						ImGui::Separator();

						// Edit each shape
						int toRemove = -1;
						for (size_t i = 0; i < comp.shapes.size(); ++i) {
							auto& shape = comp.shapes[i];
							ImGui::PushID(static_cast<int>(i));

							// Shape type label
							const char* typeNames[] = {"Box", "Sphere", "Capsule"};
							int typeIndex = static_cast<int>(shape.type);
							std::string header =
								std::string(typeNames[typeIndex]) + " #" + std::to_string(i);

							if (ImGui::CollapsingHeader(header.c_str(),
														ImGuiTreeNodeFlags_DefaultOpen)) {
								// Shape type selector
								if (ImGui::Combo("Type", &typeIndex, typeNames, 3)) {
									shape.type = static_cast<ColliderShapeType>(typeIndex);
									changed = true;
								}

								// Common properties
								changed |= ImGui::DragFloat3("Offset", &shape.offset.x, 0.01f);

								// Type-specific properties
								switch (shape.type) {
								case ColliderShapeType::Box:
									changed |=
										ImGui::DragFloat3("Half Extents", &shape.boxHalfExtents.x,
														  0.01f, 0.01f, 100.0f);
									break;
								case ColliderShapeType::Sphere:
									changed |= ImGui::DragFloat("Radius", &shape.sphereRadius, 0.01f,
																0.01f, 100.0f);
									break;
								case ColliderShapeType::Capsule:
									changed |= ImGui::DragFloat("Radius", &shape.capsuleRadius, 0.01f,
																0.01f, 100.0f);
									changed |=
										ImGui::DragFloat("Half Height", &shape.capsuleHalfHeight,
														 0.01f, 0.01f, 100.0f);
									break;
								}

								// Remove button
								ImGui::PushStyleColor(ImGuiCol_Button,
													  ImVec4(0.6f, 0.2f, 0.2f, 1.0f));
								if (ImGui::Button("Remove Shape")) {
									toRemove = static_cast<int>(i);
								}
								ImGui::PopStyleColor();
							}

							ImGui::PopID();
						}

						// Remove shape if requested
						if (toRemove >= 0) {
							comp.shapes.erase(comp.shapes.begin() + toRemove);
							changed = true;
						}

						if (comp.shapes.empty()) {
							ImGui::TextDisabled("No shapes defined. Add shapes above.");
						}
					});

				// ---- Script ----
				// registerCompUIFunc<PAIN::Script>("Script",
				//     [this](ComponentsPanel&, PAIN::Script& as) { DrawWithReflection(as,
				//     static_cast<ComponentsPanel*>(this)); });

				// ---- AI ----
				registerCompUIFunc<PAIN::AI::Controller>(
					"AIController", [](ComponentsPanel& panel, PAIN::AI::Controller& rb) {
						DrawWithReflection(rb, &panel);
					});

				// registerCompUIFunc<PAIN::AI::Controller>("AIController",
				//     [](ComponentsPanel&, PAIN::AI::Controller& rb) {
				//     DrawWithReflection(rb); });

				registerCompUIFunc<PAIN::AI::Sensors>(
					"AISensors",
					[](ComponentsPanel&, PAIN::AI::Sensors& rb) { DrawWithReflection(rb); });

				registerCompUIFunc<PAIN::AI::NavAgent>(
					"AINavAgent", [](ComponentsPanel&, PAIN::AI::NavAgent& rb) {
						DrawWithReflection(rb);
					});

				registerCompUIFunc<PAIN::AI::Steering>(
					"AISteering", [](ComponentsPanel&, PAIN::AI::Steering& rb) {
						DrawWithReflection(rb);
					});

				registerCompUIFunc<PAIN::AI::Blackboard>(
					"AIBlackboard", [](ComponentsPanel&, PAIN::AI::Blackboard& bb) {
#ifdef _DEBUG
						bb.DebugDrawImGui();
#else
						ImGui::Text("Blackboard (debug view only in _DEBUG builds)");
#endif
					});

				registerCompUIFunc<PAIN::AI::CommandQueue>(
					"AICommandQueue", [](ComponentsPanel&, PAIN::AI::CommandQueue& q) {
#ifdef _DEBUG
						q.DebugDrawImGui();
#else
						ImGui::Text("CommandQueue (debug view only in _DEBUG builds)");
#endif
					});

				/*******************************************
   *  UI comps
   *******************************************/
				registerCompUIFunc<PAIN::UIRectTransform>(
					"UIRectTransform",
					[this](ComponentsPanel&, PAIN::UIRectTransform& transform_ui) {
						DrawWithReflection(transform_ui, static_cast<ComponentsPanel*>(this));
					});

				// registerCompUIFunc<PAIN::UIButton>("UIButton",
				//     [this](ComponentsPanel&, PAIN::UIButton& ui) { DrawWithReflection(ui,
				//     static_cast<ComponentsPanel*>(this)); });

				// ---- UIJoystick ----
				registerCompUIFunc<PAIN::UIJoystick>(
					"UIJoystick", [this](ComponentsPanel& panel, PAIN::UIJoystick& joystick) {
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

						// Max radius
						ImGui::SeparatorText("Joystick Settings");
						ImGui::DragFloat("Max Radius", &joystick.max_radius, 0.01f, 0.05f, 0.5f,
										 "%.3f");

						// Runtime state (read-only)
						ImGui::SeparatorText("Runtime State (Read-Only)");
						ImGui::BeginDisabled();
						ImGui::Checkbox("Is Dragging", &joystick.is_dragging);
						ImGui::DragFloat2("Center Position", &joystick.center_position.x,
										  0.01f);
						ImGui::EndDisabled();

						if (joystick.is_dragging) {
							ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
											   "Currently dragging!");
						} else {
							ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Not dragging");
						}

						ImGui::PopStyleVar();
					});

				registerCompUIFunc<PAIN::UIElement>(
					"UIElement", [this](ComponentsPanel&, PAIN::UIElement& ui) {
						DrawWithReflection(ui, static_cast<ComponentsPanel*>(this));
					});

				registerCompUIFunc<PAIN::UICanvas>(
					"UICanvas", [this](ComponentsPanel&, PAIN::UICanvas& ui) {
						DrawWithReflection(ui, static_cast<ComponentsPanel*>(this));
					});

				registerCompUIFunc<PAIN::UIAnimation>(
					"UIAnimation", [this](ComponentsPanel&, PAIN::UIAnimation& ui) {
						DrawWithReflection(ui, static_cast<ComponentsPanel*>(this));
					});

				registerCompUIFunc<PAIN::CustomHitbox2D>(
					"CustomHitbox2D", [this](ComponentsPanel&, PAIN::CustomHitbox2D& ui) {
						DrawWithReflection(ui, static_cast<ComponentsPanel*>(this));
					});

				registerCompUIFunc<PAIN::UIFollowsWorldEntity>(
					"UIFollowsWorldEntity",
					[this](ComponentsPanel& panel, PAIN::UIFollowsWorldEntity& follow) {
						auto ecs = panel.services->get<ECS::Controller>();

						bool changed = false;
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

						// Get ECS registry and metadata service
						auto& registry = ecs->getRegistry(currentRegistryID);
						auto metadata_service = panel.services->get<MetaData::Service>();

						// --- Dropdown for selecting world_target entity ---
						// std::vector<entt::entity> all_entities;
						// std::vector<std::string> all_names;

						// auto view = registry.view<Entity::Name>();
						//// Iterate through entities with the entity name component
						// for (auto entity : view) {
						//     std::string name = metadata_service->getEntityName(entity);
						//     if (name.empty()) name = "[unnamed]";
						//     all_entities.push_back(entity);
						//     all_names.push_back(name);
						// }

						// Find currently selected entity index
						// int current_idx = -1;
						// for (size_t i = 0; i < all_entities.size(); ++i) {
						//    if (all_entities[i] == follow.entity_target) {
						//        current_idx = (int)i;
						//        break;
						//    }
						//}

						// Accept entity as drag-drop target
						if (ImGui::BeginDragDropTarget()) {
							if (const ImGuiPayload* payload =
									ImGui::AcceptDragDropPayload("ENTITY_DRAG")) {
								entt::entity dragged_entity = *(const entt::entity*)payload->Data;

								Assets::GUID entity_guid =
									ecs->getOrCreateEntityGUID(dragged_entity, currentRegistryID);

								if (entity_guid != follow.entity_target_guid) {

									follow.entity_target_guid = entity_guid;

									changed = true;
								}
							}
							ImGui::EndDragDropTarget();
						}

						entt::entity entity =
							ecs->resolveGUID(follow.entity_target_guid, currentRegistryID);

						std::string entity_name = metadata_service->getEntityName(entity);

						// Have an imgui text to show the current entity that is dragged
						std::string target_name = entity != entt::null ? entity_name : "[none]";

						ImGui::Text("World Target: %s", target_name.c_str());

						// Optionally allow clearing the target
						if (ImGui::Button("Clear Target")) {
							follow.entity_target_guid = Assets::GUID{};
							changed = true;
						}

						// Create dropdown
						// if (ImGui::BeginCombo("World Target", current_idx >= 0 ?
						// all_names[current_idx].c_str() : "[none]")) {
						//    for (size_t i = 0; i < all_entities.size(); ++i) {
						//        bool is_selected = (follow.entity_target == all_entities[i]);
						//        if (ImGui::Selectable(all_names[i].c_str(), is_selected)) {
						//            follow.entity_target = all_entities[i];
						//            changed = true;
						//        }
						//        if (is_selected)
						//            ImGui::SetItemDefaultFocus();
						//    }
						//    ImGui::EndCombo();
						//}

						// World offset input
						changed |= ImGui::DragFloat3("World Offset", &follow.world_offset.x,
													 0.1f, -100.0f, 100.0f, "%.2f");

						ImGui::PopStyleVar();
						return changed;
					});

				// ---- Script ---- (UNCHANGED)
				/*registerCompUIFunc<PAIN::Scripts>("Scripts",
      [this](ComponentsPanel&, PAIN::Scripts& as) { DrawWithReflection(as,
     this); });*/

				registerCompUIFunc<PAIN::Scripts>(
					"Scripts", [this](ComponentsPanel& panel, PAIN::Scripts& comp) {
						auto services = panel.services;

						ImGui::Spacing();
						ImGui::Separator();
						ImGui::TextUnformatted("Attached Scripts");
						ImGui::Separator();
						ImGui::Spacing();

						auto& scripts = comp.scripts;

						for (size_t i = 0; i < scripts.size(); /* manual increment */) {
							auto& s = scripts[i];

							ImGui::PushID(static_cast<int>(i));

							if (DrawAssetSelectorField("Script Asset", s.script_asset,
													   PAIN::Editor::Attributes::AssetSelector(
														   PAIN::Assets::Type::Script),
													   services)) {
								s.loaded = false; // force reload on next run
							}

							ImGui::SameLine();
							ImGui::Checkbox("Enabled", &s.enabled);

							// show Loaded as read-only
							ImGui::SameLine();
							ImGui::BeginDisabled();
							ImGui::Checkbox("Loaded", &s.loaded);
							ImGui::EndDisabled();

							ImGui::SameLine();
							if (ImGui::Button("Remove")) {
								scripts.erase(scripts.begin() + static_cast<std::ptrdiff_t>(i));
								ImGui::PopID();
								continue;
							}

							ImGui::PopID();
							++i;
							ImGui::Spacing();
						}

						if (ImGui::Button("+ Add Script")) {
							PAIN::Script s{};
							s.enabled = true;
							s.loaded = false;
							comp.scripts.push_back(s);
						}

						ImGui::Spacing();
					});

				PAIN::Editor::Panel::RegisterColliderUI(*this);

				// Get entity panel reference
				auto editor = services->get<PAIN::Editor::Editor>();
				if (editor) {
					entities_panel = editor->getPanel<EntityPanel>();
				}

				// Register Add Component popup
				registerPopUp("AddComponent", addComponentPopUp("AddComponent"));

				// Register Remove Component popup
				registerPopUp("RemoveComponent", removeComponentPopUp("RemoveComponent"));

				// Register RigidBody3D Config popup
				registerPopUp("AddRigidBody3DConfig",
							  addRigidBodyConfigPopUp("AddRigidBody3DConfig"));
			}

			void ComponentsPanel::nextWindowSettings() {
				ImGui::SetNextWindowSize(ImVec2(350, 600), ImGuiCond_FirstUseEver);
			}

			std::function<void(std::any const&)>
			ComponentsPanel::addComponentPopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					auto ecs = services->get<ECS::Controller>();
					auto entity_panel = entities_panel.lock();

					if (!entity_panel) {
						ImGui::Text("EntityPanel not available");
						if (ImGui::Button("Close", ImVec2(-1, 0))) {
							closePopUp(popup_id);
						}
						return;
					}

					entt::entity selected_entity = entity_panel->getSelectedEntity();

					if (!ecs->checkEntity(selected_entity, currentRegistryID)) {
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
					ImGui::InputTextWithHint("##Search", "Search components...", search_filter,
											 256);
					ImGui::Spacing();

					ImGui::BeginChild("##ComponentList", ImVec2(400, 350), true,
									  ImGuiWindowFlags_AlwaysVerticalScrollbar);

					bool found_any = false;

					// Iterate registered component factories
					for (const auto& [comp_name, factory_func] : ecs->getComponentFactories()) {

						if (comp_name == getComponentName<Entity::Name>() ||
							comp_name == getComponentName<Entity::Layer>() ||
							comp_name == getComponentName<MetaData::EditorVisible>()) {
							continue;
						}

						// Skip if entity already has this component
						if (ecs->hasComponentByName(selected_entity, comp_name,
													currentRegistryID)) {
							continue;
						}

						// Search filter
						if (strlen(search_filter) > 0) {
							std::string comp_lower = comp_name;
							std::string search_lower = search_filter;
							std::transform(comp_lower.begin(), comp_lower.end(), comp_lower.begin(),
										   ::tolower);
							std::transform(search_lower.begin(), search_lower.end(),
										   search_lower.begin(), ::tolower);
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
							} else {
								ecs->addComponentByName(selected_entity, comp_name,
														currentRegistryID);
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

			std::function<void(std::any const&)>
			ComponentsPanel::removeComponentPopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					auto ecs = services->get<ECS::Controller>();
					auto entity_panel = entities_panel.lock();

					if (!ecs || !entity_panel) {
						ImGui::Text("Required services not available");
						ImGui::Spacing();
						if (ImGui::Button("Close", ImVec2(-1, 0))) {
							closePopUp(popup_id);
						}
						return;
					}

					entt::entity entity = entity_panel->getSelectedEntity();
					if (entity == entt::null || !ecs->checkEntity(entity, currentRegistryID)) {
						ImGui::Text("No valid entity selected");
						ImGui::Spacing();
						if (ImGui::Button("Close", ImVec2(-1, 0))) {
							closePopUp(popup_id);
						}
						return;
					}

					if (comp_string_ref.empty()) {
						ImGui::Text("No component selected");
						ImGui::Spacing();
						if (ImGui::Button("Close", ImVec2(-1, 0))) {
							closePopUp(popup_id);
						}
						return;
					}

					ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Remove Component");
					ImGui::Separator();
					ImGui::Spacing();

					ImGui::Text("Component:");
					ImGui::SameLine();
					ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s",
									   comp_string_ref.c_str());

					ImGui::Spacing();
					ImGui::TextWrapped("Are you sure you want to remove this component?");
					ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
									   "This action cannot be undone.");

					ImGui::Spacing();
					ImGui::Separator();
					ImGui::Spacing();

					// Center buttons
					float button_width = 120.0f;
					float spacing = ImGui::GetStyle().ItemSpacing.x;
					float total_width = (button_width * 2) + spacing;
					float offset = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
					if (offset > 0)
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

					// Remove button (red)
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
										  ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive,
										  ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

					bool remove_clicked = ImGui::Button("Remove", ImVec2(button_width, 0));

					ImGui::PopStyleColor(3);
					ImGui::SameLine();

					bool cancel_clicked = ImGui::Button("Cancel", ImVec2(button_width, 0));

					if (remove_clicked) {

						if (comp_string_ref == "Name" || comp_string_ref == "Tag" ||
							comp_string_ref == "Editor Visiblity" ||
							comp_string_ref == "Relation" || comp_string_ref == "Group") {
							closePopUp(popup_id);
							comp_string_ref.clear();
							return;
						}

						// Use new removeComponentByName method
						if (ecs->hasComponentByName(entity, comp_string_ref, currentRegistryID)) {
							ecs->removeComponentByName(entity, comp_string_ref, currentRegistryID);
						}

						closePopUp(popup_id);
						comp_string_ref.clear();
					}

					if (cancel_clicked) {
						closePopUp(popup_id);
					}
				};
			}

			std::function<void(std::any const&)>
			ComponentsPanel::addRigidBodyConfigPopUp(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {
					auto ecs = services->get<ECS::Controller>();
					auto entity_panel = entities_panel.lock();
					if (!entity_panel)
						return;

					entt::entity selected_entity = entity_panel->getSelectedEntity();
					if (!ecs->checkEntity(selected_entity, currentRegistryID))
						return;

					static int motion_type_idx = 1; // Default to Dynamic
					const char* motion_names[] = {"Static", "Dynamic", "Kinematic"};

					ImGui::Text("Select Motion Type:");
					ImGui::Combo("Motion Type", &motion_type_idx, motion_names,
								 IM_ARRAYSIZE(motion_names));

					ImGui::Spacing();
					if (ImGui::Button("Add RigidBody3D", ImVec2(-1, 0))) {
						// Add the component

						if (!ecs->hasComponentByName(selected_entity, "RigidBody3D",
													 currentRegistryID)) {
							Physics::RigidBody3D rb;
							rb.motion_type =
								static_cast<PAIN::Physics::MotionType>(motion_type_idx);
							ecs->addEntityComponent<PAIN::Physics::RigidBody3D>(
								selected_entity, std::move(rb), currentRegistryID);
						}
						closePopUp(popup_id);
					}
					if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
						closePopUp(popup_id);
					}
				};
			}

			void ComponentsPanel::renderEntityComponents(entt::entity entity) {
				auto ecs = services->get<ECS::Controller>();

				if (!ecs || !ecs->checkEntity(entity, currentRegistryID)) {
					ImGui::Spacing();
					ImGui::TextDisabled("Invalid entity");
					return;
				}

				// Get all component names for this entity
				auto component_names =
					ecs->getEntityComponentNames(entity, currentRegistryID);

				if (component_names.empty()) {
					ImGui::Spacing();
					ImGui::TextDisabled("No components attached");
					return;
				}

				for (const auto& comp_name : component_names) {
					if (comp_name == getComponentName<Entity::Name>() ||
						comp_name == getComponentName<Entity::Layer>() ||
						comp_name == getComponentName<MetaData::EditorVisible>()) {
						continue;
					}

					ImGui::PushID(comp_name.c_str());
					// ========================================
					// Check if this component is overridden
					// ========================================
					bool isOverridden = false;
					auto prefabService = services->get<Prefab::Service>();
					auto& registry = ecs->getRegistry(currentRegistryID);

					if (prefabService && registry.any_of<Prefab::PrefabInstance>(entity)) {
						isOverridden = prefabService->isComponentOverridden(entity, comp_name,
																			currentRegistryID);
					}
					// ========================================
					// Apply special styling for overridden components
					// ========================================
					if (isOverridden) {
						// Blue color scheme for overridden components
						ImGui::PushStyleColor(ImGuiCol_Header,
											  ImVec4(0.2f, 0.4f, 0.8f, 1.0f)); // Base blue
						ImGui::PushStyleColor(ImGuiCol_HeaderHovered,
											  ImVec4(0.3f, 0.5f, 0.9f, 1.0f)); // Lighter on hover
						ImGui::PushStyleColor(
							ImGuiCol_HeaderActive,
							ImVec4(0.15f, 0.35f, 0.7f, 1.0f)); // Darker when active
					}
					// Component header with TreeNode
					ImGuiTreeNodeFlags node_flags =
						ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
						ImGuiTreeNodeFlags_SpanAvailWidth |
						ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
					bool node_open = ImGui::TreeNodeEx(comp_name.c_str(), node_flags);
					ImGui::PopStyleVar();
					// ========================================
					// Pop override colors
					// ========================================
					if (isOverridden) {
						ImGui::PopStyleColor(3);
					}
					// ========================================
					// Add revert button for overridden components
					// ========================================
					if (isOverridden) {
						ImGui::SameLine();
						ImGui::Text("(Override)");
						ImGui::SameLine();
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.35f, 0.2f, 0.8f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
											  ImVec4(0.8f, 0.45f, 0.25f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive,
											  ImVec4(0.5f, 0.25f, 0.15f, 1.0f));
						if (ImGui::SmallButton("Revert")) {
							if (prefabService) {
								prefabService->revertComponentOverride(entity, comp_name,
																	   currentRegistryID);
							}
						}

						ImGui::PopStyleColor(3);

						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("Revert to prefab value");
						}
					}
					// Right-click context menu
					if (ImGui::BeginPopupContextItem()) {
						ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s",
										   comp_name.c_str());

						// ========================================
						// Show override status in context menu
						// ========================================
						if (isOverridden) {
							ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "(Overridden)");
						}

						ImGui::Separator();
						if (ImGui::MenuItem("Remove Component")) {
							comp_string_ref = comp_name;
							should_open_remove_popup = true;
							ImGui::CloseCurrentPopup();
						}
						// ========================================
						// Different reset behavior for prefab instances
						// ========================================
						if (registry.any_of<Prefab::PrefabInstance>(entity)) {
							if (isOverridden) {
								// If overridden, show "Revert to Prefab"
								if (ImGui::MenuItem("Revert to Prefab")) {
									if (prefabService) {
										prefabService->revertComponentOverride(entity, comp_name,
																			   currentRegistryID);
									}
								}
							} else {
								// If not overridden, disabled (already using prefab value)
								ImGui::BeginDisabled();
								ImGui::MenuItem("Already Using Prefab Value");
								ImGui::EndDisabled();
							}
						} else {
							// Regular entity - show "Reset to Default"
							if (ImGui::MenuItem("Reset to Default")) {
								ecs->removeComponentByName(entity, comp_name, currentRegistryID);
								ecs->addComponentByName(entity, comp_name, currentRegistryID);
							}
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Copy Component")) {
							ImGui::SetClipboardText(comp_name.c_str());
						}
						if (ImGui::MenuItem("Paste Component Values")) {
							// TODO: Deserialize from clipboard
						}
						ImGui::EndPopup();
					}

					if (node_open) {
						ImGui::Spacing();

						// Get component pointer (type-erased)
						void* comp_ptr =
							ecs->getComponentPtrByName(entity, comp_name, currentRegistryID);

						// Start drag-and-drop source
						if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {

							// Set drag payload with asset name
							ImGui::SetDragDropPayload(std::string(comp_name + "_COMP").c_str(),
													  &comp_ptr, sizeof(comp_ptr));

							// Render the icon or name at the cursor during dragging
							ImGui::Text("%s", comp_name.c_str());
							ImGui::EndDragDropSource();
						}

						// Render component-specific UI
						if (comps_ui.find(comp_name) != comps_ui.end() && comp_ptr) {
							ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
							comps_ui.at(comp_name)(*this, comp_ptr);
							ImGui::PopStyleVar();
						} else {
							ImGui::TextDisabled("No UI registered for this component");
						}

						ImGui::Spacing();

						// Remove Component Button
						ImGui::Separator();
						ImGui::Spacing();

						std::string componentName = comp_name;

						// Center the button
						float buttonWidth = 150.0f;
						float availWidth = ImGui::GetContentRegionAvail().x;
						float offset = (availWidth - buttonWidth) * 0.5f;
						if (offset > 0) {
							ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);
						}

						// Red remove button
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.2f, 0.2f, 0.8f));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

						if (ImGui::Button("Remove Component", ImVec2(buttonWidth, 0))) {
							comp_string_ref = componentName;
							should_open_remove_popup = true;
						}

						ImGui::PopStyleColor(3);

						if (ImGui::IsItemHovered()) {
							ImGui::SetTooltip("Remove this component from the entity");
						}

						ImGui::Spacing();
						ImGui::TreePop();
					}

					ImGui::PopID();
				}
			}

			void ComponentsPanel::setCompStringRef(std::string const& to_set) {
				comp_string_ref = to_set;
			}

			void ComponentsPanel::onUpdate(AppTiming timing) {

				auto ecs = services->get<ECS::Controller>();
				if (!ecs) {
					ImGui::Spacing();
					ImGui::TextDisabled("ECS Controller unavailable");
					return;
				}

				// Rebuild component name set
				for (const auto& [comp_name, factory] : ecs->getComponentFactories()) {
					// Register default UI handlers for any components that don't have one yet
					if (comps_ui.find(comp_name) == comps_ui.end()) {
						comps_ui.emplace(comp_name, [](ComponentsPanel&, void*) {
							ImGui::TextDisabled("No UI registered for this component");
						});
					}
				}

				// Ensure entity panel reference is valid
				auto entity_panel = entities_panel.lock();
				if (!entity_panel) {
					// Recover weak_ptr if it expired (happens on panel reload/scene change)
					auto editor = services->get<PAIN::Editor::Editor>();
					if (editor) {
						auto ep = editor->getPanel<Panel::EntityPanel>();
						if (ep) {
							entities_panel = ep;
							entity_panel = ep;
						}
					}
				}

				if (!entity_panel) {
					ImGui::Spacing();
					ImGui::TextDisabled("Entity Panel not available");
					return;
				}
				// Get selected entity
				entt::entity selected = entity_panel->getSelectedEntity();

				// No entity selected - show placeholder
				if (selected == entt::null ||
					!ecs->checkEntity(selected, currentRegistryID)) {

					ImGui::Spacing();
					ImGui::Spacing();

					// Centered placeholder text
					const char* placeholder = "Select an entity to inspect";
					float text_width = ImGui::CalcTextSize(placeholder).x;
					float window_width = ImGui::GetContentRegionAvail().x;
					float offset = (window_width - text_width) * 0.5f;
					if (offset > 0)
						ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

					ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", placeholder);
					return;
				}

				ImGui::Spacing();
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
				ImGui::Text("Entity Properties");
				ImGui::PopStyleColor();

				ImGui::Spacing();

				// Display entity name from metadata
				std::string entity_name;
				auto name_comp_opt =
					services->get<ECS::Controller>()->getEntityComponent<Entity::Name>(
						selected, currentRegistryID);
				if (!name_comp_opt.has_value()) {
					entity_name = services->get<ECS::Controller>()
									  ->getRegistry(currentRegistryID)
									  .emplace<Entity::Name>(selected)
									  .name;

				} else {
					entity_name = name_comp_opt->get().name;
				}

				// Checkbox
				static bool checkbox = true;
				ImGui::PushID("Chkbox");
				if (ImGui::Checkbox("", &checkbox)) {
					// logic for checkbox here
				}
				ImGui::PopID();
				ImGui::SameLine(0, 8);

				// Entity Name
				char name_buf[128];
				strncpy(name_buf, entity_name.c_str(), sizeof(name_buf));
				name_buf[sizeof(name_buf) - 1] = '\0';

				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
				if (ImGui::InputText("##entityName", name_buf, sizeof(name_buf),
									 ImGuiInputTextFlags_EnterReturnsTrue)) {
					std::string new_name(name_buf);
					if (name_comp_opt.has_value())
						name_comp_opt.value().get().name = new_name;
				}

				//// Tag Dropdown
				// ImGui::Separator();
				// ImGui::Spacing();
				// ImGui::SameLine(0, 20);
				// std::string tag_value = "Untagged";
				// auto curr_tags = metadata->getRegisteredTags();
				// if (!curr_tags.empty()) {
				//     std::vector<const char*> tag_items;
				//     for (const auto& tag : curr_tags)
				//         tag_items.push_back(tag.c_str());

				//    for (const auto& tag : curr_tags)
				//        if (metadata->hasTag(selected, tag))
				//            tag_value = tag;

				//    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
				//    if (ImGui::BeginCombo("##TagCombo", tag_value.c_str())) {
				//        for (size_t i = 0; i < tag_items.size(); ++i) {
				//            bool is_selected = (tag_value == tag_items[i]);
				//            if (ImGui::Selectable(tag_items[i], is_selected)) {
				//                metadata->setEntityTag(selected, tag_items[i]);
				//            }
				//            if (is_selected) ImGui::SetItemDefaultFocus();
				//        }
				//        ImGui::EndCombo();
				//    }
				//}
				// ImGui::SameLine(0, 5);
				// ImGui::Text("Tag");

				// Layer Dropdown
				// ImGui::Spacing();
				// Get scn service

				ImGui::Spacing();

				// Get ecs controller
				auto controller = services->get<ECS::Controller>();
				if (auto layerCompOpt = controller->getEntityComponent<Entity::Layer>(
						selected, currentRegistryID)) {
					if (layerCompOpt.has_value() &&
						currentRegistryID == ECS::MAIN_REGISTRY_ID) {

						auto layerComp = layerCompOpt.value();
						auto sceneManager = services->get<Scene::SceneManager>();
						const auto& layers = sceneManager->getLayers();

						// Layer dropdown
						const char* currentLayerName =
							layers[layerComp.get().layer_id].name.c_str();
						if (ImGui::BeginCombo("Layer", currentLayerName)) {
							for (size_t i = 0; i < layers.size(); ++i) {
								bool i_selected = (layerComp.get().layer_id == i);

								// Color indicator
								ImGui::PushStyleColor(ImGuiCol_Text,
													  ImVec4(layers[i].color.x, layers[i].color.y,
															 layers[i].color.z, 1.0f));

								if (ImGui::Selectable(std::string(layers[i].name + "##" +
																  std::to_string(layers[i].id))
														  .c_str(),
													  i_selected)) {

									// Propogate layer tag
									std::function<void(entt::entity)> propogate_layer_tag =
										[&](entt::entity entity) {
											// Mark world transform as dirty
											if (auto* layer = controller->getRegistry(currentRegistryID)
																  .try_get<Entity::Layer>(entity)) {
												layer->layer_id = i;
												layer->layer_mask = 1 << i;
												layer->layerName = layers[i].name;
											}

											// Get hierarchy and propagate to children if exists
											if (auto* hierarchy =
													controller->getRegistry(currentRegistryID)
														.try_get<Entity::Hierarchy>(entity)) {
												for (const auto& childGUID : hierarchy->childrenGUIDs) {
													entt::entity child =
														controller->getGUIDRegistry(currentRegistryID)
															.resolveGUID(childGUID);
													if (child != entt::null &&
														controller->getRegistry(currentRegistryID)
															.valid(child)) {
														propogate_layer_tag(child);
													}
												}
											}
										};

									// Root entity
									entt::entity root_entity = selected;
									if (auto* id = controller->getRegistry(currentRegistryID)
													   .try_get<Entity::GUID>(root_entity)) {
										Assets::GUID root_id = id->guid;

										// Identify absolute root
										while (root_id.IsValid()) {
											if (auto* hierarchy =
													controller->getRegistry(currentRegistryID)
														.try_get<Entity::Hierarchy>(root_entity)) {
												root_entity =
													controller->resolveGUID(root_id, currentRegistryID);
												root_id = hierarchy->parentGUID;
											} else {
												break;
											}
										}

										// Propogate down
										propogate_layer_tag(root_entity);
									}
								}

								ImGui::PopStyleColor();

								if (i_selected) {
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						// Show current mask (debug)
						ImGui::Text("Bitmask: 0x%08X", layerComp.get().layer_mask);
					}
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
				ImGui::Text("Entity Components");
				ImGui::PopStyleColor();
				ImGui::Spacing();

				// if (metadata->isLocked(selected)) {
				//     ImGui::SameLine();
				//     ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "[LOCKED]");
				//     ImGui::Separator();
				//     ImGui::Spacing();
				//     ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
				//     ImGui::Text("Entity is locked");
				//     ImGui::PopStyleColor();
				//     ImGui::Spacing();
				//     ImGui::TextWrapped("Unlock this entity in the Entity Panel to edit its
				//     components."); return;
				// }
				// ImGui::Spacing();

				renderEntityComponents(selected);

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();

				ImVec2 button_size = ImVec2(ImGui::GetContentRegionAvail().x, 35);

				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.8f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.4f, 0.7f, 1.0f));

				if (ImGui::Button("+ Add Component", button_size)) {
					openPopUp("AddComponent");
				}

				ImGui::PopStyleColor(3);

				// Open remove popup after context menu closes (prevents ImGui state
				// conflicts)
				if (should_open_remove_popup) {
					openPopUp("RemoveComponent");
					should_open_remove_popup = false;
				}

				renderPopUps();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
