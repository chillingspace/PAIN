/*****************************************************************//**
 * \file   sysUIInput.cpp
 * \brief  Declaration of UI layout system
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysUIInput.h"
#include "ECS/Controller.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Scripting/luaManager.h"
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/Android/TouchEvents.h"
#include "CoreSystems/Haptics/Haptics.h"

#ifdef PN_PLATFORM_WINDOWS
#include "imgui.h"
#include "LayeredSystems/LevelEditor/Editor.h"
#endif

namespace PAIN {

	static const char* ToActionName(UIAction action) {
		switch (action) {
		case UIAction::game_Jump:              return "game_Jump";
		case UIAction::game_Hide:              return "game_Hide";
		case UIAction::game_Collect:           return "game_Collect";
		case UIAction::game_Move:              return "game_Move";
		case UIAction::game_End:               return "game_End";

		case UIAction::pause_Resume:          return "pause_Resume";
		case UIAction::pause_Restart:         return "pause_Restart";
		case UIAction::pause_Quit:            return "pause_Quit";

		case UIAction::restart_Confirm:        return "restart_Confirm";
		case UIAction::restart_Cancel:         return "restart_Cancel";

		case UIAction::menu_QuitGame:          return "menu_QuitGame";
		case UIAction::quit_Confirm:           return "quit_Confirm";
		case UIAction::quit_Cancel:            return "quit_Cancel";

		case UIAction::goto_Pause:             return "goto_Pause";

		case UIAction::cutscene_Open_Menu:     return "cutscene_Open_Menu";
		case UIAction::cutscene_Close_Menu:    return "cutscene_Close_Menu";
		case UIAction::cutscene_Menu_Quit:     return "cutscene_Menu_Quit";
		case UIAction::cutscene_Quit_Confirm:  return "cutscene_Quit_Confirm";
		case UIAction::cutscene_Quit_Cancel:   return "cutscene_Quit_Cancel";

		case UIAction::mainmenu_Quit_Confirm:  return "mainmenu_Quit_Confirm";
		case UIAction::mainmenu_Quit_Cancel:   return "mainmenu_Quit_Cancel";

		case UIAction::LoadScene:              return "LoadScene";

		case UIAction::howtoplay_Right:		   return "howtoplay_Right";
		case UIAction::howtoplay_Left:		   return "howtoplay_Left";

		case UIAction::graphics_Left:		   return "graphics_Left";
		case UIAction::graphics_Right:		   return "graphics_Right";

		case UIAction::mainmenu_Next:		   return "mainmenu_Next";
		case UIAction::settings_Back:		   return "settings_Back";
		case UIAction::settings_Sub_Back:	   return "settings_Sub_Back";
		case UIAction::settings_Next:		   return "settings_Next";
		case UIAction::to_MainMenu:			   return "to_MainMenu";
		case UIAction::credits_Next:		   return "credits_Next";
		case UIAction::howtoplay_Next:		   return "howtoplay_Next";
		case UIAction::reset_Graphics_Settings: return "reset_Graphics_Settings";
		case UIAction::rebind_Forward:          return "rebind_Forward";
		case UIAction::rebind_Left:             return "rebind_Left";
		case UIAction::rebind_Right:            return "rebind_Right";
		case UIAction::rebind_Backward:         return "rebind_Backward";
		case UIAction::rebind_Jump:             return "rebind_Jump";
		case UIAction::rebind_Hide:             return "rebind_Hide";
		case UIAction::reset_Controls:          return "reset_Controls";
		
		case UIAction::None:
		default:                               return "None";
		}
	}

	namespace UI {

		InputSystem::InputSystem(std::shared_ptr<Services> svc) : ISystem(svc) {}

		InputSystem::~InputSystem() {}

		void InputSystem::onUpdate(AppTiming timing, entt::registry& registry) {
			auto hit_entity = raycastUI(m_mouse_position, registry);

			// Update hover states
			if (hit_entity.has_value() && hit_entity.value() != m_hovered_entity) {
				// Clear old hover
				if (m_hovered_entity != entt::null && registry.valid(m_hovered_entity)) {
					updateButtonState(m_hovered_entity, registry, UIButtonState::Normal);
				}

				// Set new hover
				m_hovered_entity = hit_entity.value();
				if (m_pressed_entity == entt::null) {
					updateButtonState(m_hovered_entity, registry, UIButtonState::Highlighted);
				}
			}
			else if (!hit_entity.has_value() && m_hovered_entity != entt::null) {
				// Mouse left all UI
				if (registry.valid(m_hovered_entity)) {
					updateButtonState(m_hovered_entity, registry, UIButtonState::Normal);
				}
				m_hovered_entity = entt::null;
			}
		}

		void InputSystem::onEvent(Event::Event& event) {
#ifdef PN_PLATFORM_WINDOWS
			// ── Mouse movement ──
			if (event.getType() == Event::Type::MouseMove) {
				Event::Dispatcher dispatcher(event);
				dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {
					m_mouse_position = convertToCenterOrigin(e.getWindowPos());

					// Handle joystick drag - ONLY if entity has UIJoystick component
					if (m_pressed_entity != entt::null) {
						auto ecs = services.lock()->get<ECS::Controller>();
						auto& registry = ecs->getRegistry();

						if (registry.valid(m_pressed_entity) &&
							registry.all_of<UIJoystick>(m_pressed_entity)) {
							auto& joystick = registry.get<UIJoystick>(m_pressed_entity);
							auto& tex = registry.get<Texture2D>(m_pressed_entity);

							glm::vec2 normalized_mouse = normalizeScreenPosition(m_mouse_position);
							glm::vec2 drag_offset = normalized_mouse - joystick.center_position;
							float distance = glm::length(drag_offset);

							// Start drag if moved beyond threshold
							const float drag_threshold = 0.01f;
							if (distance > drag_threshold && !joystick.is_dragging) {
								joystick.is_dragging = true;
							}

							// If dragging, update position and call callback
							if (joystick.is_dragging) {
								// Clamp to max radius
								if (distance > joystick.max_radius) {
									drag_offset = glm::normalize(drag_offset) * joystick.max_radius;
									distance = joystick.max_radius;
								}

								// Update joystick visual position
								tex.pos = joystick.center_position + drag_offset;

								// Calculate normalized direction (-1 to 1 range)
								glm::vec2 direction(0.f, 0.f);
								if (distance > 0.001f) {
									direction = drag_offset / joystick.max_radius;
								}

								// Call Lua callback with direction
								auto& button = registry.get<UIButton>(m_pressed_entity);
								if (button.action == UIAction::game_Move) {
									// Payload format: "dirX,dirY"
									button.payload = std::to_string(direction.x) + "," + std::to_string(direction.y);
									dispatchUIAction(m_pressed_entity, registry, UIAction::game_Move);
								}
							}
						}
					}
					return false;
					});
			}

			// ── Skip input if editor wants the mouse ──
#ifdef _DEBUG
			if (ImGui::GetCurrentContext()) {
				ImGuiIO& io = ImGui::GetIO();
				if (io.WantCaptureMouse) return;
			}
			if (auto editor = services.lock()->get<Editor::Editor>()) {
				if (editor->isVisible()) return;
			}
#endif

			// ── Mouse button press ──
			if (event.getType() == Event::Type::MouseButtonPress) {
				Event::Dispatcher dispatcher(event);
				dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
					if (e.getBtnCode() != 0) return false; // Left click only

					if (m_hovered_entity != entt::null) {
						m_pressed_entity = m_hovered_entity;
						auto ecs = services.lock()->get<ECS::Controller>();
						auto& registry = ecs->getRegistry();

						// Store center position ONLY if this is a joystick
						if (registry.all_of<UIJoystick>(m_pressed_entity)) {
							auto& joystick = registry.get<UIJoystick>(m_pressed_entity);
							auto& tex = registry.get<Texture2D>(m_pressed_entity);
							joystick.center_position = tex.pos;
							joystick.is_dragging = false;
						}

						updateButtonState(m_pressed_entity, registry, UIButtonState::Pressed);
						PN_CORE_INFO("[UIInput] MouseDown on UI entity id = {}",
							static_cast<uint32_t>(m_pressed_entity));

						// Haptic feedback on button press (Android only, Windows stub does nothing)
						if (auto haptics = services.lock()->get<Haptics::Haptics>()) {
							if (haptics->hasHaptics()) {
								haptics->tick();
							}
						}

						return true; // UI consumed event
					}
					return false;
					});
			}

			// ── Mouse button release ──
			if (event.getType() == Event::Type::MouseButtonRelease) {
				Event::Dispatcher dispatcher(event);
				dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {
					if (e.getBtnCode() != 0) return false;

					if (m_pressed_entity != entt::null) {
						auto ecs = services.lock()->get<ECS::Controller>();
						auto& registry = ecs->getRegistry();
						bool was_joystick_drag = false;

						// If this was a joystick, reset it
						if (registry.valid(m_pressed_entity) &&
							registry.all_of<UIJoystick>(m_pressed_entity)) {
							auto& joystick = registry.get<UIJoystick>(m_pressed_entity);
							was_joystick_drag = joystick.is_dragging;

							if (was_joystick_drag) {
								// Reset joystick to center
								auto& tex = registry.get<Texture2D>(m_pressed_entity);
								tex.pos = joystick.center_position;

								// Notify Lua that movement stopped
								auto& button = registry.get<UIButton>(m_pressed_entity);
								if (button.action == UIAction::game_Move) {
									// CHECK IF BUTTON'S LAYER IS ENABLED
									bool should_process = true;
									if (registry.all_of<Entity::Layer>(m_pressed_entity)) {
										auto& layer_comp = registry.get<Entity::Layer>(m_pressed_entity);
										auto scene = services.lock()->get<Scene::SceneManager>();
										if (scene && !scene->isLayerEnabled(layer_comp.layer_id)) {
											/*PN_CORE_INFO("[UIInput] Joystick on disabled layer {}, ignoring",
												layer_comp.layer);*/
											should_process = false;
										}
									}

									// should process - added
									if (should_process) {
										button.payload = "0.0,0.0";
										dispatchUIAction(m_pressed_entity, registry, UIAction::game_Move);
									}
								}
								joystick.is_dragging = false;
							}
						}

						// Normal button click: go through activateButton (UIAction + fallback lua callback)
						if (!was_joystick_drag &&
							m_pressed_entity == m_hovered_entity &&
							registry.all_of<UIButton>(m_pressed_entity)) {

							bool should_process = true;
							if (registry.all_of<Entity::Layer>(m_pressed_entity)) {
								auto& layer_comp = registry.get<Entity::Layer>(m_pressed_entity);
								auto scene = services.lock()->get<Scene::SceneManager>();
								if (scene && !scene->isLayerEnabled(layer_comp.layer_id)) {
									should_process = false;
								}
							}

							if (should_process) {
								auto& button = registry.get<UIButton>(m_pressed_entity);
								PN_CORE_INFO("[UIInput] MouseUp: pressed={:08X} hovered={:08X} action={}",
									static_cast<uint32_t>(m_pressed_entity),
									static_cast<uint32_t>(m_hovered_entity),
									ToActionName(button.action));

								activateButton(m_pressed_entity, registry);

								// Haptic feedback on button click
								if (auto haptics = services.lock()->get<Haptics::Haptics>()) {
									if (haptics->hasHaptics()) {
										haptics->click();
									}
								}
							}
						}

						updateButtonState(m_pressed_entity,
							registry,
							m_pressed_entity == m_hovered_entity ?
							UIButtonState::Highlighted : UIButtonState::Normal
						);
						m_pressed_entity = entt::null;
						return true;
					}
					return false;
					});
			}

