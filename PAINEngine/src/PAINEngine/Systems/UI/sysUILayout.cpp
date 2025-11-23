/*****************************************************************//**
 * \file   sysUILayout.cpp
 * \brief  Definition of UI Layout system functions
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \date   September 2025
 * All content 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysUILayout.h"
#include "ECS/Controller.h"
#include "ECS/sMetaData.h"
#include "Systems/Scripting/GameScriptingSystem.h"
#include "ECS/Components/cUIComps.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Scene/sCameraController.h"

namespace PAIN {
    namespace UI {


        LayoutSystem::LayoutSystem(std::shared_ptr<Services> svc) : ISystem(svc)
        {
        }

        LayoutSystem::~LayoutSystem()
        {
        }

        void LayoutSystem::onUpdate(AppTiming timing, entt::registry& registry) {
            auto svc = services.lock();
            auto metadata_service = svc->get<MetaData::Service>();
            auto window_service = svc->get<Window::Window>();

            // Get camera matrices for world-space UI
            auto camera_service = svc->get<sCameraController>(); // Your camera system
            glm::mat4 view = camera_service->getViewMatrix();
            glm::mat4 projection = camera_service->getProjectionMatrix();
            glm::vec2 viewport = window_service->getFrameBuffer();

            auto canvas_view = registry.view<UICanvas, UIRectTransform>();

            for (auto&& [entity, ui_canvas, ui_rect_trans] : canvas_view.each()) {
                if (!metadata_service->hasParent(entity)) {

                    if (ui_canvas.render_mode == CanvasRenderMode::ScreenSpaceOverlay) {
                        // Current implementation (2D screen space)
                        glm::vec2 screen_size = window_service->getFrameBuffer();
                        processHierarchy(entity, registry, screen_size, glm::vec2(0));
                    }
                    else if (ui_canvas.render_mode == CanvasRenderMode::WorldSpace) {
                        // New: World-space UI
                        processWorldSpaceHierarchy(entity, registry, view, projection, viewport);
                    }
                }
            }
        }


        void LayoutSystem::processHierarchy(entt::entity entity, entt::registry& registry,
            const glm::vec2& parent_size, const glm::vec2& parent_pos) {

            auto metadata_service = services.lock()->get<MetaData::Service>();

            if (!registry.all_of<UIRectTransform>(entity)) return;

            auto& rect = registry.get<UIRectTransform>(entity);

            // Calculate anchor points in parent space
            glm::vec2 anchor_min_pos = parent_pos + parent_size * rect.anchor_min;
            glm::vec2 anchor_max_pos = parent_pos + parent_size * rect.anchor_max;

            // Calculate size based on anchors
            glm::vec2 current_size;
            glm::vec2 current_pos;

            // Convert scale from vec3 to vec2 for UI calculations
            glm::vec2 scale_2d(rect.scale.x, rect.scale.y);

            if (rect.anchor_min == rect.anchor_max) {
                // Anchors together - size_delta is the actual size
                current_size = rect.size_delta * scale_2d;

                // Position = anchor point + anchored_position - (size * pivot)
                glm::vec2 anchor_pos = anchor_min_pos;
                current_pos = anchor_pos + rect.anchored_position - current_size * rect.pivot;
            }
            else {
                // Anchors apart - stretch between them
                // Size = distance between anchors + size_delta (padding)
                current_size = (anchor_max_pos - anchor_min_pos) + rect.size_delta * scale_2d;

                // Position with offset_min (Left, Bottom padding)
                current_pos = anchor_min_pos + rect.offset_min - current_size * rect.pivot;
            }

            // Update calculated world position
            rect.local_position = glm::vec3(current_pos, 0);

            // Store calculated size for raycasting
            rect.calculated_world_size = current_size;
            rect.calculated_world_position = current_pos;

            // Process children recursively
            auto children = metadata_service->getChildren(entity);
            for (auto child : children) {
                processHierarchy(child, registry, current_size, current_pos);
            }
        }

        glm::vec2 LayoutSystem::getParentSize(entt::entity entity, entt::registry& registry) {
            auto svc = services.lock();
            auto metadata_service = svc->get<MetaData::Service>();
            auto window_service = svc->get<Window::Window>();

            auto parent_opt = metadata_service->getParent(entity);

            if (parent_opt.has_value() && registry.all_of<UIRectTransform>(parent_opt.value())) {
                auto& parent_rect = registry.get<UIRectTransform>(parent_opt.value());
                return parent_rect.size_delta;
            }

            return glm::vec2(window_service->getFrameBuffer());
        }


        glm::vec2 LayoutSystem::worldToScreen(const glm::vec3& world_pos, const glm::mat4& view, const glm::mat4& projection, const glm::vec2& viewport_size)
        {
            glm::vec4 clip_pos = projection * view * glm::vec4(world_pos, 1.0f);
            glm::vec3 ndc = glm::vec3(clip_pos) / clip_pos.w;

            // Convert from NDC [-1,1] to screen [0, viewport_size]
            glm::vec2 screen;
            screen.x = (ndc.x + 1.0f) * 0.5f * viewport_size.x;
            screen.y = (1.0f - ndc.y) * 0.5f * viewport_size.y; // Flip Y

            return screen;
        }
        void LayoutSystem::processWorldSpaceHierarchy(entt::entity entity, entt::registry& registry, const glm::mat4& view, const glm::mat4& projection, const glm::vec2& viewport)
        {
            auto metadata_service = services.lock()->get<MetaData::Service>();

            if (!registry.all_of<UIRectTransform>(entity)) return;

            auto& rect = registry.get<UIRectTransform>(entity);
            auto& canvas = registry.get<UICanvas>(entity);

            // Billboard effect (face camera)
            if (canvas.b_face_camera) {
                // Extract camera forward vector and create rotation
                glm::vec3 cam_pos = glm::inverse(view)[3];
                glm::vec3 to_camera = glm::normalize(cam_pos - rect.local_position);
                // Apply billboard rotation to rect.rotation
            }

            // Project world position to screen space for rendering/input
            glm::vec2 screen_pos = worldToScreen(rect.local_position, view, projection, viewport);

            // Calculate size in screen space (distance-based scaling)
            float distance = glm::length(glm::vec3(view * glm::vec4(rect.local_position, 1.0f)));
            glm::vec2 screen_size = rect.size_delta * canvas.world_scale / distance;

            // Store calculated values
            rect.calculated_world_position = screen_pos;
            rect.calculated_world_size = screen_size;

            // Process children
            auto children = metadata_service->getChildren(entity);
            for (auto child : children) {
                processWorldSpaceHierarchy(child, registry, view, projection, viewport);
            }
        }
    }
}
