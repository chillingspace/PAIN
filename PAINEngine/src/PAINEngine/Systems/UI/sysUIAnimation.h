/*****************************************************************//**
 * \file   sysUIAnimation.h
 * \brief  Declaration of UI Animation system
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once
#ifndef SYS_UIANIMATION_H
#define SYS_UIANIMATION_H

#include "ECS/System/ISystem.h"
#include "ECS/Components/cUIComps.h"

namespace PAIN {

    namespace UI {
        class AnimationSystem : public ECS::System::ISystem {
        public:
            explicit AnimationSystem(std::shared_ptr<Services> svc);

            ~AnimationSystem();

            void onUpdate(AppTiming timing, entt::registry& registry) override;

            // Get system name
            std::string getSysName() const override { return "UI Animation System"; }

        private:
            float easeInOut(float t);
        };
    }
}

#endif