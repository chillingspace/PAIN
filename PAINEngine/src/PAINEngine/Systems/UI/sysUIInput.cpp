/*****************************************************************//**
 * \file   sysUILayout.h
 * \brief  Declaration of UI layout system
 *
 * \author Bryan Lim, 2301214, [bryanlicheng.l@digipen.edu](mailto:bryanlicheng.l@digipen.edu) (100%)
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

                    // Handle joystick drag - ONLY if entity has UIJoystick component
                    if (m_pressed_entity != entt::null) {
                        auto ecs = services.lock()->get<ECS::Controller>();
                        auto& registry = ecs->getRegistry();

                        if (registry.valid(m_pressed_entity) &&
                            registry.all_of<Texture2D, UIButton, UIJoystick>(m_pressed_entity)) {

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
                                if (!button.on_click_callback_lua.empty()) {
                                    auto& ctx = registry.ctx();
                                    if (ctx.contains<LuaManager*>()) {
                                        auto& luaMgr = ctx.get<LuaManager*>();
                                        if (luaMgr) {
                                            luaMgr->callGlobalWithVec2(button.on_click_callback_lua,
                                                direction.x, direction.y);
                                        }
                                    }
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

                        // ✅ Store center position ONLY if this is a joystick
                        if (registry.all_of<UIButton, Texture2D, UIJoystick>(m_pressed_entity)) {
                            auto& joystick = registry.get<UIJoystick>(m_pressed_entity);
                            auto& tex = registry.get<Texture2D>(m_pressed_entity);
                            joystick.center_position = tex.pos;
                            joystick.is_dragging = false;
                        }

                        updateButtonState(m_pressed_entity, registry, UIButtonState::Pressed);
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

                        bool was_joystick_drag = false;

                        // If this was a joystick, reset it
                        if (registry.valid(m_pressed_entity) &&
                            registry.all_of<UIJoystick, Texture2D, UIButton>(m_pressed_entity)) {

                            auto& joystick = registry.get<UIJoystick>(m_pressed_entity);
                            was_joystick_drag = joystick.is_dragging;

                            if (was_joystick_drag) {
                                // Reset joystick to center
                                auto& tex = registry.get<Texture2D>(m_pressed_entity);
                                tex.pos = joystick.center_position;

                                // Notify Lua that movement stopped
                                auto& button = registry.get<UIButton>(m_pressed_entity);
                                if (!button.on_click_callback_lua.empty()) {
                                    auto& ctx = registry.ctx();
                                    if (ctx.contains<LuaManager*>()) {
                                        auto& luaMgr = ctx.get<LuaManager*>();
                                        if (luaMgr) {
                                            luaMgr->callGlobalWithVec2(button.on_click_callback_lua, 0.f, 0.f);
                                        }
                                    }
                                }

                                joystick.is_dragging = false;
                            }
                        }

                        // If NOT a joystick drag, treat as normal button click
                        if (!was_joystick_drag && m_pressed_entity == m_hovered_entity &&
                            registry.all_of<UIButton>(m_pressed_entity)) {
                            auto& button = registry.get<UIButton>(m_pressed_entity);

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

                    // Handle joystick drag ONLY for correct pointer ID and UIJoystick component
                    if (m_pressed_entity != entt::null && e.getPointerId() == m_joystick_pointer_id) {
                        auto ecs = services.lock()->get<ECS::Controller>();
                        auto& registry = ecs->getRegistry();

                        if (registry.valid(m_pressed_entity) &&
                            registry.all_of<Texture2D, UIButton, UIJoystick>(m_pressed_entity)) {

                            auto& joystick = registry.get<UIJoystick>(m_pressed_entity);

                            // Check if already dragging
                            if (joystick.is_dragging) {
                                auto& tex = registry.get<Texture2D>(m_pressed_entity);

                                glm::vec2 normalized_touch = normalizeScreenPosition(m_mouse_position);
                                glm::vec2 drag_offset = normalized_touch - joystick.center_position;
                                float distance = glm::length(drag_offset);

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
                                auto& button = registry.get<UIButton>(m_pressed_entity);
                                if (!button.on_click_callback_lua.empty()) {
                                    auto& ctx = registry.ctx();
                                    if (ctx.contains<LuaManager*>()) {
                                        auto& luaMgr = ctx.get<LuaManager*>();
                                        if (luaMgr) {
                                            luaMgr->callGlobalWithVec2(button.on_click_callback_lua, direction.x, direction.y);
                                        }
                                    }
                                }
                                return true;
                            }
                        }
                    }

                    // Check if we need to START dragging (only for joysticks)
                    if (m_pressed_entity != entt::null && m_joystick_pointer_id == -1) {
                        auto ecs = services.lock()->get<ECS::Controller>();
                        auto& registry = ecs->getRegistry();

                        if (registry.valid(m_pressed_entity) &&
                            registry.all_of<Texture2D, UIButton, UIJoystick>(m_pressed_entity)) {

                            auto& joystick = registry.get<UIJoystick>(m_pressed_entity);

                            glm::vec2 normalized_touch = normalizeScreenPosition(m_mouse_position);
                            glm::vec2 drag_offset = normalized_touch - joystick.center_position;
                            float distance = glm::length(drag_offset);

                            const float drag_threshold = 0.01f;
                            if (distance > drag_threshold && !joystick.is_dragging) {
                                joystick.is_dragging = true;
                                m_joystick_pointer_id = e.getPointerId(); // LOCK POINTER ID
                            }
                        }
                    }

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
                        PN_CORE_INFO("hit detected id={}", e.getPointerId());
                        m_hovered_entity = hit_entity.value();
                        m_pressed_entity = m_hovered_entity;

                        // Store center position ONLY if this is a joystick
                        if (registry.all_of<UIButton, Texture2D, UIJoystick>(m_pressed_entity)) {
                            auto& joystick = registry.get<UIJoystick>(m_pressed_entity);
                            auto& tex = registry.get<Texture2D>(m_pressed_entity);
                            joystick.center_position = tex.pos;
                            joystick.is_dragging = false;
                        }

                        updateButtonState(m_pressed_entity, registry, UIButtonState::Pressed);
                        return true;
                    }
                    return false;
                    });
            }

            if (event.getType() == Event::Type::TouchUp) {
                Event::Dispatcher dispatcher(event);
                dispatcher.Dispatch<Event::TouchUp>([&](Event::TouchUp& e) -> bool {
                    // ✅ Check if this touch up corresponds to a joystick
                    if (m_pressed_entity != entt::null && e.getPointerId() == m_joystick_pointer_id) {
                        auto ecs = services.lock()->get<ECS::Controller>();
                        auto& registry = ecs->getRegistry();

                        if (registry.valid(m_pressed_entity) &&
                            registry.all_of<Texture2D, UIButton, UIJoystick>(m_pressed_entity)) {

                            auto& joystick = registry.get<UIJoystick>(m_pressed_entity);
                            auto& tex = registry.get<Texture2D>(m_pressed_entity);

                            // Reset joystick to center
                            tex.pos = joystick.center_position;

                            // Stop movement
                            auto& button = registry.get<UIButton>(m_pressed_entity);
                            if (!button.on_click_callback_lua.empty()) {
                                auto& ctx = registry.ctx();
                                if (ctx.contains<LuaManager*>()) {
                                    auto& luaMgr = ctx.get<LuaManager*>();
                                    if (luaMgr) {
                                        luaMgr->callGlobalWithVec2(button.on_click_callback_lua, 0.f, 0.f);
                                    }
                                }
                            }

                            joystick.is_dragging = false;
                            m_joystick_pointer_id = -1; // Reset pointer ID
                            m_pressed_entity = entt::null;
                            return true;
                        }
                    }

                    // Normal click logic (non-joystick buttons)
                    if (m_pressed_entity != entt::null) {
                        auto ecs = services.lock()->get<ECS::Controller>();
                        auto& registry = ecs->getRegistry();
                        m_mouse_position = convertToCenterOrigin(glm::vec2(e.getX(), e.getY()));
                        auto hit_entity = raycastUI(m_mouse_position, registry);

                        if (hit_entity.has_value() && hit_entity.value() == m_pressed_entity &&
                            registry.all_of<UIButton>(m_pressed_entity)) {

                            // Only trigger click if NOT a joystick
                            if (!registry.all_of<UIJoystick>(m_pressed_entity)) {
                                PN_CORE_INFO("hit detected up");
                                auto& button = registry.get<UIButton>(m_pressed_entity);
                                if (!button.on_click_callback_lua.empty()) {
                                    auto& ctx = registry.ctx();
                                    if (ctx.contains<LuaManager*>()) {
                                        auto& luaMgr = ctx.get<LuaManager*>();
                                        if (luaMgr) luaMgr->callGlobal(button.on_click_callback_lua);
                                    }
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

                        // Reset joystick if was dragging
                        if (registry.valid(m_pressed_entity) &&
                            registry.all_of<Texture2D, UIJoystick>(m_pressed_entity)) {
                            auto& joystick = registry.get<UIJoystick>(m_pressed_entity);
                            auto& tex = registry.get<Texture2D>(m_pressed_entity);
                            tex.pos = joystick.center_position;
                            joystick.is_dragging = false;
                        }

                        updateButtonState(m_pressed_entity, registry, UIButtonState::Normal);
                        m_pressed_entity = entt::null;
                        m_hovered_entity = entt::null;
                        m_joystick_pointer_id = -1;
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

            std::vector<std::tuple<entt::entity, int, int>> candidates;

            for (auto [entity, tex, element, rect] : view.each()) {


                if (!element.b_is_enabled || !element.b_is_interactable) continue;

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
                glm::vec2 normalized_size = normalizeSize(actual_pixel_size);

                glm::vec2 rect_min = tex.pos - normalized_size;
                glm::vec2 rect_max = tex.pos + normalized_size;

                if (isPointInRect(normalized_mouse, rect_min, rect_max)) {

                    PN_CORE_INFO("POINT IN BUTTON");

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
                                     if(svc_lock) {
                                         auto texture_opt = svc_lock->get<Assets::Manager>()->getAsset<Assets::Texture>(tex.texture_guid);
                                         if (texture_opt.has_value()) {
                                             tex_w = static_cast<float>(texture_opt.value().get()->width);
                                             tex_h = static_cast<float>(texture_opt.value().get()->height);
                                             
                                             // 1.0 pixel padding (aggressively prevent bleeding)
                                             if(tex_w > 0) padding_u = 1.0f / tex_w;
                                             if(tex_h > 0) padding_v = 1.0f / tex_h;
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
                        } else {
                            // If UV component missing, create it?
                            // Typically sysUIAnimation creates it, but if we never played, it might not exist.
                            if (anim.spritesheet_columns > 0 && anim.spritesheet_rows > 0) {
                                registry.emplace<UVCoordinates>(entity);
                                // Recursively call or just copy-paste logic? Copy-paste for safety/speed now
                                auto& uv_comp = registry.get<UVCoordinates>(entity);
                                int col = target_frame % anim.spritesheet_columns;
                                int row = target_frame / anim.spritesheet_columns;
                                float frame_width = 1.0f / anim.spritesheet_columns;
                                float frame_height = 1.0f / anim.spritesheet_rows;
                                uv_comp.uv = glm::vec4(col * frame_width, row * frame_height, (col + 1) * frame_width, (row + 1) * frame_height);
                            }
                        }
                    }
                }
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
