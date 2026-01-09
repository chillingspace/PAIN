/*****************************************************************//**
 * \file   sysUILayout.h
 * \brief  Declaration of UI layout system
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once
#ifndef SYS_UIINPUT_H
#define SYS_UIINPUT_H

#include "ECS/System/ISystem.h"
#include "ECS/Components/cUIComps.h"

namespace PAIN {

    namespace UI {
        class InputSystem : public ECS::System::ISystem {
        public:
            explicit InputSystem(std::shared_ptr<Services> svc);

            ~InputSystem();

            // Get system name
            std::string getSysName() const override { return "UI Input System"; }

            void onUpdate(AppTiming timing, entt::registry& registry) override;
            void onEvent(Event::Event& e) override;

        private:
            bool isPointInRect(const glm::vec2& point, const glm::vec2& rect_min,
                const glm::vec2& rect_max);
            std::optional<entt::entity> raycastUI(const glm::vec2& mouse_pos,
                entt::registry& registry);
            void updateButtonState(entt::entity entity, entt::registry& registry, UIButtonState new_state);
            glm::vec2 convertToCenterOrigin(const glm::vec2& screen_pos);
            glm::vec2 normalizeScreenPosition(const glm::vec2& center_origin_pos); 
            glm::vec2 normalizeSize(const glm::vec2& pixel_size);


            entt::entity m_hovered_entity = entt::null;
            entt::entity m_pressed_entity = entt::null;
            glm::vec2 m_mouse_position{ -1.f, -1.f };
        };

    }
}

#endif