/*****************************************************************//**
 * \file   sysAnimation.cpp
 * \brief  Definition of animation system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysAnimation.h"

namespace PAIN {

	namespace Animation {

		void System::animationSetup()
		{
			// Initialize animation-specific data structures
			// Setup animation state machines, blend trees
			// Register animation component types if needed
			
			accumulated_time = 0.0f;
			b_animation_enabled = true;
			
			PN_CORE_TRACE("Animation System setup complete");
		}

		System::System(std::shared_ptr<Services> svc) : ISystem(svc), c_max_animated_entities(1024), c_animation_blend_speed(5.0f), accumulated_time(0.0f)
		{
			animationSetup();

			// Above numbers are placeholders
			// c_max_animated_entities will be swapped with ENTITY_MAX or relevant values
			// c_animation_blend_speed controls how fast animations blend between states
		}


		System::~System()
		{
			// Cleanup animation resources
			// Clear animation data, skeleton caches, etc.
			
			PN_CORE_TRACE("Animation System destroyed");
		}

		void System::onUpdate(AppTiming timing, entt::registry& registry)
		{
			// Skip animation updates if disabled
			if (!b_animation_enabled) return;

			// Main animation update logic
			// Process animated entities based on components
			// Update animation times, blend weights, skeletal transforms
			
			accumulated_time += timing.dt;
			
			// TODO: Add actual animation entity processing when animation components are registered
			// Example:
			// for (auto entity : animated_entities) {
			//     updateAnimationState(entity, timing.dt);
			//     blendAnimations(entity);
			//     updateSkeletonTransforms(entity);
			// }
		}

		void System::onEvent(Event::Event& e)
		{
			// Handle animation-related events
			// e.g., animation finished, state changed, blend completed
			
			// Example event handling structure:
			// Event::EventDispatcher dispatcher(e);
			// dispatcher.dispatch<Event::AnimationFinishedEvent>(PN_BIND_EVENT_FN(System::onAnimationFinished));
		}

		void System::enableAnimation(bool enable)
		{
			b_animation_enabled = enable;
			PN_CORE_TRACE("Animation System {0}", enable ? "enabled" : "disabled");
		}

		bool System::isAnimationEnabled() const
		{
			return b_animation_enabled;
		}

	}
}