#else
			// ══════════════════════════════════════════════════════════════════
			// Android Touch Events with Full Multi-Touch Support
			// ══════════════════════════════════════════════════════════════════

			// ── TouchMove ──
			if (event.getType() == Event::Type::TouchMove) {
				Event::Dispatcher dispatcher(event);
				dispatcher.Dispatch<Event::TouchMove>([&](Event::TouchMove& e) -> bool {
					glm::vec2 raw_touch(e.getX(), e.getY());
					m_mouse_position = convertToCenterOrigin(raw_touch);

					int pointer_id = e.getPointerId();

					// Look up the entity for this pointer ID
					auto it = m_active_touches.find(pointer_id);
					if (it == m_active_touches.end()) {
						return false; // No active touch for this pointer
					}

					entt::entity touch_entity = it->second;
					auto ecs = services.lock()->get<ECS::Controller>();
					auto& registry = ecs->getRegistry();

					// Check if this entity is still valid
					if (!registry.valid(touch_entity)) {
						m_active_touches.erase(it);
						return false;
					}

					// Handle joystick movement
					if (registry.all_of<UIJoystick>(touch_entity)) {
						auto& joystick = registry.get<UIJoystick>(touch_entity);
						auto& tex = registry.get<Texture2D>(touch_entity);

						glm::vec2 normalized_touch = normalizeScreenPosition(m_mouse_position);
						glm::vec2 drag_offset = normalized_touch - joystick.center_position;
						float distance = glm::length(drag_offset);

						// Start dragging if threshold exceeded
						const float drag_threshold = 0.01f;
						if (!joystick.is_dragging && distance > drag_threshold) {
							joystick.is_dragging = true;
							m_joystick_pointer_id = pointer_id;
						}

						// If dragging, update position
						if (joystick.is_dragging) {
							// Clamp to max radius
							if (distance > joystick.max_radius) {
								drag_offset = glm::normalize(drag_offset) * joystick.max_radius;
								distance = joystick.max_radius;
							}

							// Update joystick visual position
							tex.pos = joystick.center_position + drag_offset;

							// Calculate normalized direction
							glm::vec2 direction(0.f, 0.f);
							if (distance > 0.001f) {
								direction = drag_offset / joystick.max_radius;
							}

							// Call Lua callback
							auto& button = registry.get<UIButton>(touch_entity);
							if (button.action == UIAction::game_Move) {
								// Payload format: "dirX,dirY"
								button.payload = std::to_string(direction.x) + "," + std::to_string(direction.y);
								dispatchUIAction(touch_entity, registry, UIAction::game_Move);
							}
							return true;
						}
					}

					return false;
					});
			}

			// ── TouchDown ──
			if (event.getType() == Event::Type::TouchDown) {
				Event::Dispatcher dispatcher(event);
				dispatcher.Dispatch<Event::TouchDown>([&](Event::TouchDown& e) -> bool {
					m_mouse_position = convertToCenterOrigin(glm::vec2(e.getX(), e.getY()));
					auto ecs = services.lock()->get<ECS::Controller>();
					auto& registry = ecs->getRegistry();
					auto hit_entity = raycastUI(m_mouse_position, registry);

					if (hit_entity.has_value()) {
						int pointer_id = e.getPointerId();
						entt::entity entity = hit_entity.value();

						PN_CORE_INFO("[UIInput] TouchDown: pointer={}, entity={}",
							pointer_id, static_cast<uint32_t>(entity));

						// Store this touch in the map
						m_active_touches[pointer_id] = entity;

						// Initialize joystick if this is a joystick entity
						if (registry.all_of<UIJoystick>(entity)) {
							auto& joystick = registry.get<UIJoystick>(entity);
							auto& tex = registry.get<Texture2D>(entity);
							joystick.center_position = tex.pos;
							joystick.is_dragging = false;

							PN_CORE_INFO("[UIInput] Joystick initialized for pointer {}", pointer_id);
						}

						updateButtonState(entity, registry, UIButtonState::Pressed);

						// Haptic feedback on button press
						if (auto haptics = services.lock()->get<Haptics::Haptics>()) {
							if (haptics->hasHaptics()) {
								haptics->tick();
							}
						}

						return true;
					}
					return false;
					});
			}

			// ── TouchUp ──
			if (event.getType() == Event::Type::TouchUp) {
				Event::Dispatcher dispatcher(event);
				dispatcher.Dispatch<Event::TouchUp>([&](Event::TouchUp& e) -> bool {
					int pointer_id = e.getPointerId();

					// Look up the entity for this pointer
					auto it = m_active_touches.find(pointer_id);
					if (it == m_active_touches.end()) {
						return false; // No active touch for this pointer
					}

					entt::entity touch_entity = it->second;
					auto ecs = services.lock()->get<ECS::Controller>();
					auto& registry = ecs->getRegistry();

					if (!registry.valid(touch_entity)) {
						m_active_touches.erase(it);
						return false;
					}

					PN_CORE_INFO("[UIInput] TouchUp: pointer={}, entity={}",
						pointer_id, static_cast<uint32_t>(touch_entity));

					// Handle joystick release
					if (registry.all_of<UIJoystick>(touch_entity)) {
						auto& joystick = registry.get<UIJoystick>(touch_entity);
						auto& tex = registry.get<Texture2D>(touch_entity);

						// Reset joystick to center
						tex.pos = joystick.center_position;

						// Stop movement - call Lua with zero vector
						auto& button = registry.get<UIButton>(touch_entity);
						if (button.action == UIAction::game_Move) {
							// CHECK IF BUTTON'S LAYER IS ENABLED (Android)
							bool should_process = true;
							if (registry.all_of<Entity::Layer>(touch_entity)) {
								auto& layer_comp = registry.get<Entity::Layer>(touch_entity);
								auto scene = services.lock()->get<Scene::SceneManager>();
								if (scene && !scene->isLayerEnabled(layer_comp.layer_id)) {
									PN_CORE_INFO("[UIInput] Joystick on disabled layer {}, ignoring",
										layer_comp.layer_id);
									should_process = false;
								}
							}

							if (should_process) {
								button.payload = "0.0,0.0";
								dispatchUIAction(touch_entity, registry, UIAction::game_Move);
							}
						}

						joystick.is_dragging = false;

						// Clear joystick pointer ID if this was the joystick
						if (m_joystick_pointer_id == pointer_id) {
							m_joystick_pointer_id = -1;
						}

						updateButtonState(touch_entity, registry, UIButtonState::Normal);
					}
					// Handle regular button click
					else if (registry.all_of<UIButton>(touch_entity)) {
						m_mouse_position = convertToCenterOrigin(glm::vec2(e.getX(), e.getY()));
						auto hit_entity = raycastUI(m_mouse_position, registry);

						// Only trigger click if finger is still on the button
						if (hit_entity.has_value() && hit_entity.value() == touch_entity) {
							PN_CORE_INFO("[UIInput] Button click detected");

							bool should_process = true;
							if (registry.all_of<Entity::Layer>(touch_entity)) {
								auto& layer_comp = registry.get<Entity::Layer>(touch_entity);
								auto scene = services.lock()->get<Scene::SceneManager>();
								if (scene && !scene->isLayerEnabled(layer_comp.layer_id)) {
									PN_CORE_INFO("[UIInput] Button on disabled layer {}, ignoring click",
										layer_comp.layer_id);
									should_process = false;
								}
							}

							if (should_process) {
								// go through activateButton so UIAction works + fallback lua works
								activateButton(touch_entity, registry);

								// Haptic feedback on button click
								if (auto haptics = services.lock()->get<Haptics::Haptics>()) {
									if (haptics->hasHaptics()) {
										haptics->click();
									}
								}
							}
						}

						updateButtonState(
							touch_entity,
							registry,
							(hit_entity.has_value() && hit_entity.value() == touch_entity)
							? UIButtonState::Highlighted
							: UIButtonState::Normal
						);
					}

					// Remove this touch from the map
					m_active_touches.erase(it);
					return true;
					});
			}

			// ── TouchCancel ──
			if (event.getType() == Event::Type::TouchCancel) {
				Event::Dispatcher dispatcher(event);
				dispatcher.Dispatch<Event::TouchCancel>([&](Event::TouchCancel& e) -> bool {
					int pointer_id = e.getPointerId();

					// Look up the entity for this pointer
					auto it = m_active_touches.find(pointer_id);
					if (it == m_active_touches.end()) {
						return false;
					}

					entt::entity touch_entity = it->second;
					auto ecs = services.lock()->get<ECS::Controller>();
					auto& registry = ecs->getRegistry();

					if (registry.valid(touch_entity)) {
						// Reset joystick if it was being dragged
						if (registry.all_of<UIJoystick>(touch_entity)) {
							auto& joystick = registry.get<UIJoystick>(touch_entity);
							auto& tex = registry.get<Texture2D>(touch_entity);
							tex.pos = joystick.center_position;
							joystick.is_dragging = false;

							// Clear joystick pointer if this was it
							if (m_joystick_pointer_id == pointer_id) {
								m_joystick_pointer_id = -1;
							}
						}

						updateButtonState(touch_entity, registry, UIButtonState::Normal);
					}

					// Remove from map
					m_active_touches.erase(it);
					return true;
					});
			}

