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

			// Unique ptr of ECS coordinators
			std::unique_ptr<Entity::Service> entity_service;
			std::unique_ptr<Component::Service> component_service;
			std::unique_ptr<System::Service> system_service;

			static int next_entity_id;

		public:
			Controller();

			//Dispatch events to layers
			void dispatchToLayers(Event::Event& e);

			//Reverse dispatching to layers
			void dispatchToLayersReversed(Event::Event& e);

			//Update function
			void onFixedUpdate(AppTiming timing) override {}
			void onUpdate(AppTiming timing) override;

			//Event callback
			void onEvent(Event::Event& e) override;

			/*****************************************************************//**
			* Entity Methods
			*********************************************************************/

			//Create Entity
			Entity::Type createEntity();

			//Clone entity ( ID of clone returned )
			Entity::Type cloneEntity(Entity::Type copy);

			//Destroy Entity
			void destroyEntity(Entity::Type entity);

			//Check entity
			bool checkEntity(Entity::Type entity) const;

			//Get entity component count
			size_t getEntityComponentCount(Entity::Type entity) const;

			//Destroy Entity
			void destroyAllEntities();

			//Get entity count
			int getEntitiesCount() const;

			//Get all active entities
			const std::set<Entity::Type> getAllEntities() const;

			/*****************************************************************//**
			* Component Methods
			*********************************************************************/
			template<typename T>
			void registerComponent() {
				component_service->registerComponent<T>();
			}
			
			// Remove a component type (removes from all entities first)
			template<typename T>
			void removeComponent() {
				// Get all entities with this component
				Component::Type comp_type = component_service->template getComponentType<T>();
				std::set<Entity::Type> entities =
					component_service->getAllComponentEntities(comp_type);

				// Remove component from each entity
				for (Entity::Type entity : entities) {
					removeEntityComponent<T>(entity);
				}

				// Remove the component type from the manager
				component_service->template unregisterComponent<T>();
			}

			// Add component to entity (move semantics for efficiency)
			template<typename T>
			void addEntityComponent(Entity::Type entity, T&& component) {

				//Add component
				component_service->addEntityComponent<T>(entity, std::forward<T>(component));

				//Set bit signature of component to true
				Component::Signature sign = entity_service->getSignature(entity);

				auto comp_type_opt = component_service->getComponentType<T>();
				if (!comp_type_opt.has_value()) {
					PN_CORE_ERROR("Component type not registered!");
					return;
				}

				sign.set(static_cast<std::size_t>(comp_type_opt.value()), true);
				entity_service->setSignature(entity, sign);

				//sign.set(component_service->getComponentType<T>(), true);
				//entity_service->setSignature(entity, sign);

				//Update entities list
				system_service->updateEntitiesList(entity, sign);
			}

			void addDefEntityComponent(Entity::Type entity, Component::Type type);

			template<typename T>
			void removeEntityComponent(Entity::Type entity) {
				//Remove component
				component_service->removeEntityComponent<T>(entity);

				//Set bit signature of component to false
				Component::Signature sign = entity_service->getSignature(entity);

				auto comp_type_opt = component_service->getComponentType<T>();
				if (comp_type_opt.has_value()) {
					sign.set(static_cast<std::size_t>(comp_type_opt.value()), false);
				}


				entity_service->setSignature(entity, sign);

				//Update entities list
				system_service->updateEntitiesList(entity, sign);
			}

			void removeEntityComponent(Entity::Type entity, Component::Type type);

			template<typename T>
			std::optional<std::reference_wrapper<T>> getEntityComponent(Entity::Type entity) {
				return component_service->getEntityComponent<T>(entity);
			}

			std::shared_ptr<void> getEntityComponent(Entity::Type entity, Component::Type type);

			std::shared_ptr<void> getCopiedEntityComponent(Entity::Type entity, Component::Type type);

			void setEntityComponent(Entity::Type entity, Component::Type type, std::shared_ptr<void> comp);

			template<typename T>
			bool checkEntityComponent(Entity::Type entity) {
				return entity_service->getSignature(entity).test(component_service->getComponentType<T>());
			}

			bool checkEntityComponent(Entity::Type entity, Component::Type component_type) {
				return entity_service->getSignature(entity).test(component_type);
			}

			template<typename T>
			std::optional<Component::Type> getComponentType() {
				return component_service->getComponentType<T>();
			}

			std::optional<Component::Type> getComponentType(std::string const& type) {
				return component_service->getComponentType(type);
			}

			bool checkComponentType(std::string const& type) const;

			size_t getComponentEntitiesCount(Component::Type comp_type) const;

			const std::set<Entity::Type> getAllComponentEntities(Component::Type comp_type) const;

			const std::unordered_map<std::string, std::shared_ptr<void>> getAllEntityComponents(Entity::Type entity) const;

			const std::unordered_map<std::string, std::shared_ptr<void>> getAllCopiedEntityComponents(Entity::Type entity) const;

			const std::unordered_map<std::string, Component::Type> getAllComponentTypes() const;

			size_t getComponentsCount() const;

			/*****************************************************************//**
			* System Methods
			*********************************************************************/

			// Register a system
			template<typename T>
			std::shared_ptr<T> registerSystem(bool components_linked = true, int index = -1) {
				return system_service->registerSystem<T>(components_linked, index);
			}

			// Remove a system
			template<typename T>
			void removeSystem() {
				system_service->removeSystem<T>();
			}

			// Get system index in update order
			template<typename T>
			int getSystemIndex() const {
				return system_service->getSystemIndex<T>();
			}

			// Get system name
			template<typename T>
			std::string getSystemName() const {
				return system_service->getSystemName<T>();
			}

			// Add component type requirement to system
			template<typename T>
			void addSystemComponentType(Component::Type component) {
				system_service->addComponentType<T>(component);
			}

			// Set system active state
			template<typename T>
			void setSystemState(bool state) {
				system_service->setSystemState<T>(state);
			}

			// Get all systems
			std::vector<std::shared_ptr<System::ISystem>>& getAllSystems();

		};

	}
}

#endif
