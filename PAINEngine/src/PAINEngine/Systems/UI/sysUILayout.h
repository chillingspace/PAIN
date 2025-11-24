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
        };
    }



} // namespace PAIN

#endif
