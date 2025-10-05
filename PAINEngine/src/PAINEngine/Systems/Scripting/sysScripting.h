/*****************************************************************//**
 * \file   sysScripting.h
 * \brief  Declaration of scripting system states
 *
 * \author [Your Name], [ID], [email] (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifndef SYS_SCRIPTING_H
#define SYS_SCRIPTING_H

#include "pch.h"
#include "Core.h"

namespace PAIN {

	namespace Scripting {

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
			std::string getSysName() override { return "Scripting System"; }

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
