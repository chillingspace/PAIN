/*****************************************************************//**
 * \file   sysScripting.h
 * \brief  Declaration of scripting system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifndef SYS_SCRIPTING_H
#define SYS_SCRIPTING_H

#include "pch.h"
#include "ECS/System/ISystem.h"			

namespace PAIN {

	namespace Scripting {

		class System : public ECS::System::ISystem
		{
		public:
			explicit System(std::shared_ptr<Services> svc);
			~System();

			void onUpdate(AppTiming timing, entt::registry& reg) override;

			// Event handler for app layer
			void onEvent(Event::Event& e) override;
			std::string getSysName() const override { return "Scripting System"; }

			// Helper methods for external control
			void enableScripting(bool enable);
			bool isScriptingEnabled() const;

		private:

			// Scripting system configuration values
			const int c_max_script_entities;
			const int c_max_loaded_scripts;

			// Scripting state control
			bool b_scripting_enabled;
			float accumulated_time;

			// Scripting initialization setup
			void scriptingSetup();

		};
	}

}

#endif
