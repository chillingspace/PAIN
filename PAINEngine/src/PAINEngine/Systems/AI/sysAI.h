/*****************************************************************//**
 * \file   sysAI.h
 * \brief  Declaration of AI system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   5 October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifndef SYS_AI_H
#define SYS_AI_H

#include "pch.h"
#include "Core.h"

namespace PAIN {

	namespace AI {

		class System : public ECS::System::ISystem
		{
		public:
			System();
			~System();

			void onFixedUpdate(AppTiming timing) override;
			void onUpdate(AppTiming timing) override;
			void onAttach() override;
			void onDetach() override;

			// Event handler for app layer
			void onEvent(Event::Event& e) override;
			std::string getSysName() override { return "AI System"; }

			// external
			void enableAI(bool enable);
			bool isAIEnabled() const;

		private:

			// AI system configuration values
			const int c_max_ai_entities;
			const float c_ai_update_interval; // Time between AI updates in seconds

			// AI update tracking
			float accumulated_time;

			// AI state control
			bool b_ai_enabled;

			// AI initialization setup
			void aiSetup();

		};
	}

}

#endif
