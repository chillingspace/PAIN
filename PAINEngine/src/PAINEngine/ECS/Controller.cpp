#include "pch.h"
#include "Controller.h"

namespace PAIN {
	namespace ECS {
		Controller::Controller() : entity_service{ std::make_unique<Entity::Service>() }, component_service{ std::make_unique<Component::Service>() },
			system_service{ std::make_unique<System::Service>() }
		{
		}

		void Controller::dispatchToLayers(Event::Event& e) {
            auto& systems = system_service->getAllSystems();
			for (auto it = systems.begin(); it != systems.end(); ++it) {

				//Dispatch event down layers
				(*it)->onEvent(e);
				if (e.checkHandled()) break;
			}
		}

		void Controller::dispatchToLayersReversed(Event::Event& e) {
            auto& systems = system_service->getAllSystems();
			if (systems.empty()) return;
			for (auto it = systems.rbegin(); it != systems.rend(); ++it) {

				//Dispatch event down layers
				(*it)->onEvent(e);
				if (e.checkHandled()) break;
			}
		}

		void Controller::onUpdate(AppTiming timing) {
			//Iterate through all systems
            system_service->updateSystems(timing);
		}

		void Controller::onEvent([[maybe_unused]] Event::Event& e) {

		}

        /*****************************************************************//**
        * Entity Methods
        *********************************************************************/

        Entity::Type Controller::createEntity() {
            return entity_service->createEntity();
            // Event dispatching should be done by caller if needed
        }

        Entity::Type Controller::cloneEntity(Entity::Type copy) {
            // Validate source entity exists
            if (!entity_service->checkEntity(copy)) {
                PN_CORE_WARN("Cannot clone non-existent entity");
            }

            // Create new entity
            Entity::Type new_entity = entity_service->createEntity();

            // Copy signature first
            entity_service->setSignature(
                new_entity,
                entity_service->getSignature(copy)
            );

            // Clone components
            component_service->cloneEntity(new_entity, copy);

            // Add to relevant systems
            system_service->cloneEntity(new_entity, copy);

            return new_entity;
        }

        void Controller::destroyEntity(Entity::Type entity) {
            // Validate entity exists
            if (!entity_service->checkEntity(entity)) {
                return;  // Already destroyed
            }

            // Destroy in order: systems -> components -> entity
            system_service->entityDestroyed(entity);
            component_service->entityDestroyed(entity);
            entity_service->destroyEntity(entity);
        }

        bool Controller::checkEntity(Entity::Type entity) const {
            return entity_service->checkEntity(entity);
        }

        size_t Controller::getEntityComponentCount(Entity::Type entity) const {
            return entity_service->getSignature(entity).count();
        }

        void Controller::destroyAllEntities() {
            // Get copy of entities to avoid iterator invalidation
            auto entities = entity_service->getAllEntities();

            // Destroy all without individual events
            for (auto entity : entities) {
                system_service->entityDestroyed(entity);
                component_service->entityDestroyed(entity);
                entity_service->destroyEntity(entity);
            }
        }

        int Controller::getEntitiesCount() const {
            return entity_service->getEntitiesCount();
        }

        const std::set<Entity::Type> Controller::getAllEntities() const {
            return entity_service->getAllEntities();
        }

        std::vector<std::shared_ptr<System::ISystem>>& Controller::getAllSystems() {
            return system_service->getAllSystems();
        }

        /*****************************************************************//**
        * Component Methods
        *********************************************************************/

        void Controller::addDefEntityComponent(Entity::Type entity, Component::Type type) {
            // Validate entity exists
            if (!entity_service->checkEntity(entity)) {
                throw std::runtime_error("Cannot add component to non-existent entity");
            }

            // Add component
            component_service->addDefEntityComponent(entity, type);

            // Update entity signature
            Component::Signature sign = entity_service->getSignature(entity);
            sign.set(type, true);
            entity_service->setSignature(entity, sign);

            // Update systems
            system_service->updateEntitiesList(entity, sign);
        }

        void Controller::removeEntityComponent(Entity::Type entity, Component::Type type) {
            // Validate entity and component exist
            if (!entity_service->checkEntity(entity)) {
                return;  // Entity doesn't exist, nothing to remove
            }

            // Remove component
            component_service->removeEntityComponent(entity, type);

            // Update entity signature
            Component::Signature sign = entity_service->getSignature(entity);
            sign.set(type, false);
            entity_service->setSignature(entity, sign);

            // Update systems
            system_service->updateEntitiesList(entity, sign);
        }

        std::shared_ptr<void> Controller::getEntityComponent(Entity::Type entity, Component::Type type) {
            return component_service->getEntityComponent(entity, type);
        }

        std::shared_ptr<void> Controller::getCopiedEntityComponent(Entity::Type entity, Component::Type type) {
            return component_service->getCopiedEntityComponent(entity, type);
        }

        void Controller::setEntityComponent(Entity::Type entity, Component::Type type, std::shared_ptr<void> comp) {
            component_service->setEntityComponent(entity, type, comp);
        }

        bool Controller::checkComponentType(std::string const& type) const {
            return component_service->checkComponentType(type);
        }

        size_t Controller::getComponentEntitiesCount(Component::Type comp_type) const {
            return component_service->getComponentEntitiesCount(comp_type).value_or(0);
        }

        const std::set<Entity::Type> Controller::getAllComponentEntities(Component::Type comp_type) const {
            return component_service->getAllComponentEntities(comp_type);
        }

        const std::unordered_map<std::string, std::shared_ptr<void>>
            Controller::getAllEntityComponents(Entity::Type entity) const {
            return component_service->getAllEntityComponents(entity);
        }

        const std::unordered_map<std::string, std::shared_ptr<void>>
            Controller::getAllCopiedEntityComponents(Entity::Type entity) const {
            return component_service->getAllCopiedEntityComponents(entity);
        }

        const std::unordered_map<std::string, Component::Type>
            Controller::getAllComponentTypes() const {
            return component_service->getAllComponentTypes();
        }

        size_t Controller::getComponentsCount() const {
            return component_service->getComponentsCount();
        }

	}
}
