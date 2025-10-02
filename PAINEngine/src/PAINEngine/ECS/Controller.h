#pragma once

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "pch.h"
#include "Applications/AppSystem.h"

// ECS files
#include "Entity/sEntity.h"
#include "Components/sComponents.h"
#include "System/sSystem.h"

namespace PAIN {
	namespace ECS {

#ifdef PN_PLATFORM_WINDOWS
		struct EntitiesChanged : public PAIN::Event::Event {
			std::set<Entity::Type> entities;
			EntitiesChanged() = default;
			EntitiesChanged(std::set<Entity::Type> entities) : entities{ entities } {}

			//Register Event
			EVENT_CLASS_TYPE(EntitiesChange);
			EVENT_CLASS_CATEGORY(PAIN::Event::Category::EntityChange);
		};
#endif

		class Controller : public AppSystem {
		private:

			//Vector of systems
			std::vector<std::shared_ptr<System::ISystem>> systems;

			// Unique ptr of ECS coordinators
			std::unique_ptr<Entity::Service> entity_service;
			std::unique_ptr<Component::Service> components_service;

		public:
			Controller();
			~Controller() override = default;

			//Dispatch events to layers
			void dispatchToLayers(Event::Event& e);

			//Reverse dispatching to layers
			void dispatchToLayersReversed(Event::Event& e);

			//Add systems
			void addSystems(std::shared_ptr<System::ISystem> system);

			//Update function
			void onFixedUpdate(AppTiming timing) override {}
			void onUpdate(AppTiming timing) override;

			//Event callback
			void onEvent(Event::Event& e) override;
		};

	}
}

#endif
