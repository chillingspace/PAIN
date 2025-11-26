/*****************************************************************//**
 * \file   sysUILayout.cpp
 * \brief  Definition of UI Layout system functions
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \date   September 2025
 * All content 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "Core.h"
#include "sysUILayout.h"
#include "ECS/Controller.h"
#include "ECS/sMetaData.h"
#include "Systems/Scripting/GameScriptingSystem.h"
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
            auto camera_service = svc->get<sCameraController>();
            glm::mat4 view = camera_service->getViewMatrix();
            glm::mat4 projection = camera_service->getProjectionMatrix();
            glm::vec2 viewport = window_service->getFrameBuffer();

            // For every UI label with a UIFollowsWorldEntity, the position is projected from world to screen and written to the label's UIRectTransform.calculated_world_position.
            updateFloatingLabels(registry, view, projection, viewport);

            auto canvas_view = registry.view<UICanvas, UIRectTransform, Entity::Hierarchy>();

            for (auto&& [entity, ui_canvas, ui_rect_trans, hierarchy] : canvas_view.each()) {
                // Check if this canvas has a valid parent
                if (!hierarchy.parentGUID.IsValid()) { // root node (no parent)
                    glm::vec2 screen_size = window_service->getFrameBuffer();
                    glm::vec2 layout_origin(0);
                    if (registry.all_of<WorldTransform>(entity)) {
                        const auto& wtrans = registry.get<WorldTransform>(entity);
                        layout_origin = glm::vec2(wtrans.matrix[3]);
                    }

                    processHierarchy(entity, registry, screen_size, layout_origin);
                }
            }
        }


        void LayoutSystem::processHierarchy( entt::entity entity, entt::registry& registry, const glm::vec2& parent_size, const glm::vec2& parent_pos)
        {
            auto svc = services.lock();
            auto ecs = svc->get<ECS::Controller>();
            // Use WorldTransform for UI position
            glm::vec2 calculated_pos = parent_pos;
            glm::vec2 calculated_size = parent_size;
            if (registry.all_of<WorldTransform>(entity)) {
                const auto& w_trans = registry.get<WorldTransform>(entity);
                calculated_pos = glm::vec2(w_trans.matrix[3]);
                // Calculate size using UI-specific logic or from the overall world scale
                glm::vec3 world_scale(
                    glm::length(glm::vec3(w_trans.matrix[0])),
                    glm::length(glm::vec3(w_trans.matrix[1])),
                    glm::length(glm::vec3(w_trans.matrix[2]))
                );
                // If UIRectTransform has local size, multiply by world_scale.xy for final size
                if (registry.all_of<UIRectTransform>(entity)) {
                    auto& rect = registry.get<UIRectTransform>(entity);
                    calculated_size = rect.size_delta * glm::vec2(world_scale);
                    rect.calculated_world_position = calculated_pos;
                    rect.calculated_world_size = calculated_size;
                }
            }

            // Traverse children using Hierarchy 
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

        glm::vec2 LayoutSystem::worldToScreen(const glm::vec3& world_pos, const glm::mat4& view, const glm::mat4& proj, const glm::vec2& viewport)
        {
            glm::vec4 clip = proj * view * glm::vec4(world_pos, 1.0f);
            if (clip.w == 0.0f) return { -10000, -10000 }; // clearly "offscreen"
            glm::vec3 ndc = glm::vec3(clip) / clip.w;
            // If behind camera, we also consider as offscreen
            if (clip.w < 0.0f) return { -10000, -10000 };
            glm::vec2 screen;
            screen.x = (ndc.x * 0.5f + 0.5f) * viewport.x;
            screen.y = (1.0f - (ndc.y * 0.5f + 0.5f)) * viewport.y; // flip Y
            return screen;
        }

        void LayoutSystem::updateFloatingLabels(entt::registry& registry, const glm::mat4& view, const glm::mat4& proj, const glm::vec2& viewport)
        {
            auto view_floating = registry.view<UIFollowsWorldEntity, UIRectTransform>();
            auto svc = services.lock();
            auto metadata_service = svc->get<MetaData::Service>();

            for (auto&& [entity, follows, rect] : view_floating.each()) {

                // Entity here is invalid !!!!!!!!!!!!!!!!!!!
                auto ent_opt = metadata_service->getEntityByName(follows.entity_target_string);

                if (!ent_opt) continue;

                if (!registry.valid(ent_opt.value()) || !registry.all_of<WorldTransform>(ent_opt.value()))
                    continue;

                std::string ent_name = metadata_service->getEntityName(entity);

                const auto& world = registry.get<WorldTransform>(ent_opt.value());
                glm::vec3 world_pos = glm::vec3(world.matrix * glm::vec4(follows.world_offset, 1.f));
                glm::vec4 clip = proj * view * glm::vec4(world_pos, 1.0f);

                glm::vec2 screen_pos = worldToScreen(world_pos, view, proj, viewport);

                rect.calculated_world_position = screen_pos;
                // Optionally, allow a pixel offset in UIFollowsWorldEntity for vertical separation
            }
        }
        
    }
}
