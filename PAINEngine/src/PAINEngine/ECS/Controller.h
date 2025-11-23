#pragma once

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "pch.h"
#include "Applications/AppSystem.h"
#include "System/ISystem.h"

#include "CoreSystems/Serialization/sSerialization.h"
#include "ECS/Components/AllComponents.h"

namespace PAIN {
	namespace ECS {

		 // Trait detection for ShouldSerialize flag
		template <typename T, typename = void>
		struct has_should_serialize : std::false_type {};

		template <typename T>
		struct has_should_serialize<T, std::void_t<decltype(T::ShouldSerialize)>> : std::true_type {};

		// Safe compile-time accessor for ShouldSerialize
		template<typename T>
		constexpr bool getShouldSerialize() {
			if constexpr (has_should_serialize<T>::value) {
				return T::ShouldSerialize;
			}
			else {
				return true; // Default: serialize unless explicitly disabled
			}
		}

		template<typename... Components>
		nlohmann::json serializeAllComponentsImpl(
			entt::entity entity,
			const entt::registry& registry,
			std::tuple<Components...>,
			const std::unordered_set<std::string>& filter = {}
		) {
			nlohmann::json components;

			auto expand = [&](auto type_tag) {
				using T = typename decltype(type_tag)::type;

				// Compile-time check: should this type be serialized?
				constexpr bool type_should_serialize = getShouldSerialize<T>();

				// CRITICAL: Wrap entire serialization logic in if constexpr
				if constexpr (type_should_serialize) {
					std::string comp_name = getComponentName<T>();

					// Runtime check: is this component filtered out?
					const bool is_filtered = !filter.empty() && filter.count(comp_name) > 0;

					if (!is_filtered && registry.template all_of<T>(entity)) {
						const auto& comp = registry.template get<T>(entity);

						// Try reflection first, then fallback to JSON
						if constexpr (refl::trait::is_reflectable_v<T>) {
							try {
								components[comp_name] = PAIN::Serialization::to_json_reflected(comp);
							}
							catch (const std::exception& e) {
								PN_CORE_ERROR("Failed to serialize {} using reflection: {}", comp_name, e.what());
							}
						}
						else {
							// Only compile this if type is convertible to JSON
							if constexpr (std::is_constructible_v<nlohmann::json, T>) {
								try {
									components[comp_name] = nlohmann::json(comp);
								}
								catch (const std::exception& e) {
									PN_CORE_WARN("Failed to serialize {}: {}", comp_name, e.what());
								}
							}
							else {
								PN_CORE_WARN("Component {} is not serializable (no reflection or JSON converter)", comp_name);
							}
						}
					}
				}
				};

			(expand(std::type_identity<Components>{}), ...);
			return components;
		}

		template<typename... Components>
		void deserializeComponentsImpl(
			entt::entity entity,
			entt::registry& registry,
			const nlohmann::json& comps,
			std::tuple<Components...>,
			const std::unordered_set<std::string>& filter = {}
		) {
			auto expand = [&](auto type_tag) {
				using T = typename decltype(type_tag)::type;

				// Compile-time check: should this type be deserialized?
				constexpr bool type_should_deserialize = getShouldSerialize<T>();

				// CRITICAL: Wrap entire deserialization logic in if constexpr
				if constexpr (type_should_deserialize) {
					std::string comp_name = getComponentName<T>();

					// Runtime checks
					const bool is_filtered = !filter.empty() && filter.count(comp_name) > 0;

					if (!is_filtered && comps.contains(comp_name)) {
						try {
							T comp;

							// Try reflection first, then fallback to JSON
							if constexpr (refl::trait::is_reflectable_v<T>) {
								PAIN::Serialization::from_json_reflected(comp, comps[comp_name]);
							}
							else {
								// Only compile this if type is convertible from JSON
								if constexpr (std::is_constructible_v<T, nlohmann::json>) {
									comp = comps[comp_name].get<T>();
								}
								else {
									PN_CORE_WARN("Component {} cannot be deserialized from JSON", comp_name);
									return;
								}
							}

							if (registry.template all_of<T>(entity)) {
								registry.template replace<T>(entity, std::move(comp));
							}
							else {
								registry.template emplace<T>(entity, std::move(comp));
							}
						}
						catch (const nlohmann::json::exception& e) {
							PN_CORE_ERROR("Failed to deserialize {} (JSON error): {}", comp_name, e.what());
						}
						catch (const std::exception& e) {
							PN_CORE_ERROR("Failed to deserialize {} (runtime error): {}", comp_name, e.what());
						}
					}
				}
				};

			(expand(std::type_identity<Components>{}), ...);
		}


		class EntityGUIDRegistry {
		private:

			//Bidirectional Mapping
			std::unordered_map<Assets::GUID, entt::entity> guid_to_entity;
			std::unordered_map<entt::entity, Assets::GUID> entity_to_guid;
		public:
			EntityGUIDRegistry() = default;
			~EntityGUIDRegistry() = default;

			//Get or create a new GUID
			Assets::GUID getOrCreateGUID(entt::entity e, entt::registry& registry);

			//Resolve GUID
			entt::entity resolveGUID(const Assets::GUID& guid) const;

			//Update GUID
			void remapGUID(const Assets::GUID& oldGuid, const Assets::GUID& newGuid);

			//Register an entity
			void registerEntity(entt::entity e, const Assets::GUID& guid);

			//Unregister an entity
			void unregisterEntity(entt::entity e);

			//Check for GUID
			bool hasGUID(const Assets::GUID& guid) const;

			//Check for entity
			bool hasEntity(entt::entity e) const;

			//Clear everything
			void clear();
		};

		class Controller : public AppSystem {
		private:

			size_t entity_count = 0;

			// GUID Registry for stable entity references
			EntityGUIDRegistry guid_registry;

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

			//Public access to the GUID registry
			EntityGUIDRegistry& getGUIDRegistry() { return guid_registry; }
			const EntityGUIDRegistry& getGUIDRegistry() const { return guid_registry; }

			Assets::GUID getOrCreateEntityGUID(entt::entity e) {
				return guid_registry.getOrCreateGUID(e, entt_registry);
			}

			entt::entity resolveGUID(const Assets::GUID& guid) const {
				return guid_registry.resolveGUID(guid);
			}

			int getEntitiesCount() const { return static_cast<int>(entity_count); }

			//Dispatch events to layers
			void dispatchToLayers(Event::Event& e);

			//Reverse dispatching to layers
			void dispatchToLayersReversed(Event::Event& e);

			//Update function
			void onFixedUpdate(AppTiming timing) override;
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

			template<typename Tuple, typename F>
			void tuple_for_each(F&& f)
			{
				std::apply([&](auto&&... type_tag) {
					(f(type_tag), ...);
					}, Tuple{});
			}

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

			// Get all components as JSON (for serialization)
			nlohmann::json getAllComponentsAsJson(entt::entity entity) const;


			// Deserialize all components from JSON
			void loadAllComponentsFromJson(entt::entity entity, const nlohmann::json& comps);


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

			// Get All Entity's Component 
			template<typename T>
			std::optional<std::reference_wrapper<T>> getEntityAllComponent(entt::entity entity) {

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

			// Alternative name for consistency (checks component mesh_id)
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
