#include "pch.h"
#include "Controller.h"

namespace PAIN {
	namespace ECS {

		void Controller::dispatchToLayers(Event::Event& e) {
			for (auto it = systems.begin(); it != systems.end(); ++it) {

				//Dispatch event down layers
				(*it)->onEvent(e);
				if (e.checkHandled()) break;
			}
		}

		void Controller::dispatchToLayersReversed(Event::Event& e) {
			if (systems.empty()) return;
			for (auto it = systems.rbegin(); it != systems.rend(); ++it) {

				//Dispatch event down layers
				(*it)->onEvent(e);
				if (e.checkHandled()) break;
			}
		}

		void Controller::addSystems(std::shared_ptr<ISystem> system) {
			systems.push_back(system);
		}

		void Controller::onUpdate() {
			//Iterate through all systems
			for (auto& system : systems) {
				system->onUpdate();
			}
		}

		void Controller::onEvent([[maybe_unused]] Event::Event& e) {

		}
	}
}
