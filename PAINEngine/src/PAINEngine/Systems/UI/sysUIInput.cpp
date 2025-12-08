/*****************************************************************//**
 * \file   sysUILayout.h
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

#ifdef PN_PLATFORM_WINDOWS
#include "imgui.h"
#include "LayeredSystems/LevelEditor/Editor.h"
#endif

namespace PAIN {
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
                        updateButtonState(m_pressed_entity, registry, UIButtonState::Pressed);
                        // DEBUG: log which entity we pressed on
                        PN_CORE_INFO("[UIInput] MouseDown on UI entity id = {}",
                            static_cast<uint32_t>(m_pressed_entity));

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

                        // Trigger callback if released on same button
                        if (m_pressed_entity == m_hovered_entity &&
                            registry.all_of<UIButton>(m_pressed_entity)) {
                            auto& button = registry.get<UIButton>(m_pressed_entity);

                            // DEBUG:
                            PN_CORE_INFO("[UIInput] MouseUp: pressed={:08X} hovered={:08X} callback='{}'",
                                static_cast<uint32_t>(m_pressed_entity),
                                static_cast<uint32_t>(m_hovered_entity),
                                button.on_click_callback_lua);

                            if (!button.on_click_callback_lua.empty()) {
                                auto& ctx = registry.ctx();
                                if (ctx.contains<LuaManager*>()) {
                                    auto& luaMgr = ctx.get<LuaManager*>();
                                    if (luaMgr) {
                                        PN_CORE_INFO("[UIInput] Button clicked: {}, calling Lua",
                                            button.on_click_callback_lua);
                                        luaMgr->callGlobal(button.on_click_callback_lua);
                                    }
                                }
                            }
                        }

                        updateButtonState(m_pressed_entity, registry,
                            m_pressed_entity == m_hovered_entity ?
                            UIButtonState::Highlighted : UIButtonState::Normal);
                        m_pressed_entity = entt::null;
                        return true;
                    }
                    return false;
                    });
            }

#else
            // ── Android touch events ──
            if (event.getType() == Event::Type::TouchMove) {
                Event::Dispatcher dispatcher(event);
                dispatcher.Dispatch<Event::TouchMove>([&](Event::TouchMove& e) -> bool {
                    glm::vec2 raw_touch(e.getX(), e.getY());
                    m_mouse_position = convertToCenterOrigin(raw_touch);
                    glm::vec2 norm_mouse_position = normalizeScreenPosition(m_mouse_position);
                    PN_CORE_INFO("[UIInput] Touch: raw=({:.0f}, {:.0f}) center=({:.0f}, {:.0f}) normalized=({:.3f}, {:.3f})",
                        raw_touch.x, raw_touch.y,
                        m_mouse_position.x, m_mouse_position.y,
                        norm_mouse_position.x, norm_mouse_position.y);
                    return false;
                    });
            }

            if (event.getType() == Event::Type::TouchDown) {
                Event::Dispatcher dispatcher(event);
                dispatcher.Dispatch<Event::TouchDown>([&](Event::TouchDown& e) -> bool {
                    m_mouse_position = convertToCenterOrigin(glm::vec2(e.getX(), e.getY()));
                    auto ecs = services.lock()->get<ECS::Controller>();
                    auto& registry = ecs->getRegistry();
                    auto hit_entity = raycastUI(m_mouse_position, registry);

                    if (hit_entity.has_value()) {
                        PN_CORE_INFO("hit detected");
                        m_hovered_entity = hit_entity.value();
                        m_pressed_entity = m_hovered_entity;
                        updateButtonState(m_pressed_entity, registry, UIButtonState::Pressed);
                        return true;
                    }
                    return false;
                    });
            }

            if (event.getType() == Event::Type::TouchUp) {
                Event::Dispatcher dispatcher(event);
                dispatcher.Dispatch<Event::TouchUp>([&](Event::TouchUp& e) -> bool {
                    if (m_pressed_entity != entt::null) {
                        auto ecs = services.lock()->get<ECS::Controller>();
                        auto& registry = ecs->getRegistry();
                        m_mouse_position = convertToCenterOrigin(glm::vec2(e.getX(), e.getY()));
                        auto hit_entity = raycastUI(m_mouse_position, registry);

                        if (hit_entity.has_value() && hit_entity.value() == m_pressed_entity &&
                            registry.all_of<UIButton>(m_pressed_entity)) {
                            PN_CORE_INFO("hit detected");
                            auto& button = registry.get<UIButton>(m_pressed_entity);
                            if (!button.on_click_callback_lua.empty()) {
                                auto& ctx = registry.ctx();
                                if (ctx.contains<LuaManager*>()) {
                                    auto& luaMgr = ctx.get<LuaManager*>();
                                    if (luaMgr) luaMgr->callGlobal(button.on_click_callback_lua);
                                }
                            }
                        }

                        updateButtonState(m_pressed_entity, registry,
                            hit_entity.has_value() && hit_entity.value() == m_pressed_entity ?
                            UIButtonState::Highlighted : UIButtonState::Normal);
                        m_pressed_entity = entt::null;
                        m_hovered_entity = entt::null;
                        return true;
                    }
                    return false;
                    });
            }

            if (event.getType() == Event::Type::TouchCancel) {
                Event::Dispatcher dispatcher(event);
                dispatcher.Dispatch<Event::TouchCancel>([&](Event::TouchCancel& e) -> bool {
                    if (m_pressed_entity != entt::null) {
                        auto ecs = services.lock()->get<ECS::Controller>();
                        auto& registry = ecs->getRegistry();
                        updateButtonState(m_pressed_entity, registry, UIButtonState::Normal);
                        m_pressed_entity = entt::null;
                        m_hovered_entity = entt::null;
                        return true;
                    }
                    return false;
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

            // Query all UI elements with Texture2D (the actual rendered texture holds position)
            auto view = registry.view<Texture2D, UIElement, UIRectTransform>();

            //PN_CORE_INFO("[UIInput] Raycast mouse_pos = ({:.3f}, {:.3f}) (normalized)",
            //    normalized_mouse.x, normalized_mouse.y);

            std::vector<std::tuple<entt::entity, int, int>> candidates;

            for (auto [entity, tex, element, rect] : view.each()) {
                if (!element.b_is_enabled || !element.b_is_interactable) continue;

                // Actual rendered size = base size * scale multiplier
                glm::vec2 actual_pixel_size = rect.size_delta * tex.texture_scale;

                //PN_CORE_INFO("[UIInput]   size_delta=({:.1f}, {:.1f}) texture_scale=({:.3f}, {:.3f}) actual_pixel_size=({:.1f}, {:.1f})",
                //    rect.size_delta.x, rect.size_delta.y,
                //    tex.texture_scale.x, tex.texture_scale.y,
                //    actual_pixel_size.x, actual_pixel_size.y);

                // Convert size to normalized space
                glm::vec2 normalized_size = normalizeSize(actual_pixel_size);

                //PN_CORE_INFO("[UIInput] AFTER normalizeSize: normalized_size=({:.3f}, {:.3f})",
                //    normalized_size.x, normalized_size.y);

                // tex.pos is the CENTER of the texture, so calculate min/max from center
                glm::vec2 half_size = normalized_size;
                //PN_CORE_INFO("[UIInput] half_size=({:.3f}, {:.3f})", half_size.x, half_size.y);
                //PN_CORE_INFO("[UIInput] tex.pos=({:.3f}, {:.3f})", tex.pos.x, tex.pos.y);
                glm::vec2 rect_min = tex.pos - half_size;  // Center - half size
                glm::vec2 rect_max = tex.pos + half_size;  // Center + half size
                //PN_CORE_INFO("[UIInput] FINAL rect_min=({:.3f}, {:.3f}) rect_max=({:.3f}, {:.3f})",
                //    rect_min.x, rect_min.y, rect_max.x, rect_max.y);

                // DEBUG: Log every UI element being tested
                //PN_CORE_INFO("[UIInput]   Entity {:08X}: enabled={} interactable={} rect_min=({:.1f}, {:.1f}) rect_max=({:.1f}, {:.1f})",
                //    static_cast<uint32_t>(entity),
                //    element.b_is_enabled,
                //    element.b_is_interactable,
                //    rect_min.x, rect_min.y,
                //    rect_max.x, rect_max.y);

                //PN_CORE_INFO("[UIInput]   mouse=({:.3f}, {:.3f}) in_x={} in_y={}",
                //    normalized_mouse.x, normalized_mouse.y,
                //    (normalized_mouse.x >= rect_min.x && normalized_mouse.x <= rect_max.x),
                //    (normalized_mouse.y >= rect_min.y && normalized_mouse.y <= rect_max.y))

                if (isPointInRect(normalized_mouse, rect_min, rect_max)) {

                    PN_CORE_INFO("point in button");

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
                    if (registry.all_of<Entity::Hierarchy, Entity::GUID>(entity)) {
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
                button.state = new_state;
                // TODO: Apply color tinting if needed
            }
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
            normalized.x = (center_origin_pos.x / (fb.x * 0.5f));  // Map to -1 to 1
            normalized.y = (center_origin_pos.y / (fb.y * 0.5f)); // Map to -1 to 1

            return normalized;
        }

        glm::vec2 InputSystem::normalizeSize(const glm::vec2& pixel_size)
        {
            auto svc = services.lock();
            auto window = svc->get<Window::Window>();

            if (!window) return glm::vec2(0.0f);

            glm::vec2 fb = window->getFrameBuffer();

            // Convert pixel size to normalized size in -1 to 1 space
            glm::vec2 normalized;
            normalized.x = (pixel_size.x / fb.x) * 2.0f;  // Total span is 2 (from -1 to 1)
            normalized.y = (pixel_size.y / fb.y) * 2.0f;

            return normalized;
        }

    } // namespace UI
} // namespace PAIN
