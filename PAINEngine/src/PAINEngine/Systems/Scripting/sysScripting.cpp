/*****************************************************************//**
 * \file   sysScripting.cpp
 * \brief  Definition of scripting system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysScripting.h"

namespace PAIN {

	namespace Scripting {

		void System::scriptingSetup()
		{
			// Initialize scripting-specific data structures
			// Setup scripting engine (e.g., Lua, Python, or custom)
			// Register script component types if needed
			// Load and compile scripts
			
			accumulated_time = 0.0f;
			b_scripting_enabled = true;
			
			PN_CORE_TRACE("Scripting System setup complete");
		}

		System::System(std::shared_ptr<Services> svc) : ISystem(svc), c_max_script_entities(1024), c_max_loaded_scripts(256), accumulated_time(0.0f)
		{
			scriptingSetup();

			// Above numbers are placeholders
			// c_max_script_entities will be swapped with ENTITY_MAX or relevant values
			// c_max_loaded_scripts limits the number of scripts that can be loaded simultaneously
		}


		System::~System()
		{
			// Cleanup scripting resources
			// Unload scripts, cleanup scripting engine
			// Clear script caches
			
			PN_CORE_TRACE("Scripting System destroyed");
		}

		void System::onUpdate(AppTiming timing, entt::registry& registry)
		{
			// Skip script updates if disabled
			if (!b_scripting_enabled) return;

			// Main scripting update logic
			// Process entities with script components
			// Execute script update callbacks
			
			accumulated_time += timing.dt;
			
			// TODO: Add actual script entity processing when script components are registered
			// Example:
			// for (auto entity : scripted_entities) {
			//     executeScriptUpdate(entity, timing.dt);
			//     handleScriptCallbacks(entity);
			// }
			
			// TODO: Script hot-reloading support
			// Check for modified script files and reload if needed
		}

		void System::onEvent(Event::Event& e)
		{
			// Handle scripting-related events
			// e.g., script error, script loaded, script execution finished
			
			// TODO: Forward relevant events to scripts
			// Example event handling structure:
			// Event::EventDispatcher dispatcher(e);
			// dispatcher.dispatch<Event::ScriptErrorEvent>(PN_BIND_EVENT_FN(System::onScriptError));
			// dispatcher.dispatch<Event::ScriptLoadedEvent>(PN_BIND_EVENT_FN(System::onScriptLoaded));
			
			// Scripts may also want to receive game events (collision, input, etc.)
			// Forward events to active scripts for handling
		}

		void System::enableScripting(bool enable)
		{
			b_scripting_enabled = enable;
			PN_CORE_TRACE("Scripting System {0}", enable ? "enabled" : "disabled");
		}

		bool System::isScriptingEnabled() const
		{
			return b_scripting_enabled;
		}

	}
}
