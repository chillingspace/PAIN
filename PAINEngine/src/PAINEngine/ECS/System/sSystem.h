#pragma once

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

// Eh wtf somehow i include "pch.h" causes an error, so i have to include <set> ?? and it fixes?
#include <set>

#include "../ECSTypes.h"
#include "CoreSystems/Events/Event.h"
#include <Applications/AppSystem.h>


namespace PAIN {
	namespace ECS {
		namespace System {
			//Base ECS Class
			class ISystem : public AppSystem {
			protected:
				//System signature
				Component::Signature system_signature;

				//List of entities to update
				std::set<Entity::Type> entities;

				//Linked components will use signature for filtering entities
				//Unlinked components will check if any of the system components
				//are present within entity. If present, entity is added to the entities list
				bool b_components_linked;

				//System active
				bool b_system_active;
			public:

				//Constructor
				ISystem() : system_signature{}, b_components_linked{ true }, b_system_active{ true } {}

				// Get system name
				virtual std::string getSysName() = 0;

				//Event handler for app layer
				virtual void onEvent(Event::Event& e) = 0;

				//Set components linked mode
				void setComponentsLinked(bool state);

				//Get components linked mode
				bool getComponentsLinked() const;

				//Set System active
				void setActiveState(bool state);

				//Get System active
				bool getActiveState() const;

				//Add component type
				void addComponentType(Component::Type component);

				//Remove component type
				void removeComponentType(Component::Type component);

				//Check if component is present
				bool checkComponentType(Component::Type component) const;

				//Get system signautre
				Component::Signature getSignature() const;

				//Add entity
				void addEntity(Entity::Type entity);

				//Remove Entity
				void removeEntity(Entity::Type entity);

				//Check if entity is present
				bool checkEntity(Entity::Type entity) const;

				//Virtual Destructor
				virtual ~ISystem() = default;
			};

			//System Manager
			class Service {
			private:

				// This to track last call time
				std::chrono::steady_clock::time_point last_call_time = std::chrono::steady_clock::now();

				//Delete Copy Constructor & Copy Assignment
				Service(Service const& copy) = delete;
				void operator=(Service const& copy) = delete;

				//Pointer to systems in contiguous array
				std::vector<std::shared_ptr<System::ISystem>> systems;

				//Map to systems for individual access
				std::unordered_map<std::string, std::pair<int, std::shared_ptr<System::ISystem>>> systems_map;

				//Private type casting for easy retrieval
				template<typename T>
				std::shared_ptr<T> getSystem() {
					std::string type_name{ typeid(T).name() };
					return std::static_pointer_cast<T>(systems_map.at(type_name).second);
				}


			public:
				//Default Constructor
				Service() = default;

				//Index for adding system is only if
				template<typename T>
				std::shared_ptr<T> registerSystem(bool components_linked = true, int index = -1) {

					//Get iterator
					auto it = systems_map.find(typeid(T).name());

					//Check if system has already been added
					if (it != systems_map.end()) {
						throw std::runtime_error("System already registered.");
					}

					//System object
					std::shared_ptr<T> system = std::make_shared<T>();

					//Init system
					system->onAttach();

					//Set system components linked
					system->setComponentsLinked(components_linked);

					//Insert system at back
					if (index >= 0) {
						systems.emplace(systems.begin() + index, system);
					}
					else {
						systems.push_back(system);
						index = static_cast<int>(systems.size()) - 1;
					}

					//Emplace shared pointer to system in map
					systems_map.emplace(std::piecewise_construct, std::forward_as_tuple(typeid(T).name()), std::forward_as_tuple(std::move(std::make_pair(index, system))));

					//Return system created
					return system;
				}

				//Index for adding system is only if
				template<typename T>
				void removeSystem() {
					//Get iterator
					auto it = systems_map.find(typeid(T).name());

					//Check if system has already been added
					if (it == systems_map.end()) {
						throw std::runtime_error("System not registered. Removal failed.");
					}

					systems.erase(systems.begin() + it->second.first);
					systems_map.erase(it);
				}

				//Set system state
				template<typename T>
				void setSystemState(bool state) {
					//Get iterator
					auto it = systems_map.find(typeid(T).name());

					//Check if system has already been added
					if (it == systems_map.end()) {
						throw std::runtime_error("System not registered. Setting of state failed");
					}

					//Set system state
					it->second.second->setActiveState(state);
				}

				//Set signature of system
				template<typename T>
				void addComponentType(Component::Type component) {
					//Get iterator
					auto it = systems_map.find(typeid(T).name());

					//Check if system has already been added
					if (it == systems_map.end()) {
						throw std::runtime_error("System not registered. Adding component type failed");
					}

					//Set signature of system
					it->second.second->addComponentType(component);
				}

				//Get System index
				template<typename T>
				int getSystemIndex() {
					//Get iterator
					auto it = systems_map.find(typeid(T).name());

					//Check if system has already been added
					if (it == systems_map.end()) {
						throw std::runtime_error("System not registered. Checking of index failed");
					}

					return it->second.first;
				}

				//Get System name
				template<typename T>
				std::string getSystemName() {
					//Get iterator
					auto it = systems_map.find(typeid(T).name());

					//Check if system has already been added
					if (it == systems_map.end()) {
						throw std::runtime_error("System not registered. Checking of index failed");
					}

					return it->second.second->getSysName();
				}

				//Update entities list based on signature
				void updateEntitiesList(Entity::Type entity, Component::Signature e_signature);

				//Clone entity for all systems
				void cloneEntity(Entity::Type clone, Entity::Type copy);

				//Remove entity from all systems
				void entityDestroyed(Entity::Type entity);

				//Update all systems
				void updateSystems(AppTiming time);

				//Get all systems
				std::vector<std::shared_ptr<System::ISystem>>& getAllSystems();
			};
		}
	}
}

#endif