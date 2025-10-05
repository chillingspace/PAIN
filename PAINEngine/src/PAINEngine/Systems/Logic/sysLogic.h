/*****************************************************************//**
 * \file   sysLogic.h
 * \brief  Declaration of gameplay logic system states
 *
 * \author [Your Name], [ID], [email] (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifndef SYS_LOGIC_H
#define SYS_LOGIC_H

#include "pch.h"
#include "Core.h"

namespace PAIN {

	namespace Logic {

		class System : public ECS::System::ISystem
		{
		public:
			System();
			~System();

			// Virtual override methods for system lifecycle
			void onFixedUpdate(AppTiming timing) override;
			void onUpdate(AppTiming timing) override;
			void onAttach() override;
			void onDetach() override;

			// Event handler for app layer
			void onEvent(Event::Event& e) override;
			std::string getSysName() override { return "Logic System"; }

			// Helper methods for external control
			void enableLogic(bool enable);
			bool isLogicEnabled() const;

		private:

			// Logic system configuration values
			const int c_max_logic_entities;
			const float c_logic_tick_interval; // Time between logic ticks

			// Logic state control
			bool b_logic_enabled;
			float accumulated_time;

			// Initialization setup
			void logicSetup();

			// Optional: simple game state tracker
			enum class GameState { None, Playing, Paused, GameOver };
			GameState game_state;
		};
	}

}

#endif
