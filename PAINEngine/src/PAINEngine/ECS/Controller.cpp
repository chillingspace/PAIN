#include "pch.h"
#include "Controller.h"

namespace PAIN {
	namespace ECS {
		Controller::Controller() : entity_service{ std::make_unique<Entity::Service>() }, components_service{ std::make_unique<Component::Service>() }
		{
		}

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

		void Controller::addSystems(std::shared_ptr<System::ISystem> system) {
			systems.push_back(system);
			system->onAttach();
		}

		void Controller::onUpdate(AppTiming timing) {
			//Iterate through all systems
			for (auto& system : systems) {
				system->onUpdate();
			}
		}

		void Controller::onEvent([[maybe_unused]] Event::Event& e) {

		}
	}
}
