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
#include "ECS/Components/cAudioSource.h"
#include "ECS/Components/cBoundingVolume.h"

namespace PAIN {
	namespace ECS {


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

		void Controller::onUpdate(AppTiming timing) {
			//Iterate through all systems
            for (auto& sys : systems) {

                // Check if system is enabled or exists first before continueing
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
            // Core components
            registerComponent<Transform>("Transform");
            registerComponent<MeshRenderer>("MeshRenderer");
            registerComponent<Lighting>("Lighting");
            registerComponent<Hierarchy>("Hierarchy");
            //registerComponent<Camera>("Camera");
            registerComponent<Physics::RigidBody3D>("RigidBody3D");
            registerComponent<Collision::Collider>("Collider");
            registerComponent<Joint>("Joint");
            registerComponent<cBoundingVolume>("BoundingVolume");
            registerComponent<Audio::AudioSource>("AudioSource");

            // Metadata components
            registerComponent<MetaData::EntityName>("Name");
            registerComponent<MetaData::Tag>("Tag");
            registerComponent<MetaData::Relation>("Relation");
            registerComponent<MetaData::EditorVisible>("Editor Visiblity");
            registerComponent<MetaData::Group>("Group");
        }

		void Controller::onEvent([[maybe_unused]] Event::Event& e) {

		}

        /*****************************************************************//**
        * Entity Methods
        *********************************************************************/

        entt::entity Controller::createEntity() {
            auto entity = entt_registry.create();
            ++entity_count;
            // TODO: When create entity, default add the metadata comps
            return entity;

        }

        entt::entity Controller::cloneEntity(entt::entity copy) {
            if (!checkEntity(copy)) {
                PN_CORE_INFO("Cannot clone invalid entity: {}", entt::to_integral(copy));
                return entt::null;
            }

            entt::entity clone = createEntity();

            // Copy all components from source to clone, EnTT doesn't have built-in cloning, so we iterate through storage pools

            // Iterate all registered component types and copy if present
            for (const auto [type_index, storage] : entt_registry.storage()) {
                if (storage.contains(copy)) {
                    // Component exists on source, copy to clone
                    // Note: This requires components to be copy-constructible
                    storage.push(clone, storage.value(copy));
                }
            }

            return clone;
        }

        void Controller::destroyEntity(entt::entity entity) {
            if (entt_registry.valid(entity)) {
                entt_registry.destroy(entity);
                --entity_count;
            }
        }

        bool Controller::checkEntity(entt::entity entity) const {
            return entt_registry.valid(entity);
        }

        void Controller::destroyAllEntities() {
            entity_count = 0;
            entt_registry.clear();
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
