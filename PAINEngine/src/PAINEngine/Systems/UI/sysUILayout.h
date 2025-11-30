/*****************************************************************//**
 * \file   sysUIInput.h
 * \brief  Declaration of UI Input system
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once
#ifndef SYS_UILAYOUT_H
#define SYS_UILAYOUT_H

#include "ECS/System/ISystem.h"

namespace PAIN {

    namespace UI {
        class LayoutSystem : public ECS::System::ISystem {
        public:
            explicit LayoutSystem(std::shared_ptr<Services> svc);

            ~LayoutSystem();

            void onUpdate(AppTiming timing, entt::registry& registry) override;

            // Get system name
            std::string getSysName() const override { return "UI Layout System"; }

        private:
            void processHierarchy(entt::entity entity, entt::registry& registry, const glm::vec2& parent_size, const glm::vec2& parent_pos);
            bool isPositionBehindCamera(const glm::vec4& clip_space_pos);
            bool isInCameraFrustum(const glm::vec3& ndc);
            glm::vec4 worldToClipSpace(const glm::vec3& world_pos, const glm::mat4& view, const glm::mat4& proj);
            glm::vec3 clipToNDC(const glm::vec4& clip);
            glm::vec2 ndcToScreen(const glm::vec3& ndc, const glm::vec2& viewport);
            glm::vec2 worldToScreen(const glm::vec3& world_pos, const glm::mat4& view, const glm::mat4& proj, const glm::vec2& viewport);
            bool isScreenPosVisible(const glm::vec2& screen_pos, const glm::vec2& viewport, float margin);
            glm::vec3 getEntityWorldPosition(const WorldTransform& transform, const glm::vec3& offset);
            void updateFloatingLabels(entt::registry& registry, const glm::mat4& view, const glm::mat4& proj, const glm::vec2& viewport);
        };
    }



} // namespace PAIN

#endif
