/*****************************************************************//**
 * \file   sysUILayout.cpp
 * \brief  UI layout system definitions
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysUILayout.h"
#include "ECS/Controller.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Scene/sCameraController.h"
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Assets/Types/Texture.h"

namespace PAIN {
    namespace UI {

        LayoutSystem::LayoutSystem(std::shared_ptr<Services> svc) : ISystem(svc) {}

        LayoutSystem::~LayoutSystem() {}

        void LayoutSystem::onUpdate(AppTiming timing, entt::registry& registry) {
            auto svc = services.lock();
            auto window_service = svc->get<Window::Window>();
            auto scene_service = svc->get<Scene::SceneManager>();

            Camera* cam = scene_service->GetActiveCamera();
            
            glm::mat4 view = cam->view();
            glm::mat4 projection = cam->projection();
            glm::vec2 viewport = window_service->getFrameBuffer();

            // ── Update floating labels (UIFollowsWorldEntity) ──
            // Note: updateFloatingLabels rebuilds view/projection from live game camera data
            updateFloatingLabels(registry, view, projection, viewport);

            // ── Process canvas hierarchies ──
            auto canvas_group = registry.group<Entity::Hierarchy>(entt::get<UICanvas, UIRectTransform>);
            glm::vec2 screen_size = viewport;

            for (auto [entity, hierarchy, ui_canvas, ui_rect_trans] : canvas_group.each()) {
                // Only process root canvases (no parent)
                if (hierarchy.parentGUID.IsValid()) continue;

                // Check layer visibility
                if (registry.all_of<Entity::Layer>(entity)) {
                    const auto& layer = registry.get<Entity::Layer>(entity);
                    if (!scene_service->isLayerEnabled(layer.layer_id)) continue;
                }

                // Canvas always fills screen (screen-space only)
                glm::vec2 canvas_pos(0.0f, 0.0f);
                glm::vec2 canvas_size = screen_size;

                ui_rect_trans.calculated_world_position = glm::vec2(0.0f, 0.0f);
                ui_rect_trans.calculated_world_size = canvas_size;

                // Recursively layout children
                processHierarchy(entity, registry, canvas_size, glm::vec2(0.0f, 0.0f));
            }
        }

        void LayoutSystem::processHierarchy(entt::entity entity, entt::registry& registry,
            const glm::vec2& parent_size, const glm::vec2& parent_pos) {
            auto svc = services.lock();
            auto ecs = svc->get<ECS::Controller>();

            if (!registry.all_of<UIRectTransform>(entity)) return;
            if (registry.all_of<UIFollowsWorldEntity>(entity)) return; // Handled separately

            auto& rect = registry.get<UIRectTransform>(entity);

            glm::vec2 anchor_min_pos = parent_pos + (rect.anchor_min - glm::vec2(0.5f)) * parent_size;
            glm::vec2 anchor_max_pos = parent_pos + (rect.anchor_max - glm::vec2(0.5f)) * parent_size;

            glm::vec2 calculated_size;
            glm::vec2 calculated_pos;

            if (rect.anchor_min == rect.anchor_max) {
                // Point anchor mode
                calculated_size = rect.size_delta;
                calculated_pos = anchor_min_pos + rect.anchored_position;
            }
            else {
                // Stretch mode
                calculated_size = anchor_max_pos - anchor_min_pos +
                    glm::vec2(rect.offset_max.x - rect.offset_min.x,
                        rect.offset_max.y - rect.offset_min.y);
                calculated_pos = anchor_min_pos + rect.offset_min;
            }

            // Apply pivot, local offset, and scale
            calculated_pos -= rect.pivot * calculated_size;
            calculated_pos += glm::vec2(rect.local_position);
            calculated_size *= glm::vec2(rect.scale);

            // Store calculated values
            rect.calculated_world_position = calculated_pos;
            rect.calculated_world_size = calculated_size;
            rect.layout_dirty = true;

            // ── Recurse children ──
            if (registry.all_of<Entity::Hierarchy>(entity)) {
                const auto& hierarchy = registry.get<Entity::Hierarchy>(entity);
                for (const auto& childGUID : hierarchy.childrenGUIDs) {
                    entt::entity child = ecs->resolveGUID(childGUID);
                    if (child != entt::null && ecs->checkEntity(child)) {
                        processHierarchy(child, registry, calculated_size, calculated_pos);
                    }
                }
            }
        }

        // ══════════════════════════════════════════════════════════════════
        // Floating Labels 
        // ══════════════════════════════════════════════════════════════════

        void LayoutSystem::updateFloatingLabels(entt::registry& registry, const glm::mat4& view,
            const glm::mat4& proj, const glm::vec2& viewport) {
            auto view_floating = registry.group<UIFollowsWorldEntity>(entt::get<UIRectTransform, UIElement>);
            auto svc = services.lock();
            auto ecs = svc->get<ECS::Controller>();
            auto scene_service = svc->get<Scene::SceneManager>();
            
            // Get the active game camera (updated by Lua thirdPersonCamera.lua every frame)
            Camera* player_camera = scene_service->GetGameCamera();
            if (!player_camera) {
                player_camera = scene_service->GetActiveCamera(); // fallback
                if (!player_camera) return;
            }
            
            // Use camera's current position, forward, and up vectors
            glm::vec3 cam_pos = player_camera->pos;
            glm::vec3 cam_forward = player_camera->forward;
            glm::vec3 cam_up = player_camera->up;
            
            // Rebuild view and projection matrices from the live camera data
            glm::mat4 live_view = glm::lookAt(cam_pos, cam_pos + cam_forward, cam_up);
            glm::mat4 live_proj = glm::perspective(
                glm::radians(player_camera->fov),
                player_camera->aspect_ratio,
                player_camera->near_plane,
                player_camera->far_plane
            );

            for (auto&& [entity, follows, rect, elem] : view_floating.each()) {
                entt::entity target = ecs->resolveGUID(follows.entity_target_guid);

                if (target == entt::null || !registry.valid(target) ||
                    !registry.all_of<WorldTransform>(target)) {
                    PN_CORE_WARN("[UIFollowsWorldEntity] Invalid or missing target entity for UIFollowsWorldEntity - disabling");
                    elem.b_is_enabled = false;
                    continue;
                }

                const auto& world_transform = registry.get<WorldTransform>(target);
                glm::vec3 world_pos = getEntityWorldPosition(world_transform, follows.world_offset);
                glm::vec4 clip = worldToClipSpace(world_pos, live_view, live_proj);

                // Check visibility
                 if (isPositionBehindCamera(clip)) {
                     elem.b_is_enabled = false;
                     continue;
                 }

                glm::vec3 ndc = clipToNDC(clip);
                if (!isInCameraFrustum(ndc)) {
                    elem.b_is_enabled = false;
                    continue;
                }

                elem.b_is_enabled = true;
                rect.calculated_world_position = glm::vec2(ndc.x, ndc.y);

                // Line of Sight check
                auto collision_sys = scene_service->getCameraCollisionSystem();
                if (collision_sys) {
                    float target_dist = glm::length(world_pos - cam_pos);
                    float hit_dist = collision_sys->raycastToPosition(cam_pos, world_pos);

                    // Hit_dist >= 0 means something was hit
                    // If it hit something closer than the target, LOS is blocked
                    if (hit_dist >= 0.0f && hit_dist < target_dist - 0.5f) {
                        elem.b_is_enabled = false;
                        continue;
                    }
                }

                // Distance-based scaling
                float distance = glm::length(world_pos - cam_pos);
                const float reference_distance = 10.0f;
                float scale_factor = glm::clamp(reference_distance / distance, 0.2f, 3.0f);
                // In updateFloatingLabels, after setting calculated_world_position:
                float ndc_scale_y = (rect.size_delta.y * scale_factor) / viewport.y;
                float ndc_scale_x = (rect.size_delta.x * scale_factor) / viewport.x;
                rect.scale = glm::vec3(ndc_scale_x, ndc_scale_y, scale_factor);
                
                // Signal render system to apply these changes to Texture2D and UIText
                rect.layout_dirty = true;
            }
        }

        // ══════════════════════════════════════════════════════════════════
        // World-to-Screen Projection Helpers
        // ══════════════════════════════════════════════════════════════════

        glm::vec4 LayoutSystem::worldToClipSpace(const glm::vec3& world_pos, const glm::mat4& view, const glm::mat4& proj) {
            return proj * view * glm::vec4(world_pos, 1.0f);
        }

        glm::vec3 LayoutSystem::clipToNDC(const glm::vec4& clip) {
            if (clip.w == 0.0f) return glm::vec3(0.0f);
            return glm::vec3(clip) / clip.w;
        }

        glm::vec2 LayoutSystem::ndcToScreen(const glm::vec3& ndc, const glm::vec2& viewport) {
            glm::vec2 screen;
            screen.x = (ndc.x + 1.0f) * 0.5f * viewport.x;
            screen.y = (1.0f - ndc.y) * 0.5f * viewport.y;
            return screen;
        }

        glm::vec2 LayoutSystem::worldToScreen(const glm::vec3& world_pos, const glm::mat4& view,
            const glm::mat4& proj, const glm::vec2& viewport) {
            glm::vec4 clip = worldToClipSpace(world_pos, view, proj);
            if (isPositionBehindCamera(clip)) return { -10000, -10000 };
            glm::vec3 ndc = clipToNDC(clip);
            return ndcToScreen(ndc, viewport);
        }

        bool LayoutSystem::isPositionBehindCamera(const glm::vec4& clip_space_pos) {
            return clip_space_pos.w <= 0.0f;
        }

        bool LayoutSystem::isInCameraFrustum(const glm::vec3& ndc) {
            return ndc.x >= -1.0f && ndc.x <= 1.0f &&
                ndc.y >= -1.0f && ndc.y <= 1.0f &&
                ndc.z >= -1.0f && ndc.z <= 1.0f;
        }

        bool LayoutSystem::isScreenPosVisible(const glm::vec2& screen_pos, const glm::vec2& viewport, float margin) {
            return screen_pos.x >= -margin && screen_pos.x <= viewport.x + margin &&
                screen_pos.y >= -margin && screen_pos.y <= viewport.y + margin;
        }

        glm::vec3 LayoutSystem::getEntityWorldPosition(const WorldTransform& transform, const glm::vec3& offset) {
            glm::vec3 world_pos = glm::vec3(transform.matrix[3]); 
            return world_pos + offset;
        }

    } // namespace UI
} // namespace PAIN
