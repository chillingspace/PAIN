/*****************************************************************//**
 * \file   sysAI.cpp
 * \brief  Definition of AI system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysAI.h"

namespace PAIN {

	namespace AI {

		void System::aiSetup()
		{
			// Initialize AI-specific data structures
			// Setup behavior trees, state machines, or decision systems
			// Register AI component types if needed

			accumulated_time = 0.0f;
			b_ai_enabled = true;

			PN_CORE_TRACE("AI System setup complete");
		}

		System::System() : c_max_ai_entities(1024), c_ai_update_interval(0.1f), accumulated_time(0.0f)
		{
			aiSetup();
		}


		System::~System()
		{
			// Cleanup AI resources
			// Clear behavior trees, navigation data, etc.

			PN_CORE_TRACE("AI System destroyed");
		}

		void System::onUpdate(AppTiming timing)
		{
			// Skip AI updates if disabled
			if (!b_ai_enabled) return;

			// Main AI update logic
			// Process AI entities based on components
			// Execute behavior trees, state machines, pathfinding, etc.

			// Implement time-sliced updates for better performance
			accumulated_time += timing.dt;

			if (accumulated_time >= c_ai_update_interval)
			{
				// Update AI logic here
				// Iterate through entities with AI components
				// Execute decision-making, pathfinding, behavior selection

				// TODO: Add actual AI entity processing when AI components are registered
				// Example:
				// for (auto entity : ai_entities) {
				//     processAILogic(entity);
				// }

				accumulated_time -= c_ai_update_interval;
			}
		}

		void System::onFixedUpdate(AppTiming timing)
		{
			// Skip AI updates if disabled
			if (!b_ai_enabled) return;

			// Fixed timestep AI updates
			// Useful for deterministic AI behaviors

			// TODO: Add fixed-timestep AI logic here
			// Example: navigation mesh updates, pathfinding recalculation
		}

		void System::onAttach()
		{
			// Initialize AI world state
			// Setup navigation meshes, decision graphs, etc.
			// Register AI components with ECS

			PN_CORE_INFO("AI SYSTEM INITIALIZED!");
		}

		void System::onDetach()
		{
			// Cleanup on system detachment
			b_ai_enabled = false;

			PN_CORE_TRACE("AI System detached");
		}

		void System::onEvent(Event::Event& e)
		{
			// Handle AI-related events
			// e.g., target acquired, path blocked, entity spawned

			// Example event handling structure:
			// Event::EventDispatcher dispatcher(e);
			// dispatcher.dispatch<Event::EntitySpawnedEvent>(PN_BIND_EVENT_FN(System::onEntitySpawned));
			// dispatcher.dispatch<Event::EntityDestroyedEvent>(PN_BIND_EVENT_FN(System::onEntityDestroyed));
		}

		void System::enableAI(bool enable)
		{
			b_ai_enabled = enable;
			PN_CORE_TRACE("AI System {0}", enable ? "enabled" : "disabled");
		}

		bool System::isAIEnabled() const
		{
			return b_ai_enabled;
		}

	}
}
