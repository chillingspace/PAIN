/*****************************************************************//**
 * \file   Controller.h
 * \brief  ECS Controller
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \date   October 2024
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "Controller.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "ECS/Components/GLMSerialization.h"
#include "ECS/Components/AllComponents.h" 

namespace PAIN {
	namespace ECS {

        Assets::GUID EntityGUIDRegistry::getOrCreateGUID(entt::entity e, entt::registry& registry) {
            // Check if entity already has GUID component
            if (auto* guidComp = registry.try_get<Entity::GUID>(e)) {
                // Update registry mapping
                guid_to_entity[guidComp->guid] = e;
                entity_to_guid[e] = guidComp->guid;
                return guidComp->guid;
            }

            // Generate new GUID
            Assets::GUID newGuid = Assets::GUID::Generate();

            // Add component
            registry.emplace<Entity::GUID>(e, newGuid);

            // Register mapping
            guid_to_entity[newGuid] = e;
            entity_to_guid[e] = newGuid;

            return newGuid;
        }

        entt::entity EntityGUIDRegistry::resolveGUID(const Assets::GUID& guid) const {
            auto it = guid_to_entity.find(guid);
            if (it != guid_to_entity.end()) {
                return it->second;
            }
            return entt::null;
        }

        void EntityGUIDRegistry::remapGUID(const Assets::GUID& oldGuid, const Assets::GUID& newGuid) {
            auto it = guid_to_entity.find(oldGuid);
            if (it != guid_to_entity.end()) {
                entt::entity e = it->second;

                // Remove old mapping
                guid_to_entity.erase(it);

                // Add new mapping
                guid_to_entity[newGuid] = e;
                entity_to_guid[e] = newGuid;

                PN_CORE_INFO("[GUID Registry] Remapped entity {} from {} to {}",
                    static_cast<uint32_t>(e),
                    oldGuid.ToString(),
                    newGuid.ToString());
            }
        }

        void EntityGUIDRegistry::registerEntity(entt::entity e, const Assets::GUID& guid) {
            guid_to_entity[guid] = e;
            entity_to_guid[e] = guid;
        }

        void EntityGUIDRegistry::unregisterEntity(entt::entity e) {
            auto it = entity_to_guid.find(e);
            if (it != entity_to_guid.end()) {
                Assets::GUID guid = it->second;
                guid_to_entity.erase(guid);
                entity_to_guid.erase(it);
            }
        }

        bool EntityGUIDRegistry::hasGUID(const Assets::GUID& guid) const {
            return guid_to_entity.find(guid) != guid_to_entity.end();
        }

        bool EntityGUIDRegistry::hasEntity(entt::entity e) const {
            return entity_to_guid.find(e) != entity_to_guid.end();
        }

        void EntityGUIDRegistry::clear() {
            guid_to_entity.clear();
            entity_to_guid.clear();
        }

        void Controller::dispatchToLayers(Event::Event& e) {
            for (auto& sys : systems) {
                if (sys && sys->enabled) {
                    sys->onEvent(e);
                    if (e.checkHandled()) break;
                }
            }
        }

        void Controller::dispatchToLayersReversed(Event::Event& e) {
            if (systems.empty()) return;

            for (auto it = systems.rbegin(); it != systems.rend(); ++it) {
                if (*it && (*it)->enabled) {
                    (*it)->onEvent(e);
                    if (e.checkHandled()) break;
                }
            }
        }

        void Controller::onFixedUpdate(AppTiming timing) {
            //Iterate through all systems for fixed update
            for (auto& sys : systems) {

                // Check if system is enabled or mesh_id first before continueing
                if (!sys || !sys->enabled) {
                    continue;
                }

                try {
                    sys->onFixedUpdate(timing, entt_registry); // Call fixed update
                }
                catch (const std::exception& e) {
                    PN_CORE_ERROR("System '{}' threw exception in onFixedUpdate: {}",
                        sys->getSysName(), e.what());

                    // Disable the problematic system
                    sys->enabled = false;
                }
                catch (...) {
                    // Catch-all for non-standard exceptions (rare but possible)
                    PN_CORE_ERROR("System '{}' threw unknown exception in onFixedUpdate!",
                        sys->getSysName());

                    // Disable the problematic system
                    sys->enabled = false;
                }
            }
        }

		void Controller::onUpdate(AppTiming timing) {
			//Iterate through all systems
            for (auto& sys : systems) {

                // Check if system is enabled or mesh_id first before continueing
                if (!sys || !sys->enabled) {
                    continue;
                }

                try {
                    sys->onUpdate(timing, entt_registry);
                }   
                catch (const std::exception& e) {
                    PN_CORE_ERROR("System '{}' threw exception: {}",
                        sys->getSysName(), e.what());

                    // Disable the problematic system
                    sys->enabled = false;
                }
                catch (...) {
                    // Catch-all for non-standard exceptions (rare but possible)
                    PN_CORE_ERROR("System '{}' threw unknown exception!",
                        sys->getSysName());

                    // Disable the problematic system
                    sys->enabled = false;
                }
            }
		}

        void Controller::registerAllComponents()
        {
            // Entity components
            registerComponent<Entity::GUID>("GUID");
            //registerComponent<Entity::Name>("Name");
            registerComponent<Entity::Hierarchy>("Hierarchy");

            // Core components
            registerComponent<LocalTransform>("LocalTransform");
            registerComponent<WorldTransform>("WorldTransform");
            registerComponent<ModelRenderer>("ModelRenderer");
            registerComponent<Lighting>("Lighting");
            //registerComponent<Hierarchy>("Hierarchy");
            //registerComponent<Camera>("Camera");
            registerComponent<Physics::RigidBody3D>("RigidBody3D");
            registerComponent<Collision::Collider>("Collider");
            registerComponent<Joint>("Joint");
            registerComponent<BoundingVolume>("BoundingVolume");
            registerComponent<Audio::AudioSource>("AudioSource");
            registerComponent<Script>("Script");

            // Metadata components
            registerComponent<MetaData::EntityName>("Name");
            registerComponent<MetaData::Tag>("Tag");
            registerComponent<MetaData::Relation>("Relation");
            registerComponent<MetaData::EditorVisible>("Editor Visiblity");
            registerComponent<MetaData::Group>("Group");
        }

		void Controller::onEvent(Event::Event& e) {
            dispatchToLayers(e);
		}

        /*****************************************************************//**
        * Entity Methods
        *********************************************************************/

        entt::entity Controller::createEntity() {
            entt::entity new_entity = entt_registry.create();
            entity_count++;

            Assets::GUID guid = Assets::GUID::Generate();
            entt_registry.emplace<Entity::GUID>(new_entity, guid);
            guid_registry.registerEntity(new_entity, guid);

            PN_CORE_INFO("Created entity {} with GUID {}",
                static_cast<uint32_t>(new_entity),
                guid.ToString());

            return new_entity;

        }

        entt::entity Controller::cloneEntity(entt::entity copy) {
            if (!checkEntity(copy)) {
                PN_CORE_ERROR("Cannot clone invalid entity: {}", static_cast<uint32_t>(copy));
                return entt::null;
            }

            // Create new entity (auto-assigns new GUID)
            entt::entity clone = createEntity();

            // Copy all components EXCEPT EntityGUID (already has new one)
            for (auto [id, storage] : entt_registry.storage()) {
                if (storage.contains(copy)) {
                    // Skip EntityGUID component (already assigned)
                    if (id == entt::type_hash<Entity::GUID>::value()) {
                        continue;
                    }

                    // Copy component
                    storage.push(clone, storage.value(copy));
                }
            }

            PN_CORE_INFO("Cloned entity {} to {} with new GUID",
                static_cast<uint32_t>(copy),
                static_cast<uint32_t>(clone));

            return clone;
        }


        void Controller::destroyEntity(entt::entity entity) {
            if (!checkEntity(entity)) {
                PN_CORE_ERROR("Attempted to destroy invalid entity: {}",
                    static_cast<uint32_t>(entity));
                return;
            }

            guid_registry.unregisterEntity(entity);

            entt_registry.destroy(entity);
            entity_count--;
        }

        bool Controller::checkEntity(entt::entity entity) const {
            return entt_registry.valid(entity);
        }

        void Controller::destroyAllEntities() {
            entt_registry.clear();
            entity_count = 0;
            guid_registry.clear();
        }

        /*****************************************************************//**
        * Component Methods
        *********************************************************************/

        const std::unordered_map<std::string, std::function<void(entt::entity)>>& Controller::getComponentFactories() const {
            return component_factories;
        }

        std::vector<std::string> Controller::getEntityComponentNames(entt::entity entity) const {
            std::vector<std::string> component_names;

            if (!checkEntity(entity)) {
                return component_names;
            }

            // Iterate all registered component checkers
            for (const auto& [name, checker] : component_checkers) {
                if (checker(entity)) {
                    component_names.push_back(name);
                }
            }

            return component_names;
        }

        template<typename... Components>
        nlohmann::json serializeAllComponentsImpl(
            entt::entity entity,
            const entt::registry& registry,
            std::tuple<Components...>) {

            nlohmann::json components;

            // Use fold expression to check and serialize each component
            ([&] {
                if constexpr (!requires { Components::ShouldSerialize; } || Components::ShouldSerialize) {
                    if (registry.all_of<Components>(entity)) {
                        const auto& comp = registry.get<Components>(entity);
                        std::string comp_name = getComponentName<Components>();

                        // Check if the type is reflectable
                        if constexpr (refl::trait::is_reflectable_v<Components>) {
                            // Use reflection-based serialization
                            components[comp_name] = PAIN::Serialization::to_json_reflected(comp);
                        }
                        else {
                            // Use custom JSON serialization (for glm types)
                            // nlohmann::json will use custom to_json
                            components[comp_name] = nlohmann::json(comp);
                        }
                    }
                }
                }(), ...);

            return components;
        }

        template<typename... Components>
        void Controller::deserializeComponentsImpl(
            entt::entity entity,
            const nlohmann::json& comps,
            std::tuple<Components...>
        ) {
            ([&] {
                if constexpr (!requires { Components::ShouldSerialize; } || Components::ShouldSerialize) {
                    std::string comp_name = getComponentName<Components>();

                    if (comps.contains(comp_name)) {
                        try {
                            Components comp;

                            // Check if type is reflectable
                            if constexpr (refl::trait::is_reflectable_v<Components>) {
                                // Use refl-cpp deserialization
                                PAIN::Serialization::from_json_reflected(comp, comps[comp_name]);
                            }
                            else {
                                // Use adl_serializer (for Transform, Lighting, Physics components)
                                comp = comps[comp_name].get<Components>();
                            }

                            // Add or replace component
                            if (getRegistry().all_of<Components>(entity)) {
                                getRegistry().replace<Components>(entity, std::move(comp));
                            }
                            else {
                                getRegistry().emplace<Components>(entity, std::move(comp));
                            }
                        }
                        catch (const nlohmann::json::exception& e) {
                            PN_CORE_ERROR("Failed to deserialize {} (JSON error): {}", comp_name, e.what());
                        }
                        catch (const std::exception& e) {
                            PN_CORE_ERROR("Failed to deserialize {}: {}", comp_name, e.what());
                        }
                    }
                }
                }(), ...);
        }


        void Controller::loadAllComponentsFromJson(entt::entity entity, const nlohmann::json& comps) {
            if (!comps.is_object()) {
                PN_CORE_WARN("loadAllComponentsFromJson: Expected object, got {}", comps.type_name());
                return;
            }

            deserializeComponentsImpl(entity, comps, AllGameplayComponents{});
        }

        nlohmann::json Controller::getAllComponentsAsJson(entt::entity entity) const {
            if (!checkEntity(entity)) {
                return nlohmann::json::object();
            }

            return serializeAllComponentsImpl(entity, getRegistry(), AllGameplayComponents{});
        }

        bool Controller::hasComponentByName(entt::entity entity, const std::string& name) const {
            auto it = component_checkers.find(name);
            if (it == component_checkers.end()) {
                return false;
            }

            return it->second(entity);
        }

        void Controller::removeComponentByName(entt::entity entity, const std::string& name) {
            auto it = component_removers.find(name);
            if (it != component_removers.end()) {
                it->second(entity);
            }
        }

        void* Controller::getComponentPtrByName(entt::entity entity, const std::string& name) {
            auto it = component_getters.find(name);
            if (it == component_getters.end()) {
                return nullptr;
            }

            return it->second(entity);
        }


	}
}
