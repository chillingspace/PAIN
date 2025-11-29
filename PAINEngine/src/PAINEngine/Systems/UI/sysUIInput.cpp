/*****************************************************************//**
 * \file   sysUIInput.cpp
 * \brief  Definition of UI Input system
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysUIInput.h"
#include "ECS/Controller.h"
#include "ECS/sMetaData.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Scripting/luaManager.h" 
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"
#include "CoreSystems/Events/Android/TouchEvents.h"
#include "CoreSystems/Events/Android/OtherEvents.h"
#include "CoreSystems/Events/Android/SurfaceEvents.h"

#ifdef PN_PLATFORM_WINDOWS
#include "imgui.h"
#include "LayeredSystems/LevelEditor/Editor.h" 
#endif

namespace PAIN {
	namespace UI {

		InputSystem::InputSystem(std::shared_ptr<Services> svc) : ISystem(svc)
		{
		}

		InputSystem::~InputSystem()
		{
		}

		void InputSystem::onUpdate(AppTiming timing, entt::registry& registry) {

#ifdef PN_PLATFORM_WINDOWS
			if (m_mouse_position.x < 0.0f || m_mouse_position.y < 0.0f)
				return;
#endif

			// Raycast to find hovered UI element
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

			// Handle mouse movement for windows
			if (event.getType() == Event::Type::MouseMove) {
				Event::Dispatcher dispatcher(event);

				dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {
					//m_mouse_position = glm::vec2(e.getWindowPos());
					//// Return false to continue dispatching to other systems
					//return false;

					auto svc = services.lock();
					auto window = svc->get<Window::Window>();
					if (window) {
						glm::vec2 fb = window->getFrameBuffer(); // (width, height)
						glm::vec2 winPos = e.getWindowPos();     // origin: top-left

						// Convert to UI space: origin bottom-left
						m_mouse_position = { winPos.x, fb.y - winPos.y };
					}
					else {
						// Fallback: old behaviour
						m_mouse_position = glm::vec2(e.getWindowPos());
					}

					return false;
					}
				);
			}


			// If editor wants the mouse, don't let game UI handle it
			if (ImGui::GetCurrentContext()) {
				ImGuiIO& io = ImGui::GetIO();
				if (io.WantCaptureMouse) {
					// Optional: debug
					// PN_CORE_INFO("[UIInput] Skipping UI input because ImGui wants the mouse");
					return;
				}
			}

			// Ignore game UI clicks when the editor overlay is visible
			if (auto editor = services.lock()->get<PAIN::Editor::Editor>()) {
				if (editor->isVisible()) {
					return;
				}
			}


			// Handle mouse button press
			if (event.getType() == Event::Type::MouseButtonPress) {
				Event::Dispatcher dispatcher(event);

				dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
					// Only handle left click
					if (e.getBtnCode() != 0) return false;

					// Check if we're hovering over a UI element
					if (m_hovered_entity != entt::null) {
						m_pressed_entity = m_hovered_entity;

						auto ecs = services.lock()->get<ECS::Controller>();
						auto& registry = ecs->getRegistry();
						updateButtonState(m_pressed_entity, registry, UIButtonState::Pressed);

						// DEBUG: log which entity we pressed on
						PN_CORE_INFO("[UIInput] MouseDown on UI entity id = {}",
							static_cast<uint32_t>(m_pressed_entity));

						// Return true to stop dispatching - UI consumed this event
						return true;
					}

					// Return false - no UI hit, let other systems handle it
					return false;
					});
			}

			// Handle mouse button release
			if (event.getType() == Event::Type::MouseButtonRelease) {
				Event::Dispatcher dispatcher(event);

				dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {
					// Only handle left click
					if (e.getBtnCode() != 0) return false;

					if (m_pressed_entity != entt::null) {
						auto ecs = services.lock()->get<ECS::Controller>();
						auto& registry = ecs->getRegistry();

						// Trigger callback if released on same button
						if (m_pressed_entity == m_hovered_entity &&
							registry.all_of<UIButton>(m_pressed_entity)) {
							auto& button = registry.get<UIButton>(m_pressed_entity);

							// DEBUG:
							PN_CORE_INFO("[UIInput] MouseUp: pressed={:08X} hovered={:08X} callback='{}'",
								static_cast<uint32_t>(m_pressed_entity),
								static_cast<uint32_t>(m_hovered_entity),
								button.on_click_callback_lua);

							// Execute Lua callback if exists
							if (!button.on_click_callback_lua.empty()) {
								// TODO: Call Lua function via your scripting service
								//PN_CORE_INFO("Button clicked: {}", button.on_click_callback_lua);

								// Example: If you have a Lua service
								// auto lua_service = services->get<LuaService>();
								// lua_service->executeCallback(button.on_click_callback_lua);

								auto& ctx = registry.ctx();

								if (ctx.contains<PAIN::LuaManager*>()) {
									auto& luaMgrPtr = ctx.get<PAIN::LuaManager*>();
									if (luaMgrPtr) {
										PN_CORE_INFO("[UIInput] Button clicked: {}, calling Lua",
											button.on_click_callback_lua);
										luaMgrPtr->callGlobal(button.on_click_callback_lua);
									}
									else {
										PN_CORE_WARN("[UIInput] LuaManager* in registry ctx is null; cannot run '{}'",
											button.on_click_callback_lua);
									}
								}
								else {
									PN_CORE_WARN("[UIInput] No LuaManager* in registry ctx; cannot run '{}'",
										button.on_click_callback_lua);
								}
							}
						}

						// Update state
						updateButtonState(m_pressed_entity, registry,
							m_pressed_entity == m_hovered_entity ?
							UIButtonState::Highlighted : UIButtonState::Normal);
						m_pressed_entity = entt::null;

						// Return true - UI consumed this event
						return true;
					}

					// Return false - no pressed UI element
					return false;
					});
			}
#else
			// To handle android events here
			if (event.getType() == Event::Type::TouchMove) {
				Event::Dispatcher dispatcher(event);

				dispatcher.Dispatch<Event::TouchMove>([&](Event::TouchMove& e) -> bool {
					// Use first touch point as "mouse" position
					m_mouse_position = glm::vec2(e.getX(), e.getY());
					// Return false to continue dispatching to other systems
					return false;
					});
			}

			// Handle touch down (equivalent to mouse press)
			if (event.getType() == Event::Type::TouchDown) {
				Event::Dispatcher dispatcher(event);

				dispatcher.Dispatch<Event::TouchDown>([&](Event::TouchDown& e) -> bool {
					// Update touch position
					m_mouse_position = glm::vec2(e.getX(), e.getY());

					// Perform immediate raycast at touch position
					auto ecs = services.lock()->get<ECS::Controller>();
					auto& registry = ecs->getRegistry();
					auto hit_entity = raycastUI(m_mouse_position, registry);

					// Check if we touched a UI element
					if (hit_entity.has_value()) {
						m_hovered_entity = hit_entity.value();
						m_pressed_entity = m_hovered_entity;

						updateButtonState(m_pressed_entity, registry, UIButtonState::Pressed);

						// Return true to stop dispatching - UI consumed this event
						return true;
					}

					// Return false - no UI hit, let other systems handle it
					return false;
					});
			}

			// Handle touch up (equivalent to mouse release)
			if (event.getType() == Event::Type::TouchUp) {
				Event::Dispatcher dispatcher(event);

				dispatcher.Dispatch<Event::TouchUp>([&](Event::TouchUp& e) -> bool {
					if (m_pressed_entity != entt::null) {
						auto ecs = services.lock()->get<ECS::Controller>();
						auto& registry = ecs->getRegistry();

						// Update position for final raycast
						m_mouse_position = glm::vec2(e.getX(), e.getY());
						auto hit_entity = raycastUI(m_mouse_position, registry);

						// Trigger callback if released on same button
						if (hit_entity.has_value() &&
							hit_entity.value() == m_pressed_entity &&
							registry.all_of<UIButton>(m_pressed_entity)) {
							auto& button = registry.get<UIButton>(m_pressed_entity);

							// Execute Lua callback if exists
							if (!button.on_click_callback_lua.empty()) {
								// TODO: Call Lua function via your scripting service
								//PN_CORE_INFO("Button clicked: {}", button.on_click_callback_lua);

								auto& ctx = registry.ctx();

								if (ctx.contains<PAIN::LuaManager*>()) {
									auto& luaMgrPtr = ctx.get<PAIN::LuaManager*>();
									if (luaMgrPtr) {
										PN_CORE_INFO("[UIInput] Button clicked: {}, calling Lua",
											button.on_click_callback_lua);
										luaMgrPtr->callGlobal(button.on_click_callback_lua);
									}
									else {
										PN_CORE_WARN("[UIInput] LuaManager* in registry ctx is null; cannot run '{}'",
											button.on_click_callback_lua);
									}
								}
								else {
									PN_CORE_WARN("[UIInput] No LuaManager* in registry ctx; cannot run '{}'",
										button.on_click_callback_lua);
								}
							}
						}

						// Update state
						updateButtonState(m_pressed_entity, registry,
							hit_entity.has_value() && hit_entity.value() == m_pressed_entity ?
							UIButtonState::Highlighted : UIButtonState::Normal);

						// Clear pressed and hover states
						m_pressed_entity = entt::null;
						m_hovered_entity = entt::null;

						// Return true - UI consumed this event
						return true;
					}

					// Return false - no pressed UI element
					return false;
					});
			}

			// Handle touch cancel (finger lifted unexpectedly or interrupted)
			if (event.getType() == Event::Type::TouchCancel) {
				Event::Dispatcher dispatcher(event);

				dispatcher.Dispatch<Event::TouchCancel>([&](Event::TouchCancel& e) -> bool {
					// Cancel any ongoing UI interaction
					if (m_pressed_entity != entt::null) {
						auto ecs = services.lock()->get<ECS::Controller>();
						auto& registry = ecs->getRegistry();

						// Reset to normal state (no callback fired)
						updateButtonState(m_pressed_entity, registry, UIButtonState::Normal);

						m_pressed_entity = entt::null;
						m_hovered_entity = entt::null;

						// Return true - UI was handling this
						return true;
					}

					return false;
					});
			}
#endif
		}

		std::optional<entt::entity> InputSystem::raycastUI(const glm::vec2& mouse_pos, entt::registry& registry) {
			auto ecs = services.lock()->get<ECS::Controller>();
			auto view = registry.view<UIRectTransform, UIElement>();

			//PN_CORE_INFO("[UIInput] Raycast mouse_pos = ({:.1f}, {:.1f})", mouse_pos.x, mouse_pos.y);


			// Each candidate: entity, canvas_sort, sibling_index
			std::vector<std::tuple<entt::entity, int, int>> candidates;

			for (auto&& [entity, rect, element] : view.each()) {
				if (!element.b_is_enabled || !element.b_is_interactable) continue;

				glm::vec2 pos = rect.calculated_world_position;
				glm::vec2 size = rect.calculated_world_size;
				glm::vec2 rect_min = pos;
				glm::vec2 rect_max = pos + size;

				// DEBUG: log every UI element tested
				/*PN_CORE_INFO("[UIInput] UI Entity {:08X}: rect min=({:.1f}, {:.1f}) max=({:.1f}, {:.1f}) enabled={} interactable={}",
					static_cast<uint32_t>(entity),
					rect_min.x, rect_min.y,
					rect_max.x, rect_max.y,
					element.b_is_enabled,
					element.b_is_interactable);*/


				if (isPointInRect(mouse_pos, rect_min, rect_max)) {
					PN_CORE_INFO("[UIInput] ---> Hit entity {:08X}", static_cast<uint32_t>(entity));

					if (registry.all_of<UIButton>(entity)) {
						auto& btn = registry.get<UIButton>(entity);
						PN_CORE_INFO("[UIInput]      UIButton on_click_callback_lua = '{}'",
							btn.on_click_callback_lua);
					}


					// Canvas sort order by traversing up the hierarchy
					int canvas_sort = 0;
					entt::entity parent = entity;
					while (parent != entt::null && registry.all_of<Entity::Hierarchy>(parent)) {
						const auto& parentHierarchy = registry.get<Entity::Hierarchy>(parent);
						if (registry.all_of<UICanvas>(parent)) {
							canvas_sort = registry.get<UICanvas>(parent).sort_order;
							break;
						}
						if (!parentHierarchy.parentGUID.IsValid())
							break;
						parent = ecs->resolveGUID(parentHierarchy.parentGUID);
					}

					// Sibling index (in parent's children list)
					int sibling_index = 0;
					if (registry.all_of<Entity::Hierarchy>(entity) && registry.all_of<Entity::GUID>(entity)) {
						const auto& hierarchy = registry.get<Entity::Hierarchy>(entity);
						const auto& entity_guid = registry.get<Entity::GUID>(entity); 
						if (hierarchy.parentGUID.IsValid()) {
							entt::entity parent_entity = ecs->resolveGUID(hierarchy.parentGUID);
							if (parent_entity != entt::null && registry.all_of<Entity::Hierarchy>(parent_entity)) {
								const auto& parentHierarchy = registry.get<Entity::Hierarchy>(parent_entity);
								auto it = std::find(parentHierarchy.childrenGUIDs.begin(), parentHierarchy.childrenGUIDs.end(), entity_guid.guid);
								// Find current entity guid in parent guid vector
								if (it != parentHierarchy.childrenGUIDs.end()) {
									sibling_index = static_cast<int>(std::distance(parentHierarchy.childrenGUIDs.begin(), it));
								}
							}
						}
					}


					candidates.push_back({ entity, canvas_sort, sibling_index });
				}
			}

			// Sort top-most first: canvas_sort DESC, sibling_index DESC (larger sibling = "front")
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



		bool InputSystem::isPointInRect(const glm::vec2& point, const glm::vec2& rect_min,
			const glm::vec2& rect_max) {
			return point.x >= rect_min.x && point.x <= rect_max.x &&
				point.y >= rect_min.y && point.y <= rect_max.y;
		}

		void InputSystem::updateButtonState(entt::entity entity, entt::registry& registry,
			UIButtonState new_state) {
			if (registry.all_of<UIButton>(entity)) {
				auto& button = registry.get<UIButton>(entity);
				button.state = new_state;

				// Optional: Apply color tinting based on state
				//if (registry.all_of<UIBU>(entity)) {
				//    auto& image = registry.get<UIImage>(entity);
				//    switch (new_state) {
				//    case UIButtonState::Normal:
				//        image.color = button.normal_color;
				//        break;
				//    case UIButtonState::Highlighted:
				//        image.color = button.highlighted_color;
				//        break;
				//    case UIButtonState::Pressed:
				//        image.color = button.pressed_color;
				//        break;
				//    case UIButtonState::Disabled:
				//        image.color = button.disabled_color;
				//        break;
				//    }
				//}
			}
		}
	}
}