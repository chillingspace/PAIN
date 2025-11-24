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
            auto camera_service = svc->get<sCameraController>(); // Your camera system
            glm::mat4 view = camera_service->getViewMatrix();
            glm::mat4 projection = camera_service->getProjectionMatrix();
            glm::vec2 viewport = window_service->getFrameBuffer();

            auto canvas_view = registry.view<UICanvas, UIRectTransform, Entity::Hierarchy>();

            for (auto&& [entity, ui_canvas, ui_rect_trans, hierarchy] : canvas_view.each()) {
                // Check if this canvas has a valid parent
                if (!hierarchy.parentGUID.IsValid()) { // root node (no parent)
                    glm::vec2 screen_size = window_service->getFrameBuffer();

                    // OPTIONAL: Use WorldTransform if you want to support moving screens 
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
        
    }
}
