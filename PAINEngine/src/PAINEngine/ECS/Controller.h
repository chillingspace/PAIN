#pragma once

#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "Applications/AppSystem.h"
#include "System/ISystem.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "Utility/Log.h"

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

			(expand(entt::type_identity<Components>{}), ...);
			return components;
		}

		template<typename... Components>
		void deserializeAllComponentsImpl(
			entt::entity entity,
			entt::registry& registry,
			const nlohmann::json& components,
			std::tuple<Components...>,
			const std::unordered_set<std::string>& filter = {}
		) {
			// Validate input
			if (!components.is_object()) {
				PN_CORE_WARN("deserializeAllComponentsImpl: Expected JSON object, got {}", components.type_name());
				return;
			}

			auto expand = [&](auto type_tag) {
				using T = typename decltype(type_tag)::type;

				// Compile-time check: should this type be deserialized?
				constexpr bool type_should_deserialize = getShouldSerialize<T>();

				// CRITICAL: Wrap entire deserialization logic in if constexpr
				if constexpr (type_should_deserialize) {
					std::string comp_name = getComponentName<T>();

					// Runtime check: is this component filtered out?
					const bool is_filtered = !filter.empty() && filter.count(comp_name) > 0;

					// Check if component exists in JSON
					if (!is_filtered && components.contains(comp_name)) {
						try {
							T comp;

							// Try reflection first, then fallback to JSON
							if constexpr (refl::trait::is_reflectable_v<T>) {
								try {
									PAIN::Serialization::from_json_reflected(comp, components[comp_name]);
								}
								catch (const std::exception& e) {
									PN_CORE_ERROR("Failed to deserialize {} using reflection: {}", comp_name, e.what());
									return; // Skip this component
								}
							}
							else {
								// Only compile this if type is convertible from JSON
								if constexpr (std::is_constructible_v<T, nlohmann::json>) {
									try {
										comp = components[comp_name].get<T>();
									}
									catch (const std::exception& e) {
										PN_CORE_WARN("Failed to deserialize {}: {}", comp_name, e.what());
										return; // Skip this component
									}
								}
								else {
									PN_CORE_WARN("Component {} is not deserializable (no reflection or JSON converter)", comp_name);
									return; // Skip this component
								}
							}

							// Successfully deserialized - now emplace or replace
							if (registry.template all_of<T>(entity)) {
								registry.template replace<T>(entity, std::move(comp));
							}
							else {
								registry.template emplace<T>(entity, std::move(comp));
							}
						}
						catch (const std::exception& e) {
							PN_CORE_ERROR("Unexpected error deserializing {}: {}", comp_name, e.what());
						}
					}
				}
				};

			(expand(entt::type_identity<Components>{}), ...);
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

		//Registry type alias
		using RegistryID = uint32_t;
		const RegistryID MAIN_REGISTRY_ID = 0;

		//Registry context
		struct RegistryContext {
			std::string name;
			entt::registry registry;
			EntityGUIDRegistry guid_registry;
			size_t entity_count = 0;
			bool auto_simulate = false;
		};

		//Improved controller with optional registry usage
		class Controller : public AppSystem {
		private:

			//Multi-registry support
			std::unordered_map<RegistryID, RegistryContext> registries;
			RegistryID next_registry_id = 1;

			std::vector<std::shared_ptr<System::ISystem>> systems;

			std::unordered_map<std::string, std::function<void(entt::entity, RegistryID)>> component_factories;

			// Map component names to type check functions
			std::unordered_map<std::string, std::function<bool(entt::entity, RegistryID)>> component_checkers;

			// Map component names to removal functions
			std::unordered_map<std::string, std::function<void(entt::entity, RegistryID)>> component_removers;

			// Map component names to getter functions (returns void*)
			std::unordered_map<std::string, std::function<void* (entt::entity, RegistryID)>> component_getters;

			// Map component names to serialization functions
			std::unordered_map<std::string, std::function<nlohmann::json(entt::entity, RegistryID)>> component_serializers;

			// Helper methods for multi-registry support
			RegistryContext* getRegistryContext(RegistryID id);
			const RegistryContext* getRegistryContext(RegistryID id) const;
			
			// Update systems for a specific registry
			void updateSystemsForRegistry(RegistryID id, AppTiming timing, bool isFixed);

		public:
			explicit Controller(std::shared_ptr<Services> svc);

			/*****************************************************************//**
			* Registry Management Methods
			*********************************************************************/
			//Create a new registry with optional name and auto-simulate flag
			RegistryID createRegistry(const std::string& name = "", bool autoSimulate = false);
			
			//Destroy a registry (cannot destroy main registry)
			void destroyRegistry(RegistryID id);

			//Check if registry exists
			bool hasRegistry(RegistryID id) const;

			//Registry configuration
			void setRegistryAutoSimulate(RegistryID id, bool autoSimulate);
			bool isRegistryAutoSimulate(RegistryID id) const;
			std::string getRegistryName(RegistryID id) const;

			//Registry updates
			void updateRegistry(RegistryID id, AppTiming timing);
			void fixedUpdateRegistry(RegistryID id, AppTiming timing);

			/*****************************************************************//**
			* GUID Registry Methods
			*********************************************************************/
			//Public access to the GUID registry
			EntityGUIDRegistry& getGUIDRegistry(RegistryID registryId = MAIN_REGISTRY_ID);
			const EntityGUIDRegistry& getGUIDRegistry(RegistryID registryId = MAIN_REGISTRY_ID) const;
			Assets::GUID getOrCreateEntityGUID(entt::entity e, RegistryID registryId = MAIN_REGISTRY_ID);
			entt::entity resolveGUID(const Assets::GUID& guid, RegistryID registryId = MAIN_REGISTRY_ID) const;
			int getEntitiesCount(RegistryID registryId = MAIN_REGISTRY_ID) const;

			//Dispatch events to layers
			void dispatchToLayers(Event::Event& e);

			//Reverse dispatching to layers
			void dispatchToLayersReversed(Event::Event& e);

			//Update function
			void onFixedUpdate(AppTiming timing) override;
			void onUpdate(AppTiming timing) override;

			void registerAllComponents();

			void registerAllSystems();

			//Event callback
			void onEvent(Event::Event& e) override;

			//Overloaded to support multi-registry
			entt::registry& getRegistry(RegistryID id = MAIN_REGISTRY_ID);
			const entt::registry& getRegistry(RegistryID id = MAIN_REGISTRY_ID) const;

			/*****************************************************************//**
			* Entity Methods
			*********************************************************************/

			//Create Entity
			entt::entity createEntity(RegistryID registryId = MAIN_REGISTRY_ID);
			
			//Create Entity with GUID
			entt::entity createEntity(Assets::GUID const& e_id, RegistryID registryId = MAIN_REGISTRY_ID);
			
			//Clone entity (supports cross-registry cloning)
			entt::entity cloneEntity(entt::entity copy, RegistryID srcRegistry = MAIN_REGISTRY_ID, RegistryID dstRegistry = MAIN_REGISTRY_ID);
			
			//Destroy Entity
			void destroyEntity(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID);
			
			//Check entity
			bool checkEntity(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) const;
			
			//Destroy all entities in a registry
			void destroyAllEntities(RegistryID registryId = MAIN_REGISTRY_ID);

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
				// Note: These lambdas work with MAIN_REGISTRY_ID for backward compatibility
				// For multi-registry support, use the template component methods with registryId parameter
				
				// Factory for creating by name
				component_factories[name] = [this](entt::entity e, RegistryID const& registry_id) {
					getRegistry(registry_id).emplace<T>(e);
					};
				// Checker for hasComponentByName
				component_checkers[name] = [this](entt::entity e, RegistryID const& registry_id) {
					return getRegistry(registry_id).all_of<T>(e);
					};
				// Remover for removeComponentByName
				component_removers[name] = [this](entt::entity e, RegistryID const& registry_id) {
					getRegistry(registry_id).remove<T>(e);
					};
				// Getter for editor UI (type-erased)
				component_getters[name] = [this](entt::entity e, RegistryID const& registry_id) -> void* {
					return static_cast<void*>(getRegistry(registry_id).try_get<T>(e));
					};
				// Getter for comp serializers
				component_serializers[name] = [this](entt::entity e, RegistryID const& registry_id) -> nlohmann::json {
					return PAIN::Serialization::to_json_reflected(getRegistry(registry_id).try_get<T>(e));
					};
			}

			// Check if component type is registered
			bool hasComponentFactory(const std::string& name) const {
				return component_factories.find(name) != component_factories.end();
			}

			// Create component by string name (for editor/serialization)
			void addComponentByName(entt::entity entity, const std::string& name, RegistryID registry_id = MAIN_REGISTRY_ID) {
				auto it = component_factories.find(name);
				if (it != component_factories.end()) {
					it->second(entity, registry_id);
				}
			}

			// Get all component names registered for an entity
			std::vector<std::string> getEntityComponentNames(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) const;

			//Get component name
			std::vector<std::string> getComponentNames(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) const;

			// Get all components as JSON (for serialization)
			nlohmann::json getAllComponentsAsJson(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) const;

			//Get components as json
			nlohmann::json getComponentAsJson(entt::entity entity, std::string const& comp_name, RegistryID registryId) const;

			// Deserialize all components from JSON
			void loadAllComponentsFromJson(entt::entity entity, const nlohmann::json& comps, RegistryID registryId = MAIN_REGISTRY_ID);


			// Check if entity has a component by name (uses registered factories)
			bool hasComponentByName(entt::entity entity, const std::string& name, RegistryID registryId = MAIN_REGISTRY_ID) const;

			// Remove component by name (for editor use)
			void removeComponentByName(entt::entity entity, const std::string& name, RegistryID registryId = MAIN_REGISTRY_ID);

			// Get component pointer by name (type-erased for editor)
			void* getComponentPtrByName(entt::entity entity, const std::string& name, RegistryID registryId = MAIN_REGISTRY_ID);

			const std::unordered_map<std::string, std::function<void(entt::entity, RegistryID)>>& getComponentFactories() const;

			/*****************************************************************//**
			* Template Component Methods (Type-Safe)
			*********************************************************************/

			// Check if entity has a specific component type
			template<typename T>
			bool hasEntityComponent(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) const {
				return getRegistry(registryId).all_of<T>(entity);
			}

			// Get component (non-const version)
			template<typename T>
			std::optional<std::reference_wrapper<T>> getEntityComponent(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) {
				if (!checkEntity(entity, registryId) || !getRegistry(registryId).all_of<T>(entity)) {
					return std::nullopt;
				}
				return std::ref(getRegistry(registryId).get<T>(entity));
			}

			// Get All Entity's Component 
			template<typename T>
			std::optional<std::reference_wrapper<T>> getEntityAllComponent(entt::entity entity) {

			}

			// Get component (const version)
			template<typename T>
			std::optional<std::reference_wrapper<const T>> getEntityComponent(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) const {
				if (!checkEntity(entity, registryId) || !getRegistry(registryId).all_of<T>(entity)) {
					return std::nullopt;
				}
				return std::ref(getRegistry(registryId).get<T>(entity));
			}

			// Add component to entity (move semantics for efficiency)
			template<typename T>
			void addEntityComponent(entt::entity entity, T&& component, RegistryID registryId = MAIN_REGISTRY_ID) {
				if (!checkEntity(entity, registryId)) {
					PN_CORE_ERROR("Cannot add component to invalid entity: {}",
						static_cast<uint32_t>(entity));
					return;
				}
				getRegistry(registryId).emplace_or_replace<T>(entity, std::forward<T>(component));
			}

			// Remove component from entity
			template<typename T>
			void removeEntityComponent(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) {
				if (getRegistry(registryId).all_of<T>(entity)) {
					getRegistry(registryId).remove<T>(entity);
				}
			}

			// Alternative name for consistency (checks component mesh_id)
			template<typename T>
			bool checkEntityComponent(entt::entity entity, RegistryID registryId = MAIN_REGISTRY_ID) const {
				return getRegistry(registryId).all_of<T>(entity);
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
