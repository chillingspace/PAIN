/*****************************************************************//**
 * \file   sysAnimation.h
 * \brief  Declaration of animation system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifndef SYS_ANIMATION_H
#define SYS_ANIMATION_H

#include "pch.h"
#include "ECS/System/ISystem.h"

namespace PAIN {

	namespace Animation {

		class System : public ECS::System::ISystem
		{
		public:
			explicit System(std::shared_ptr<Services> svc);
			~System();

			// Virtual override methods for system lifecycle
			void onUpdate(AppTiming timing, entt::registry& reg) override;

			// Event handler for app layer
			void onEvent(Event::Event& e) override;
			std::string getSysName() const override { return "Animation System"; }

			// Helper methods for external control
			void enableAnimation(bool enable);
			bool isAnimationEnabled() const;

		private:

			// Animation system configuration values
			const int c_max_animated_entities;
			const float c_animation_blend_speed; // Blending speed for transitions

			// Animation state control
			bool b_animation_enabled;
			float accumulated_time;

			// Animation initialization setup
			void animationSetup();

		};
	}

}

#endif
