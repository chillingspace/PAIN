#pragma once

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "pch.h"
#include "Applications/AppSystem.h"

// ECS files
#include "Entity/sEntity.h"

namespace PAIN {
	namespace ECS {

		class Controller : public AppSystem {
		private:

			//Vector of systems
			std::vector<std::shared_ptr<ISystem>> systems;

			// Unique ptr of ECS coordinators
			std::unique_ptr<Entity::Service> entity_service;

		public:
			Controller();
			~Controller() override = default;

			//Dispatch events to layers
			void dispatchToLayers(Event::Event& e);

			//Reverse dispatching to layers
			void dispatchToLayersReversed(Event::Event& e);

			//Add systems
			void addSystems(std::shared_ptr<ISystem> system);

			//Update function
			void onFixedUpdate(AppTiming timing) override {}
			void onUpdate(AppTiming timing) override;

			//Event callback
			void onEvent(Event::Event& e) override;
		};

	}
}

#endif
