/*****************************************************************//**
 * \file   ComponentService.cpp
 * \brief  Component manager for ECS architecture
 *
 * \author Your Name
 * \date   October 2025
 *********************************************************************/

#include "pch.h"
#include "sComponents.h"

namespace PAIN {
    namespace ECS {
        namespace Component {

            /*****************************************************************//**
            * Helper: Get component string from type (refactored for reuse)
            *********************************************************************/
            namespace {
                std::optional<std::string> getComponentStringFromType(
                    Component::Type type,
                    std::unordered_map<std::string, Component::Type> const& component_types)
                {
                    for (auto const& comp_type : component_types) {
                        if (comp_type.second == type) {
                            return comp_type.first;
                        }
                    }
                    return std::nullopt;
                }
            }

            /*****************************************************************//**
            * Component Service Implementation
            *********************************************************************/

            bool Service::addDefEntityComponent(Entity::Type entity, Component::Type type) {
                auto component_string = getComponentStringFromType(type, component_types);

                if (!component_string.has_value()) {
                    return false;  // Component type not registered
                }

                auto it = component_arrays.find(component_string.value());
                if (it == component_arrays.end()) {
                    return false;  // Array not found
                }

                it->second->createDefEntityComponent(entity);
                return true;
            }

            bool Service::removeEntityComponent(Entity::Type entity, Component::Type type) {
                auto component_string = getComponentStringFromType(type, component_types);

                if (!component_string.has_value()) {
                    return false;  // Component type not registered
                }

                auto it = component_arrays.find(component_string.value());
                if (it == component_arrays.end()) {
                    return false;  // Array not found
                }

                it->second->removeComponent(entity);
                return true;
            }

            std::shared_ptr<void> Service::getEntityComponent(Entity::Type entity, Component::Type type) {
                auto component_string = getComponentStringFromType(type, component_types);

                if (!component_string.has_value()) {
                    return nullptr;  // Component type not registered
                }

                auto it = component_arrays.find(component_string.value());
                if (it == component_arrays.end()) {
                    return nullptr;  // Array not found
                }

                return it->second->getEntityComponent(entity);
            }

            std::shared_ptr<void> Service::getCopiedEntityComponent(Entity::Type entity, Component::Type type) {
                auto component_string = getComponentStringFromType(type, component_types);

                if (!component_string.has_value()) {
                    return nullptr;  // Component type not registered
                }

                auto it = component_arrays.find(component_string.value());
                if (it == component_arrays.end()) {
                    return nullptr;  // Array not found
                }

                return it->second->getCopiedEntityComponent(entity);
            }

            bool Service::setEntityComponent(Entity::Type entity, Component::Type type, std::shared_ptr<void> comp) {
                if (!comp) {
                    return false;  // Invalid component pointer
                }

                auto component_string = getComponentStringFromType(type, component_types);

                if (!component_string.has_value()) {
                    return false;  // Component type not registered
                }

                auto it = component_arrays.find(component_string.value());
                if (it == component_arrays.end()) {
                    return false;  // Array not found
                }

                it->second->setEntityComponent(entity, comp);
                return true;
            }

            std::optional<Component::Type> Service::getComponentType(std::string const& type) const {
                auto it = component_types.find(type);
                if (it == component_types.end()) {
                    return std::nullopt;  // Component not registered
                }

                return it->second;
            }

            bool Service::checkComponentType(std::string const& type) const {
                return component_types.find(type) != component_types.end();
            }

            std::optional<size_t> Service::getComponentEntitiesCount(Component::Type type) {
                auto component_string = getComponentStringFromType(type, component_types);

                if (!component_string.has_value()) {
                    return std::nullopt;  // Component type not registered
                }

                auto it = component_arrays.find(component_string.value());
                if (it == component_arrays.end()) {
                    return std::nullopt;  // Array not found
                }

                return it->second->getComponentEntitiesCount();
            }

            std::set<Entity::Type> Service::getAllComponentEntities(Component::Type type) {
                auto component_string = getComponentStringFromType(type, component_types);

                if (!component_string.has_value()) {
                    return {};  // Return empty set if not registered
                }

                auto it = component_arrays.find(component_string.value());
                if (it == component_arrays.end()) {
                    return {};  // Return empty set if array not found
                }

                return it->second->getComponentEntities();
            }

            void Service::cloneEntity(Entity::Type clone, Entity::Type copy) {
                // Clone all components from copy entity to clone entity
                for (auto& c_array : component_arrays) {
                    c_array.second->cloneEntity(clone, copy);
                }
            }

            void Service::entityDestroyed(Entity::Type entity) {
                // Remove entity from all component arrays
                for (auto& c_array : component_arrays) {
                    c_array.second->entityDestroyed(entity);
                }
            }

            std::unordered_map<std::string, std::shared_ptr<void>> Service::getAllEntityComponents(Entity::Type entity) const {
                std::unordered_map<std::string, std::shared_ptr<void>> comp_map;

                // Get all components for this entity
                for (auto const& comp : component_arrays) {
                    if (comp.second->checkEntity(entity)) {
                        auto component = comp.second->getEntityComponent(entity);
                        if (component) {
                            comp_map.emplace(comp.first, component);
                        }
                    }
                }

                return comp_map;
            }

            std::unordered_map<std::string, std::shared_ptr<void>> Service::getAllCopiedEntityComponents(Entity::Type entity) const {
                std::unordered_map<std::string, std::shared_ptr<void>> comp_map;

                // Get copies of all components for this entity
                for (auto const& comp : component_arrays) {
                    if (comp.second->checkEntity(entity)) {
                        auto component = comp.second->getCopiedEntityComponent(entity);
                        if (component) {
                            comp_map.emplace(comp.first, component);
                        }
                    }
                }

                return comp_map;
            }

            std::unordered_map<std::string, Component::Type> const& Service::getAllComponentTypes() const {
                return component_types;
            }

            size_t Service::getComponentsCount() const {
                return component_types.size();
            }

            bool Service::hasEntityComponent(Entity::Type entity, Component::Type type) const {
                auto component_string = getComponentStringFromType(type, component_types);

                if (!component_string.has_value()) {
                    return false;
                }

                auto it = component_arrays.find(component_string.value());
                if (it == component_arrays.end()) {
                    return false;
                }

                return it->second->checkEntity(entity);
            }

        } // namespace Component
    } // namespace ECS
} // namespace PAIN
