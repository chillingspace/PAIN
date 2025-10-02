#include "pch.h"
#include "sSystem.h"

namespace PAIN {
    namespace ECS {
        namespace System {

            /*****************************************************************//**
            * ISystem
            *********************************************************************/
            void ISystem::setComponentsLinked(bool state) {
                b_components_linked = state;
            }

            bool ISystem::getComponentsLinked() const {
                return b_components_linked;
            }

            void ISystem::setActiveState(bool state) {
                b_system_active = state;
            }

            bool ISystem::getActiveState() const {
                return b_system_active;
            }

            void ISystem::addComponentType(Component::Type component) {
                system_signature.set(component, true);
            }

            void ISystem::removeComponentType(Component::Type component) {
                system_signature.set(component, false);
            }

            bool ISystem::checkComponentType(Component::Type component) const {
                return system_signature.test(component);
            }

            Component::Signature ISystem::getSignature() const {
                return system_signature;
            }

            void ISystem::addEntity(Entity::Type entity) {
                // std::set::insert automatically handles duplicates
                entities.insert(entity);
            }

            void ISystem::removeEntity(Entity::Type entity) {
                // std::set::erase is safe even if entity doesn't exist
                if (entities.find(entity) == entities.end()) {
                    return;
                }

                entities.erase(entity);

                // TODO: Uncomment when metadata service is ready
                // Handle child entity removal recursively
            }

            bool ISystem::checkEntity(Entity::Type entity) const {
                return entities.find(entity) != entities.end();
            }

            /*****************************************************************//**
            * System Manager
            *********************************************************************/

            void Service::updateEntitiesList(Entity::Type entity, Component::Signature e_signature) {
                for (auto& system : systems) {
                    // Check if system uses linked components (ALL must match)
                    if (system->getComponentsLinked()) {
                        // Entity must have ALL required components
                        if ((system->getSignature() & e_signature) == system->getSignature()) {
                            system->addEntity(entity);
                        }
                        else {
                            system->removeEntity(entity);
                        }
                    }
                    else {
                        // Entity needs ANY of the components
                        if ((system->getSignature() & e_signature).any()) {
                            system->addEntity(entity);
                        }
                        else {
                            system->removeEntity(entity);
                        }
                    }
                }
            }

            void Service::cloneEntity(Entity::Type clone, Entity::Type copy) {
                for (auto& system : systems) {
                    if (system->checkEntity(copy)) {
                        system->addEntity(clone);
                    }
                }
            }

            void Service::entityDestroyed(Entity::Type entity) {
                // Remove entity from all systems
                // No need to check - removeEntity handles non-existent entities safely
                for (auto& system : systems) {
                    system->removeEntity(entity);
                }
            }

            void Service::updateSystems() {
                // Pre-allocate vector for performance
                std::vector<double> system_times;
                system_times.reserve(systems.size());

                // Update all active systems
                for (auto const& system : systems) {
                    if (system->getActiveState()) {
                        auto system_start_time = std::chrono::steady_clock::now();

                        // Update the system
                        system->onUpdate();  // Match your header declaration

                        auto system_end_time = std::chrono::steady_clock::now();
                        std::chrono::duration<double, std::milli> system_duration =
                            system_end_time - system_start_time;
                        system_times.push_back(system_duration.count());
                    }
                    else {
                        // Inactive system - zero runtime
                        system_times.push_back(0.0);
                    }
                }

                // Profile system performance every 1 second (1000ms)
                auto current_time = std::chrono::steady_clock::now();
                auto time_since_last_call =
                    std::chrono::duration<double, std::milli>(current_time - last_call_time).count();

                if (time_since_last_call >= 1000.0) {
                    // TODO: Replace with your debug/profiling service
                    // PAIN_DEBUG_SERVICE->updateSystemPercentage(system_times, systems);

                    last_call_time = current_time;
                }
            }

            std::vector<std::shared_ptr<System::ISystem>>& Service::getAllSystems() {
                return systems;
            }
        }
    }
}

