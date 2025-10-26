#pragma once

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "pch.h"
#include "Applications/AppSystem.h"
#include "System/ISystem.h"

namespace PAIN {
	namespace ECS {

		class Controller : public AppSystem {
		private:

			size_t entity_count = 0;

			entt::registry entt_registry;

			std::vector<std::shared_ptr<System::ISystem>> systems;

			std::unordered_map<std::string, std::function<void(entt::entity)>> component_factories;

			// Map component names to type check functions
			std::unordered_map<std::string, std::function<bool(entt::entity)>> component_checkers;

			// Map component names to removal functions
			std::unordered_map<std::string, std::function<void(entt::entity)>> component_removers;

			// Map component names to getter functions (returns void*)
			std::unordered_map<std::string, std::function<void* (entt::entity)>> component_getters;

		public:
			explicit Controller(std::shared_ptr<Services> svc) {
				services = svc;
			}

			int getEntitiesCount() const { return static_cast<int>(entity_count); }

			//Dispatch events to layers
			void dispatchToLayers(Event::Event& e);

			//Reverse dispatching to layers
			void dispatchToLayersReversed(Event::Event& e);

			//Update function
			void onFixedUpdate(AppTiming timing) override {}
			void onUpdate(AppTiming timing) override;

			void registerAllComponents();

			//Event callback
			void onEvent(Event::Event& e) override;

			// Direct registry access for advanced use cases
			entt::registry& getRegistry() { return entt_registry; }
			const entt::registry& getRegistry() const { return entt_registry; }

			/*****************************************************************//**
			* Entity Methods
			*********************************************************************/

			//Create Entity
			entt::entity createEntity();

			//Clone entity ( ID of clone returned )
			entt::entity cloneEntity(entt::entity copy);

			//Destroy Entity
			void destroyEntity(entt::entity entity);

			//Check entity
			bool checkEntity(entt::entity entity) const;

			//Destroy Entity
			void destroyAllEntities();

			/*****************************************************************//**
			* Component Methods
			*********************************************************************/
			template<typename T>
			void registerComponent(const std::string& name) {
				// Factory for creating by name
				component_factories[name] = [this](entt::entity e) {
					entt_registry.emplace<T>(e);
					};

				// Checker for hasComponentByName
				component_checkers[name] = [this](entt::entity e) {
					return entt_registry.all_of<T>(e);
					};

				// Remover for removeComponentByName
				component_removers[name] = [this](entt::entity e) {
					entt_registry.remove<T>(e);
					};

				// Getter for editor UI (type-erased)
				component_getters[name] = [this](entt::entity e) -> void* {
					return static_cast<void*>(entt_registry.try_get<T>(e));
					};
			}

			// Check if component type is registered
			bool hasComponentFactory(const std::string& name) const {
				return component_factories.find(name) != component_factories.end();
			}

			// Create component by string name (for editor/serialization)
			void addComponentByName(entt::entity entity, const std::string& name) {
				auto it = component_factories.find(name);
				if (it != component_factories.end()) {
					it->second(entity);
				}
			}

			// Get all component names registered for an entity
			std::vector<std::string> getEntityComponentNames(entt::entity entity) const;

			// Check if entity has a component by name (uses registered factories)
			bool hasComponentByName(entt::entity entity, const std::string& name) const;

			// Remove component by name (for editor use)
			void removeComponentByName(entt::entity entity, const std::string& name);

			// Get component pointer by name (type-erased for editor)
			void* getComponentPtrByName(entt::entity entity, const std::string& name);

			const std::unordered_map<std::string, std::function<void(entt::entity)>>& getComponentFactories() const;

			/*****************************************************************//**
			* Template Component Methods (Type-Safe)
			*********************************************************************/

			// Check if entity has a specific component type
			template<typename T>
			bool hasEntityComponent(entt::entity entity) const {
				return entt_registry.all_of<T>(entity);
			}

			// Get component (non-const version)
			template<typename T>
			std::optional<std::reference_wrapper<T>> getEntityComponent(entt::entity entity) {
				if (!checkEntity(entity) || !entt_registry.all_of<T>(entity)) {
					return std::nullopt;
				}
				return std::ref(entt_registry.get<T>(entity));
			}

			// Get component (const version)
			template<typename T>
			std::optional<std::reference_wrapper<const T>> getEntityComponent(entt::entity entity) const {
				if (!checkEntity(entity) || !entt_registry.all_of<T>(entity)) {
					return std::nullopt;
				}
				return std::ref(entt_registry.get<T>(entity));
			}

			// Add component to entity (move semantics for efficiency)
			template<typename T>
			void addEntityComponent(entt::entity entity, T&& component) {
				// Safety checks to check for valid entity before adding comp to the entity
				if (!checkEntity(entity)) {
					PN_CORE_ERROR("Cannot add component to invalid entity: {}",
						static_cast<uint32_t>(entity));
					return;
				}

				entt_registry.emplace_or_replace<T>(entity, std::forward<T>(component));
			}

			// Remove component from entity
			template<typename T>
			void removeEntityComponent(entt::entity entity) {
				if (entt_registry.all_of<T>(entity)) {
					entt_registry.remove<T>(entity);
				}
			}

			// Alternative name for consistency (checks component exists)
			template<typename T>
			bool checkEntityComponent(entt::entity entity) const {
				return entt_registry.all_of<T>(entity);
			}


			/*****************************************************************//**
			* System Methods
			*********************************************************************/

			// Register a system
			template<typename T> void registerSystem() {
				systems.push_back(std::make_shared<T>(services));
			}

			// Get system by type 
			template<typename T>
			std::shared_ptr<T> getSystem() {
				for (auto& sys : systems) {
					if (auto casted = std::dynamic_pointer_cast<T>(sys)) {
						return casted;
					}
				}
				return nullptr;
			}

			// Get all systems 
			const std::vector<std::shared_ptr<System::ISystem>>& getAllSystems() const {
				return systems;
			}

		};

	}
}

#endif