#endif
		}


		// ══════════════════════════════════════════════════════════════════
		// Raycast UI
		// ══════════════════════════════════════════════════════════════════
		std::optional<entt::entity> InputSystem::raycastUI(const glm::vec2& mouse_pos, entt::registry& registry) {
			auto ecs = services.lock()->get<ECS::Controller>();
			glm::vec2 normalized_mouse = normalizeScreenPosition(mouse_pos);
			auto svc = services.lock();
			auto window = svc->get<Window::Window>();
			glm::vec2 fb = window ? glm::vec2(window->getFrameBuffer()) : glm::vec2(1.0f);

			// Query all UI elements with Texture2D (the actual rendered texture holds position)
			auto view = registry.view<Texture2D, UIElement, UIRectTransform>();
			std::vector<std::tuple<entt::entity, int, int>> candidates;

			auto scene = services.lock()->get<Scene::SceneManager>();

			for (auto [entity, tex, element, rect] : view.each()) {
				if (!element.b_is_enabled || !element.b_is_interactable) continue;

				// Skip if layer is disabled
				if (registry.all_of<Entity::Layer>(entity)) {
					const auto& layer = registry.get<Entity::Layer>(entity);
					if (scene && !scene->isLayerEnabled(layer.layer_id)) {
						continue;
					}
				}

				glm::vec2 rect_min, rect_max;

				// Check for custom hitbox first
				if (registry.all_of<CustomHitbox2D>(entity)) {
					const auto& hitbox = registry.get<CustomHitbox2D>(entity);

					// Hitbox position = texture position + offset
					glm::vec2 hitbox_center = tex.pos + normalizeSize(hitbox.position_offset, services);

					// Convert hitbox bounds to normalized space
					glm::vec2 normalized_min = normalizeSize(hitbox.min_point, services);
					glm::vec2 normalized_max = normalizeSize(hitbox.max_point, services);

					// Calculate absolute bounds
					rect_min = hitbox_center + normalized_min;
					rect_max = hitbox_center + normalized_max;
				}
				else {
					// Default behavior: use texture bounds
					// Calculate actual frame size (accounting for spritesheets)
					glm::vec2 frame_size = rect.size_delta;

					if (registry.all_of<UIAnimation>(entity)) {
						const auto& anim = registry.get<UIAnimation>(entity);
						if (anim.spritesheet_columns > 0 || anim.spritesheet_rows > 0) {
							// Get texture dimensions from asset manager
							auto texture_opt = services.lock()->get<Assets::Manager>()->getAsset<Assets::Texture>(tex.texture_guid);
							if (texture_opt.has_value()) {
								float width = static_cast<float>(texture_opt.value().get()->width);
								float height = static_cast<float>(texture_opt.value().get()->height);

								// Divide by spritesheet dimensions to get frame size
								if (anim.spritesheet_columns > 0) width /= anim.spritesheet_columns;
								if (anim.spritesheet_rows > 0) height /= anim.spritesheet_rows;
								frame_size = glm::vec2(width, height);
							}
						}
					}

					// Actual rendered size = base size * scale multiplier
					glm::vec2 actual_pixel_size = frame_size * tex.texture_scale;

					// Convert size to normalized space
					glm::vec2 normalized_size = normalizeSize(actual_pixel_size, services);
					rect_min = tex.pos - normalized_size;
					rect_max = tex.pos + normalized_size;
				}

				if (isPointInRect(normalized_mouse, rect_min, rect_max)) {

					// Get canvas sort order
					int canvas_sort = 0;
					entt::entity parent = entity;
					while (parent != entt::null && registry.all_of<Entity::Hierarchy>(parent)) {
						const auto& parentHierarchy = registry.get<Entity::Hierarchy>(parent);
						if (registry.all_of<UICanvas>(parent)) {
							canvas_sort = registry.get<UICanvas>(parent).sort_order;
							break;
						}
						if (!parentHierarchy.parentGUID.IsValid()) break;
						parent = ecs->resolveGUID(parentHierarchy.parentGUID);
					}

					// Get sibling index
					int sibling_index = 0;
					if (registry.all_of<Entity::Hierarchy>(entity)) {
						const auto& hierarchy = registry.get<Entity::Hierarchy>(entity);
						const auto& entity_guid = registry.get<Entity::GUID>(entity);

						if (hierarchy.parentGUID.IsValid()) {
							entt::entity parent_entity = ecs->resolveGUID(hierarchy.parentGUID);
							if (parent_entity != entt::null && registry.all_of<Entity::Hierarchy>(parent_entity)) {
								const auto& parentHierarchy = registry.get<Entity::Hierarchy>(parent_entity);
								auto it = std::find(parentHierarchy.childrenGUIDs.begin(),
									parentHierarchy.childrenGUIDs.end(), entity_guid.guid);
								if (it != parentHierarchy.childrenGUIDs.end()) {
									sibling_index = static_cast<int>(std::distance(parentHierarchy.childrenGUIDs.begin(), it));
								}
							}
						}
					}

					candidates.push_back({ entity, canvas_sort, sibling_index });
				}
			}

			// Query all UI elements with UIText
			auto textView = registry.view<UIText, UIElement, UIRectTransform>();
			PN_CORE_INFO("[UIInput] UIText raycast: mouse NDC = ({:.3f}, {:.3f}), fb = ({:.0f}, {:.0f})", normalized_mouse.x, normalized_mouse.y, fb.x, fb.y);
			int text_entity_count = 0;
			for (auto [entity, text_comp, element, rect] : textView.each()) {
				text_entity_count++;
				PN_CORE_INFO("[UIInput] UIText entity {:08X}: enabled={}, interactable={}, text='{}', text_pos=({:.1f}, {:.1f})",
					static_cast<uint32_t>(entity), element.b_is_enabled, element.b_is_interactable, text_comp.display_text, text_comp.text_pos.x, text_comp.text_pos.y);
				if (!element.b_is_enabled || !element.b_is_interactable) {
					PN_CORE_INFO("[UIInput]   -> skipped (not enabled or not interactable)");
					continue;
				}

				// Skip if layer is disabled
				if (registry.all_of<Entity::Layer>(entity)) {
					const auto& layer = registry.get<Entity::Layer>(entity);
					if (scene && !scene->isLayerEnabled(layer.layer_id)) {
						PN_CORE_INFO("[UIInput]   -> skipped (layer {} disabled)", layer.layer_id);
						continue;
					}
				}

				// Get text bounds in screen pixel coordinates
				// Position is top-left origin (designer coords), convert to bottom-left origin
				// to match the text renderer's glm::ortho projection
				glm::vec2 text_pos = text_comp.position;
				text_pos.y = fb.y - text_pos.y;
				PN_CORE_INFO("[UIInput]   text position (flipped)=({:.1f},{:.1f}), font_size={:.1f}, scale_factor={:.2f}",
					text_pos.x, text_pos.y, text_comp.font_size, text_comp.scale_factor);
				float text_width = 0.0f;
				float text_height = 0.0f;

				// Measure text width using font metrics
				auto font_opt = svc->get<Assets::Manager>()->getAsset<Assets::Fonts::FontFace>(text_comp.font_guid);
				if (!font_opt.has_value()) {
					PN_CORE_INFO("[UIInput]   -> skipped (font not found)");
					continue;
				}
				auto font = font_opt.value().get()->getFont();
				if (!font) {
					PN_CORE_INFO("[UIInput]   -> skipped (font null)");
					continue;
				}
				float final_scale = text_comp.font_size * text_comp.scale_factor;
				PN_CORE_INFO("[UIInput]   font_size={:.1f}, scale_factor={:.3f}, final_scale={:.3f}, line_height={:.2f}",
					text_comp.font_size, text_comp.scale_factor, final_scale, text_comp.line_height);

				// Measure widest line
				std::vector<std::string> lines;
				std::string curr_line, curr_word;
				float curr_width = 0.f, word_width = 0.f;
				float wrap_limit = text_comp.word_wrap ? text_comp.wrap_width : FLT_MAX;
				int length = (text_comp.max_length > 0) ?
					std::min<int>(text_comp.max_length, text_comp.display_text.length()) :
					text_comp.display_text.length();

				for (int i = 0; i < length; ++i) {
					char ch = text_comp.display_text[i];
					if (ch == ' ' || ch == '\n' || i == length - 1) {
						if (i == length - 1 && ch != '\n' && ch != ' ') curr_word.push_back(ch);

						word_width = 0.f;
						for (char c : curr_word) {
							if (font->glyphs.count(c)) {
								word_width += (font->glyphs[c].advance >> 6) * final_scale;
							}
						}

						if (curr_width + word_width > wrap_limit && !curr_line.empty()) {
							lines.push_back(curr_line);
							if (curr_width > text_width) text_width = curr_width;
							curr_line.clear();
							curr_width = 0.f;
						}
						curr_line += curr_word;
						curr_width += word_width;

						if (ch == ' ' || ch == '\n') {
							curr_line.push_back(ch);
							float space_width = 0.f;
							if (font->glyphs.count(' ')) {
								space_width = (font->glyphs[' '].advance >> 6) * final_scale;
							}
							curr_width += space_width;
						}
						if (ch == '\n') {
							lines.push_back(curr_line);
							if (curr_width > text_width) text_width = curr_width;
							curr_line.clear();
							curr_width = 0.f;
						}
						curr_word.clear();
					}
					else {
						curr_word.push_back(ch);
					}
				}
				if (!curr_line.empty()) {
					lines.push_back(curr_line);
					if (curr_width > text_width) text_width = curr_width;
				}
				if (lines.empty() && !text_comp.display_text.empty()) {
					text_width = 0.f;
					for (char c : text_comp.display_text) {
						if (font->glyphs.count(c)) {
							text_width += (font->glyphs[c].advance >> 6) * final_scale;
						}
					}
					lines.push_back(text_comp.display_text);
				}

				// Calculate text height using actual glyph dimensions (same as renderer)
				size_t num_lines = lines.empty() ? 1 : lines.size();
				
				// Find max glyph height across all characters (same as renderer uses)
				float max_glyph_height = 0.0f;
				for (const auto& line : lines) {
					for (char c : line) {
						if (font->glyphs.count(c)) {
							float gh = font->glyphs[c].size.y * final_scale;
							if (gh > max_glyph_height) max_glyph_height = gh;
						}
					}
				}
				if (max_glyph_height == 0.0f) {
					// Fallback: use first character or default
					for (char c : text_comp.display_text) {
						if (font->glyphs.count(c)) {
							max_glyph_height = font->glyphs[c].size.y * final_scale;
							break;
						}
					}
				}
				text_height = num_lines * max_glyph_height * text_comp.line_height;

				// Adjust width based on alignment
				float min_x = text_pos.x;
				float max_x = text_pos.x;

				if (text_comp.alignment == TextAlignment::Left) {
					min_x = text_pos.x - text_width;
					max_x = text_pos.x;
				}
				else if (text_comp.alignment == TextAlignment::Center) {
					min_x = text_pos.x - text_width * 0.5f;
					max_x = text_pos.x + text_width * 0.5f;
				}
				else {
					min_x = text_pos.x;
					max_x = text_pos.x + text_width;
				}

				float min_y = text_pos.y;
				float max_y = text_pos.y + text_height;

				PN_CORE_INFO("[UIInput]   text bounds (pixels): min=({:.1f},{:.1f}) max=({:.1f},{:.1f}), size=({:.1f},{:.1f}), alignment={}",
					min_x, min_y, max_x, max_y, text_width, text_height, (int)text_comp.alignment);

				// Convert screen pixel coords to NDC
				// Both text_pos and mouse NDC now use bottom-left origin
				// ortho(0, w, 0, h): x=0→-1, x=w→+1, y=0→-1, y=h→+1
				glm::vec2 rect_min_ndc, rect_max_ndc;
				rect_min_ndc.x = (min_x / fb.x) * 2.0f - 1.0f;
				rect_min_ndc.y = (min_y / fb.y) * 2.0f - 1.0f;
				rect_max_ndc.x = (max_x / fb.x) * 2.0f - 1.0f;
				rect_max_ndc.y = (max_y / fb.y) * 2.0f - 1.0f;

				PN_CORE_INFO("[UIInput]   text bounds (NDC): min=({:.3f},{:.3f}) max=({:.3f},{:.3f})",
					rect_min_ndc.x, rect_min_ndc.y, rect_max_ndc.x, rect_max_ndc.y);

				if (isPointInRect(normalized_mouse, rect_min_ndc, rect_max_ndc)) {
					PN_CORE_INFO("[UIInput]   -> HIT!");
					// Get canvas sort order
					int canvas_sort = 0;
					entt::entity parent = entity;
					while (parent != entt::null && registry.all_of<Entity::Hierarchy>(parent)) {
						const auto& parentHierarchy = registry.get<Entity::Hierarchy>(parent);
						if (registry.all_of<UICanvas>(parent)) {
							canvas_sort = registry.get<UICanvas>(parent).sort_order;
							break;
						}
						if (!parentHierarchy.parentGUID.IsValid()) break;
						parent = ecs->resolveGUID(parentHierarchy.parentGUID);
					}

					// Get sibling index
					int sibling_index = 0;
					if (registry.all_of<Entity::Hierarchy>(entity)) {
						const auto& hierarchy = registry.get<Entity::Hierarchy>(entity);
						const auto& entity_guid = registry.get<Entity::GUID>(entity);

						if (hierarchy.parentGUID.IsValid()) {
							entt::entity parent_entity = ecs->resolveGUID(hierarchy.parentGUID);
							if (parent_entity != entt::null && registry.all_of<Entity::Hierarchy>(parent_entity)) {
								const auto& parentHierarchy = registry.get<Entity::Hierarchy>(parent_entity);
								auto it = std::find(parentHierarchy.childrenGUIDs.begin(),
									parentHierarchy.childrenGUIDs.end(), entity_guid.guid);
								if (it != parentHierarchy.childrenGUIDs.end()) {
									sibling_index = static_cast<int>(std::distance(parentHierarchy.childrenGUIDs.begin(), it));
								}
							}
						}
					}

					candidates.push_back({ entity, canvas_sort, sibling_index });
				}
				else {
					PN_CORE_INFO("[UIInput]   -> MISS");
				}
			}
			PN_CORE_INFO("[UIInput] UIText raycast: checked {} entities, {} candidates", text_entity_count, candidates.size());

			// Sort by canvas_sort DESC, then sibling_index DESC (front-to-back)
			if (!candidates.empty()) {
				std::sort(candidates.begin(), candidates.end(),
					[](const auto& a, const auto& b) {
						if (std::get<1>(a) != std::get<1>(b))
							return std::get<1>(a) > std::get<1>(b);
						return std::get<2>(a) > std::get<2>(b);
					});
				return std::get<0>(candidates.front());
			}

			return std::nullopt;
		}

		bool InputSystem::isPointInRect(const glm::vec2& point, const glm::vec2& rect_min, const glm::vec2& rect_max) {
			return point.x >= rect_min.x && point.x <= rect_max.x &&
				point.y >= rect_min.y && point.y <= rect_max.y;
		}

		void InputSystem::updateButtonState(entt::entity entity, entt::registry& registry, UIButtonState new_state) {
			if (registry.all_of<UIButton>(entity)) {
				auto& button = registry.get<UIButton>(entity);
				if (button.state != new_state) {
					button.state = new_state;

					// Handle Hover Interaction (Frame Swap)
					if (registry.all_of<UIAnimation>(entity)) {
						auto& anim = registry.get<UIAnimation>(entity);

						// We assume Frame 0 is Normal, Frame 1 is Hover
						int target_frame = 0;
						if (new_state == UIButtonState::Highlighted || new_state == UIButtonState::Pressed) {
							target_frame = 1;
						}

						// Use 0 if we don't have enough frames
						if (target_frame >= anim.total_frames) {
							target_frame = 0;
						}

						// Set state (ensure not playing)
						anim.b_playing = false;
						anim.current_frame = target_frame;

						// Force UV update
						if (registry.all_of<UVCoordinates>(entity)) {
							auto& uv_comp = registry.get<UVCoordinates>(entity);

							if (anim.spritesheet_columns > 0 && anim.spritesheet_rows > 0) {
								// Retrieve texture dimensions for pixel padding (fixes bleeding)
								float padding_u = 0.0f;
								float padding_v = 0.0f;
								float tex_w = 0.0f;
								float tex_h = 0.0f;

								if (registry.all_of<Texture2D>(entity)) {
									auto& tex = registry.get<Texture2D>(entity);
									auto svc_lock = services.lock();
									if (svc_lock) {
										auto texture_opt = svc_lock->get<Assets::Manager>()->getAsset<Assets::Texture>(tex.texture_guid);
										if (texture_opt.has_value()) {
											tex_w = static_cast<float>(texture_opt.value().get()->width);
											tex_h = static_cast<float>(texture_opt.value().get()->height);

											// 1.0 pixel padding (aggressively prevent bleeding)
											if (tex_w > 0) padding_u = 1.0f / tex_w;
											if (tex_h > 0) padding_v = 1.0f / tex_h;
										}
									}
								}

								int col = target_frame % anim.spritesheet_columns;
								int row = target_frame / anim.spritesheet_columns;
								row = row % anim.spritesheet_rows; // Safety

								float frame_width = 1.0f / anim.spritesheet_columns;
								float frame_height = 1.0f / anim.spritesheet_rows;

								glm::vec4 new_uvs;
								// Standard Top-Left with Inset Padding
								new_uvs.x = (col * frame_width) + padding_u;
								new_uvs.y = (row * frame_height) + padding_v;
								new_uvs.z = ((col + 1) * frame_width) - padding_u;
								new_uvs.w = ((row + 1) * frame_height) - padding_v;

								uv_comp.uv = new_uvs;
							}
						}
						else {
							// If UV component missing, create it
							if (anim.spritesheet_columns > 0 && anim.spritesheet_rows > 0) {
								registry.emplace<UVCoordinates>(entity);
								auto& uv_comp = registry.get<UVCoordinates>(entity);

								int col = target_frame % anim.spritesheet_columns;
								int row = target_frame / anim.spritesheet_columns;

								float frame_width = 1.0f / anim.spritesheet_columns;
								float frame_height = 1.0f / anim.spritesheet_rows;

								uv_comp.uv = glm::vec4(col * frame_width, row * frame_height,
									(col + 1) * frame_width, (row + 1) * frame_height);
							}
						}
					}
				}
			}
		}

		void InputSystem::activateButton(entt::entity entity, entt::registry& registry)
		{
			if (!registry.all_of<UIButton>(entity))
				return;

			auto& button = registry.get<UIButton>(entity);
			PN_CORE_INFO("[UI] On click, UIButton.action = {}", ToActionName(button.action));
			if (button.action != UIAction::None) {
				dispatchUIAction(entity, registry, button.action);
				return;
			}
		}

		void InputSystem::dispatchUIAction(entt::entity entity, entt::registry& registry, UIAction action)
		{
			auto& ctx = registry.ctx();
			if (!ctx.contains<LuaManager*>()) {
				PN_CORE_WARN("[UIInput] dispatchUIAction: LuaManager* not found in registry context");
				return;
			}

			auto& luaMgr = ctx.get<LuaManager*>();
			if (!luaMgr) {
				PN_CORE_WARN("[UIInput] dispatchUIAction: LuaManager is null");
				return;
			}

			auto& button = registry.get<UIButton>(entity);

			const char* actionName = ToActionName(action);

			// Call a single Lua router: UI_OnAction(actionName, entityId, payload)
			luaMgr->callGlobal("UI_OnAction",
				actionName,
				entity,
				button.payload);
		}

		glm::vec2 InputSystem::convertToCenterOrigin(const glm::vec2& screen_pos)
		{
			auto svc = services.lock();
			auto window = svc->get<Window::Window>();
			if (window) {
				glm::vec2 fb = window->getFrameBuffer();
				glm::vec2 center_pos;
				center_pos.x = screen_pos.x - (fb.x * 0.5f);
				center_pos.y = (fb.y * 0.5f) - screen_pos.y;
				return center_pos;
			}
			// Fallback: return as-is
			return screen_pos;
		}

		glm::vec2 InputSystem::normalizeScreenPosition(const glm::vec2& center_origin_pos)
		{
			auto svc = services.lock();
			auto window = svc->get<Window::Window>();
			if (!window) return center_origin_pos;

			glm::vec2 fb = window->getFrameBuffer();

			// Convert from center-origin pixels to normalized -1 to 1 range
			glm::vec2 normalized{};
			normalized.x = (center_origin_pos.x / (fb.x * 0.5f)); // Map to -1 to 1
			normalized.y = (center_origin_pos.y / (fb.y * 0.5f)); // Map to -1 to 1

			return normalized;
		}

		glm::vec2 normalizeSize(const glm::vec2& pixel_size, std::weak_ptr<Services> services)
		{
			auto svc = services.lock();
			auto window = svc->get<Window::Window>();
			if (!window) return glm::vec2(0.0f);

			glm::vec2 fb = window->getFrameBuffer();

			// Convert pixel size to normalized size in -1 to 1 space
			glm::vec2 normalized;
			normalized.x = (pixel_size.x / fb.x) * 2.0f; // Total span is 2 (from -1 to 1)
			normalized.y = (pixel_size.y / fb.y) * 2.0f;

			return normalized;
		}

	} // namespace UI
} // namespace PAIN
