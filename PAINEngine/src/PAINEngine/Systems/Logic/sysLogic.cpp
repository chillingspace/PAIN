/*****************************************************************//**
 * \file   sysLogic.cpp
 * \brief  Definition of gameplay logic system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysLogic.h"

namespace PAIN {

	namespace Logic {

		void System::logicSetup()
		{
			// Initialize logic-specific data structures
			// Register components for objectives, triggers, score, etc.
			
			accumulated_time = 0.0f;
			b_logic_enabled = true;
			game_state = GameState::Playing;

			PN_CORE_TRACE("Logic System setup complete");
		}

		System::System() : c_max_logic_entities(1024), c_logic_tick_interval(0.05f), accumulated_time(0.0f)
		{
			logicSetup();
		}

		System::~System()
		{
			// Cleanup logic state
			PN_CORE_TRACE("Logic System destroyed");
		}

		void System::onUpdate(AppTiming timing)
		{
			if (!b_logic_enabled) return;

			// Pause-aware logic updates
			if (game_state == GameState::Paused) return;

			// Time-sliced logic tick
			accumulated_time += timing.dt;

			while (accumulated_time >= c_logic_tick_interval)
			{
				// TODO: Process gameplay logic:
				// - Objectives progression
				// - Trigger volumes and interactions
				// - Score and timer updates
				// - Wave spawners or level scripting hooks

				// Example pseudo:
				// for (auto e : logic_entities) {
				//     updateObjective(e);
				//     checkTriggers(e);
				// }

				accumulated_time -= c_logic_tick_interval;
			}
		}

		void System::onFixedUpdate(AppTiming timing)
		{
			if (!b_logic_enabled) return;

			// Optional deterministic logic here
			// Example: fixed-step timers, deterministic triggers
		}

		void System::onAttach()
		{
			// Initialize world-level logic
			// Load mission data, reset counters, setup triggers

			PN_CORE_INFO("LOGIC SYSTEM INITIALIZED!");
		}

		void System::onDetach()
		{
			b_logic_enabled = false;
			PN_CORE_TRACE("Logic System detached");
		}

		void System::onEvent(Event::Event& e)
		{
			// Handle gameplay events
			// Example: LevelLoaded, EntityDied, ObjectiveCompleted, Pause/Resume

			// Event::EventDispatcher dispatcher(e);
			// dispatcher.dispatch<Event::PauseEvent>([this](auto&) { game_state = GameState::Paused; return true; });
			// dispatcher.dispatch<Event::ResumeEvent>([this](auto&) { game_state = GameState::Playing; return true; });
		}

		void System::enableLogic(bool enable)
		{
			b_logic_enabled = enable;
			PN_CORE_TRACE("Logic System {0}", enable ? "enabled" : "disabled");
		}

		bool System::isLogicEnabled() const
		{
			return b_logic_enabled;
		}

	}
}
