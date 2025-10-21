#include "pch.h"
#include "Controller.h"

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
            //registerComponent<Camera>("Camera");
            //registerComponent<RigidBody>("RigidBody");

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

        Entity::Type Controller::createEntity() {
            auto entity = static_cast<Entity::Type>(entt_registry.create());
            ++entity_count;
            // TODO: When create entity, default add the metadata comps
            return entity;

        }

        Entity::Type Controller::cloneEntity(Entity::Type copy) {
            if (!checkEntity(copy)) {
                PN_CORE_INFO("Cannot clone invalid entity: {}", copy);
                return Entity::INVALID;
            }

            Entity::Type clone = createEntity();

            // Copy all components from source to clone, EnTT doesn't have built-in cloning, so we iterate through storage pools
            entt::entity src_entity = static_cast<entt::entity>(copy);
            entt::entity clone_entity = static_cast<entt::entity>(clone);

            // Iterate all registered component types and copy if present
            for (const auto [type_index, storage] : entt_registry.storage()) {
                if (storage.contains(src_entity)) {
                    // Component exists on source, copy to clone
                    // Note: This requires components to be copy-constructible
                    storage.push(clone_entity, storage.value(src_entity));
                }
            }

            ++entity_count;
            return clone;
        }

        void Controller::destroyEntity(Entity::Type entity) {
            if (entt_registry.valid(static_cast<entt::entity>(entity))) {
                entt_registry.destroy(static_cast<entt::entity>(entity));
                --entity_count;
            }
        }

        bool Controller::checkEntity(Entity::Type entity) const {
            return entt_registry.valid(static_cast<entt::entity>(entity));
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

        std::vector<std::string> Controller::getEntityComponentNames(Entity::Type entity) const {
            std::vector<std::string> component_names;

            if (!checkEntity(entity)) {
                return component_names;
            }

            auto e = static_cast<entt::entity>(entity);

            // Iterate all registered component checkers
            for (const auto& [name, checker] : component_checkers) {
                if (checker(e)) {
                    component_names.push_back(name);
                }
            }

            return component_names;
        }

        bool Controller::hasComponentByName(Entity::Type entity, const std::string& name) const {
            auto it = component_checkers.find(name);
            if (it == component_checkers.end()) {
                return false;
            }

            return it->second(static_cast<entt::entity>(entity));
        }

        void Controller::removeComponentByName(Entity::Type entity, const std::string& name) {
            auto it = component_removers.find(name);
            if (it != component_removers.end()) {
                it->second(static_cast<entt::entity>(entity));
            }
        }

        void* Controller::getComponentPtrByName(Entity::Type entity, const std::string& name) {
            auto it = component_getters.find(name);
            if (it == component_getters.end()) {
                return nullptr;
            }

            return it->second(static_cast<entt::entity>(entity));
        }


	}
}
