#pragma once

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include <memory>
#include <vector>

#include "Applications/AppLayer.h"

namespace PAIN {
	namespace ECS {

		class Controller : public AppLayer {
		private:

			//Vector of systems
			std::vector<std::shared_ptr<ISystem>> systems;

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

			//Event callback
			void onEvent(Event::Event& e) override;
		};

	}
}

#endif
