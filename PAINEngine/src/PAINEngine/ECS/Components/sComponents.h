

#include "../ECSTypes.h"
#include "Utility/ECSUtility.h"

namespace PAIN {
	namespace ECS {
		namespace Component {
			//Component Array interface
			class IArray {
			private:
			public:
				virtual ~IArray() = default;

				//Check entity comp
				virtual bool checkEntity(Entity::Type entity) = 0;

				//Get count of entity with current comp
				virtual size_t getComponentEntitiesCount() = 0;

				//Get all entity with current comp
				virtual std::set<Entity::Type> getComponentEntities() = 0;

				//Get entity comp
				virtual std::shared_ptr<void> getEntityComponent(Entity::Type entity) = 0;

				//Get entity comp deep copy
				virtual std::shared_ptr<void> getCopiedEntityComponent(Entity::Type entity) = 0;

				//Set entity comp
				virtual void setEntityComponent(Entity::Type entity, std::shared_ptr<void> comp) = 0;

				//Clone entity
				virtual void cloneEntity(Entity::Type clone, Entity::Type copy) = 0;

				//Create default entity component
				virtual void createDefEntityComponent(Entity::Type entity) = 0;

				//Remove destroyed entity
				virtual void entityDestroyed(Entity::Type entity) = 0;

				// Remove component
				virtual void removeComponent(Entity::Type entity) = 0;
			};


			//Component Array ( Entity Map of unique components of type T )
			template<typename T>
			class Array : public IArray {
			private:

				//Unique component of same type identified by Entity type
				std::unordered_map<Entity::Type, std::shared_ptr<T>> component_array;

			public:
				//Default constructor
				Array() = default;

				//Add new component
				void addComponent(Entity::Type entity, T&& component) {
					// Only add if doesn't exist (won't throw)
					component_array.try_emplace(entity, std::make_shared<T>(std::move(component)));
				}

				//Remove existing component
				void removeComponent(Entity::Type entity) override {
					// erase() returns 0 if not found, 1 if erased - doesn't throw
					//Remove entitys
					component_array.erase(entity);
				}

				//Get entity component data
				std::optional<std::reference_wrapper<T>> getComponent(Entity::Type entity) {
					auto it = component_array.find(entity);
					if (it == component_array.end()) {
						return std::nullopt;
					}
					return std::ref(*it->second);
				}

				//Check entity component
				bool checkEntity(Entity::Type entity) override {
					return component_array.find(entity) != component_array.end();
				}

				size_t getComponentEntitiesCount() override {
					return component_array.size();
				}

				//Get all entities with current component
				std::set<Entity::Type> getComponentEntities() override {
					std::set<Entity::Type> entities_set;
					for (const auto& entity_comp : component_array) {
						entities_set.insert(entity_comp.first);
					}
					return entities_set;
				}

				//get entity component
				std::shared_ptr<void> getEntityComponent(Entity::Type entity) override {
					auto it = component_array.find(entity);
					if (it == component_array.end()) {
						return nullptr;
					}
					// Return the shared_ptr (type-erased to void)
					return it->second;
				}

				//Get copied entity component
				std::shared_ptr<void> getCopiedEntityComponent(Entity::Type entity) override {
					auto it = component_array.find(entity);
					if (it == component_array.end()) {
						return nullptr;
					}
					// Create a NEW copy for this method
					return std::make_shared<T>(*it->second);
				}

				//Set entity
				void setEntityComponent(Entity::Type entity, std::shared_ptr<void> comp) override {
					if (comp) {
						component_array[entity] = std::static_pointer_cast<T>(comp);
					}
				}

				//Clone entity
				void cloneEntity(Entity::Type clone, Entity::Type copy) override {
					auto it = component_array.find(copy);
					if (it != component_array.end()) {
						// Create a copy of the component for the clone
						component_array[clone] = std::make_shared<T>(*it->second);
					}
				}

				//Create default entity component
				// OPTIMIZED: Use try_emplace
				void createDefEntityComponent(Entity::Type entity) override {
					// Creates default T() if doesn't exist
					component_array.try_emplace(entity, std::make_shared<T>());
				}

				//Remove destroyed entity
				// OPTIMIZED: Remove redundant check
				void entityDestroyed(Entity::Type entity) override {
					// Safe even if doesn't exist
					removeComponent(entity);
				}
			};

			//Manager of the component Array
			class Service {
			private:

				//Delete Copy Constructor & Copy Assignment
				Service(Service const& copy) = delete;
				void operator=(Service const& copy) = delete;

				//Map to component type ( used for signature setting )
				std::unordered_map<std::string, Component::Type> component_types;

				//Map to array of component type
				std::unordered_map<std::string, std::shared_ptr<IArray>> component_arrays;

