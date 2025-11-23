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
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"
#include "CoreSystems/Events/Android/TouchEvents.h"
#include "CoreSystems/Events/Android/OtherEvents.h"
#include "CoreSystems/Events/Android/SurfaceEvents.h"

namespace PAIN {
	namespace UI {

		InputSystem::InputSystem(std::shared_ptr<Services> svc) : ISystem(svc)
		{
		}

		InputSystem::~InputSystem()
		{
		}

		void InputSystem::onUpdate(AppTiming timing, entt::registry& registry) {
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
					m_mouse_position = glm::vec2(e.getWindowPos());
					// Return false to continue dispatching to other systems
					return false;
					});
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

							// Execute Lua callback if exists
							if (!button.on_click_callback_lua.empty()) {
								// TODO: Call Lua function via your scripting service
								PN_CORE_INFO("Button clicked: {}", button.on_click_callback_lua);

								// Example: If you have a Lua service
								// auto lua_service = services->get<LuaService>();
								// lua_service->executeCallback(button.on_click_callback_lua);
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
								PN_CORE_INFO("Button clicked: {}", button.on_click_callback_lua);
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
			auto metadata_service = services.lock()->get<MetaData::Service>();
			auto view = registry.view<UIRectTransform, UIElement>();

			// Collect candidates with canvas sort order and hierarchy depth
			// entity, canvas_sort, sibling_index
			std::vector <std::tuple<entt::entity, int, int>> candidates;

			for (auto&& [entity, rect, element] : view.each()) {
				if (!element.b_is_enabled || !element.b_is_interactable) continue;

				// Use calculated world position and size from layout system
				glm::vec2 pos = rect.calculated_world_position;
				glm::vec2 size = rect.calculated_world_size;
				glm::vec2 rect_min = pos;
				glm::vec2 rect_max = pos + size;

				if (isPointInRect(mouse_pos, rect_min, rect_max)) {
					// Get canvas sort order by traversing up hierarchy
					int canvas_sort = 0;
					auto parent = entity;
					while (parent != entt::null) {
						if (registry.all_of<UICanvas>(parent)) {
							canvas_sort = registry.get<UICanvas>(parent).sort_order;
							break;
						}
						auto parent_opt = metadata_service->getParent(parent);
						parent = parent_opt.has_value() ? parent_opt.value() : entt::null;
					}

					// Get sibling index (hierarchy order)
					int sibling_index = 0;
					auto parent_opt = metadata_service->getParent(entity);
					if (parent_opt.has_value()) {
						auto siblings = metadata_service->getChildren(parent_opt.value());
						auto it = std::find(siblings.begin(), siblings.end(), entity);
						if (it != siblings.end()) {
							sibling_index = std::distance(siblings.begin(), it);
						}
					}

					candidates.push_back({ entity, canvas_sort, sibling_index });
				}
			}

			// Sort by canvas first (higher = front), then sibling index (higher = front)
			if (!candidates.empty()) {
				std::sort(candidates.begin(), candidates.end(),
					[](const auto& a, const auto& b) {
						if (std::get<1>(a) != std::get<1>(b))
							// Canvas sort order
							return std::get<1>(a) > std::get<1>(b);
						// Last child rendered on top
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