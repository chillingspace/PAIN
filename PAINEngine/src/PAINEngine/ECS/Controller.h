#pragma once

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <memory>
#include <vector>

namespace PAIN {
	namespace ECS {

		class Controller {
		private:

			//Vector of systems
			std::vector<std::shared_ptr<ISystem>> systems;

			//Boolean to stop operation
			bool b_app_running = true;

		public:
			Controller() = default;

			//Dispatch events to layers
			void dispatchToLayers(Event::Event& e);

			//Reverse dispatching to layers
			void dispatchToLayersReversed(Event::Event& e);

			//Add systems
			void addSystems(std::shared_ptr<ISystem> system);

			//Update function
			void onUpdate();

			//Fetch flag
			bool checkAppRunning() const;

			//Terminate
			void terminateApp();
		};

	}
}

#endif