				//Private type casting for easy retrieval
				template<typename T>
				std::shared_ptr<Array<T>> getComponentArray() {
					std::string type_name = Utility::convertTypeString(typeid(T).name());

					auto it = component_arrays.find(type_name);
					if (it == component_arrays.end()) {
						// Component type not registered
						return nullptr;  
					}

					return std::static_pointer_cast<Array<T>>(it->second);
				}

				//Component id
				Component::Type component_id;

			public:

				//Default Constructor
				Service() : component_id{ 0 } {}

				//Register component with manager
				// IMPROVED: Idempotent - doesn't throw if already registered
				template<typename T>
				bool registerComponent() {

					std::string type_name = Utility::convertTypeString(typeid(T).name());

					// Already registered - return false but don't throw
					if (component_types.find(type_name) != component_types.end()) {
						return false;
					}

					// Register new component type
					component_types[type_name] = component_id++;
					component_arrays[type_name] = std::make_shared<Array<T>>();

					return true;
				}

				// IMPROVED: Renamed and made safe
				template<typename T>
				bool unregisterComponent() {
					std::string type_name = Utility::convertTypeString(typeid(T).name());

					// Not registered - return false but don't throw
					if (component_types.find(type_name) == component_types.end()) {
						return false;
					}

					component_types.erase(type_name);
					component_arrays.erase(type_name);

					return true;
				}

				// IMPROVED: Check if component type is registered
				template<typename T>
				bool isComponentRegistered() const {
					std::string type_name = Utility::convertTypeString(typeid(T).name());
					return component_types.find(type_name) != component_types.end();
				}

				//Add component associated with entity type
				// IMPROVED: Safe add with check
				template<typename T>
				bool addEntityComponent(Entity::Type entity, T&& component) {
					auto array = getComponentArray<T>();
					if (!array) {
						// Component type not registered
						return false;  
					}

					array->addComponent(entity, std::move(component));
					return true;
				}

				//Add default entity component for entity
				bool addDefEntityComponent(Entity::Type entity, Component::Type type);

				//Remove component associated with entity type
				// IMPROVED: Safe remove
				template<typename T>
				bool removeEntityComponent(Entity::Type entity) {
					auto array = getComponentArray<T>();
					if (!array) {
						// Component type not registered
						return false;  
					}

					array->removeComponent(entity);
					return true;
				}

				bool removeEntityComponent(Entity::Type entity, Component::Type type);

				// IMPROVED: Safe get with proper optional handling
				template<typename T>
				std::optional<std::reference_wrapper<T>> getEntityComponent(Entity::Type entity) {
					auto array = getComponentArray<T>();
					if (!array) {
						// Component type not registered
						return std::nullopt;  
					}

					return array->getComponent(entity);
				}

				//Retrieve a void* to component based on component type
				std::shared_ptr<void> getEntityComponent(Entity::Type entity, Component::Type type);

				//Retrieve a copied void* to component based on component type
				std::shared_ptr<void> getCopiedEntityComponent(Entity::Type entity, Component::Type type);

				//Set entity component based on void* and comp type
				bool setEntityComponent(Entity::Type entity, Component::Type type, std::shared_ptr<void> comp);

				//Get Component Type string overload
				std::optional<Component::Type> getComponentType(std::string const& type) const;

				// IMPROVED: Return optional instead of throwing
				template<typename T>
				std::optional<Component::Type> getComponentType() {
					std::string type_name = Utility::convertTypeString(typeid(T).name());

					auto it = component_types.find(type_name);
					if (it == component_types.end()) {
						// Not registered
						return std::nullopt;  
					}

					return it->second;
				}

				//Check component exists
				bool checkComponentType(std::string const& type) const;

				// Check whether a specific entity has a particular component type attached to it
				bool hasEntityComponent(Entity::Type entity, Component::Type type) const;

				//Get count of entities with that component
				std::optional<size_t> getComponentEntitiesCount(Component::Type comp_type);

				//Get all entities with that component
				std::set<Entity::Type> getAllComponentEntities(Component::Type comp_type);

				//Clone entity
				void cloneEntity(Entity::Type clone, Entity::Type copy);

				//Remove entity from all components
				void entityDestroyed(Entity::Type entity);

				//Get all entity components
				std::unordered_map<std::string, std::shared_ptr<void>> getAllEntityComponents(Entity::Type entity) const;

				//Get all copied entity components
				std::unordered_map<std::string, std::shared_ptr<void>> getAllCopiedEntityComponents(Entity::Type entity) const;

				//Get all component types
				std::unordered_map<std::string, Component::Type> const& getAllComponentTypes() const;

				//Get components count
				size_t getComponentsCount() const;
			};
		}
	}
}