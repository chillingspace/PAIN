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
			Entity::Type createEntity();

			//Clone entity ( ID of clone returned )
			Entity::Type cloneEntity(Entity::Type copy);

			//Destroy Entity
			void destroyEntity(Entity::Type entity);

			//Check entity
			bool checkEntity(Entity::Type entity) const;

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

				//// Copier for cloning
				//component_copiers[name] = [this](entt::entity src, entt::entity dst) {
				//	if (entt_registry.all_of<T>(src)) {
				//		const T& src_comp = entt_registry.get<T>(src);
				//		entt_registry.emplace<T>(dst, src_comp);
				//	}
				//	};
			}

			// Check if component type is registered
			bool hasComponentFactory(const std::string& name) const {
				return component_factories.find(name) != component_factories.end();
			}

			// Create component by string name (for editor/serialization)
			void addComponentByName(Entity::Type entity, const std::string& name) {
				auto it = component_factories.find(name);
				if (it != component_factories.end()) {
					it->second(static_cast<entt::entity>(entity));
				}
			}

			// Add component to entity (move semantics for efficiency)
			template<typename T>
			void addEntityComponent(Entity::Type entity, T&& component) {
				entt_registry.emplace<T>(static_cast<entt::entity>(entity), std::forward<T>(component));
			}


			template<typename T>
			void removeEntityComponent(Entity::Type entity) {
				entt_registry.remove<T>(static_cast<entt::entity>(entity));
			}

			template<typename T>
			std::optional<std::reference_wrapper<T>> getEntityComponent(Entity::Type entity) {
				if (auto* comp = entt_registry.try_get<T>(static_cast<entt::entity>(entity))) {
					return *comp;
				}
				return std::nullopt;
			}	

			template<typename T>
			bool checkEntityComponent(Entity::Type entity) {
				return entt_registry.all_of<T>(static_cast<entt::entity>(entity));
			}		

			const std::unordered_map<std::string, std::function<void(entt::entity)>>& getComponentFactories() const;

			// Get all component names registered for an entity
			std::vector<std::string> getEntityComponentNames(Entity::Type entity) const;

			// Check if entity has a component by name (uses registered factories)
			bool hasComponentByName(Entity::Type entity, const std::string& name) const;

			// Remove component by name (for editor use)
			void removeComponentByName(Entity::Type entity, const std::string& name);

			// Get component pointer by name (type-erased for editor)
			void* getComponentPtrByName(Entity::Type entity, const std::string& name);

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
