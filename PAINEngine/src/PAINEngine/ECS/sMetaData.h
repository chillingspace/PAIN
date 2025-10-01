/*****************************************************************//**
 * \file   sMetaData.h
 * \brief  Meta Data Management
 *
 * \author Ho Shu Hng, 2301339, shuhng.ho@digipen.edu (100%)
 * \date   September 2024
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/
#pragma once

#ifndef M_METADATA_HPP
#define M_METADATA_HPP

#include "Controller.h"

namespace PAIN {
	namespace MetaData {
        // Parent-child relationship for entities
        struct Parent {
            std::set<ECS::Entity::Type> children;
            Parent() = default;
        };

        struct Child {
            ECS::Entity::Type parent;
            Child() : parent(0) {}
            explicit Child(ECS::Entity::Type p) : parent(p) {}
        };

        // Group struct for organizing entities
        struct Group {
            std::string name;
            std::optional<std::string> parent_group;
            std::set<std::string> child_groups;
            std::set<ECS::Entity::Type> entities;
            // For editor UI state
            bool expanded; 

            Group() : expanded(true) {}
            explicit Group(std::string const& group_name)
                : name(group_name), expanded(true) {
            }

            nlohmann::json serialize() const;
            void deserialize(nlohmann::json const& data);
        };

        // Core entity metadata
        struct EntityData {
            // Basic identity
            std::string name;

            // Prefab system
            std::string prefab_id;
            nlohmann::json prefab_override;

            // Editor properties
            bool locked;
            bool visible;

            // Layer system
            unsigned int layer_id;
            size_t layer_order;

            // Tagging system
            std::set<std::string> tags;

            // Parent-child hierarchy
            std::variant<Parent, Child> relation;

            // Group assignment
            std::optional<std::string> group_id;

            // Constructors
            EntityData()
                : name("entity_"), prefab_id(""), locked(false),
                visible(true), layer_id(0), layer_order(0),
                relation(Parent()) {
            }

            explicit EntityData(std::string const& n)
                : name(n), prefab_id(""), locked(false),
                visible(true), layer_id(0), layer_order(0),
                relation(Parent()) {
            }

            // Serialization
            nlohmann::json serialize() const;
            void deserialize(nlohmann::json const& data);
        };

        // Metadata Service
        class Service : public AppSystem{
        private:
            // Delete copy operations
            Service(Service const& copy) = delete;
            void operator=(Service const& copy) = delete;

            // Default entity name
            std::string default_entity_name;

            // Entity storage
            struct EntitySorter {
                bool operator()( ECS::Entity::Type const& e1,  ECS::Entity::Type const& e2) const {
                    return e1 < e2;
                }
            };
            std::map< ECS::Entity::Type, EntityData, EntitySorter> entities;

            // Name mapping for quick lookup
            std::unordered_map<std::string,  ECS::Entity::Type> entity_names;

            // Tag registry
            std::set<std::string> registered_tags;


            //Ecs entities
            std::set<ECS::Entity::Type> ecs_entities;

            // Group management
            std::unordered_map<std::string, Group> groups;
            std::unordered_map< ECS::Entity::Type, std::string> entity_to_group;

            //Default entity name
            std::string def_name;

            // Pending operations
            std::set< ECS::Entity::Type> entities_to_destroy;

            // Helper functions
            void updateData();
            std::string generateUniqueName(std::string const& base_name) const;
            void removeEntityFromGroup( ECS::Entity::Type entity);

        public:
            // Constructor
            Service() = default;

            // Initialization
            void init(std::string const& def_entity_name = "entity_");

            // Update
            void onUpdate(AppTiming timing) override;

            // === Entity Management ===

            void onEvent(Event::Event& e) override;

            // Name operations
            bool isNameValid(std::string const& name) const;
            void setEntityName( ECS::Entity::Type entity, std::string const& name);
            std::string getEntityName( ECS::Entity::Type entity) const;
            std::optional< ECS::Entity::Type> getEntityByName(std::string const& name) const;

            // Visibility
            void setEntityVisible( ECS::Entity::Type entity, bool visible);
            bool isEntityVisible( ECS::Entity::Type entity) const;

            // Locking
            void setEntityLocked( ECS::Entity::Type entity, bool locked);
            bool isEntityLocked( ECS::Entity::Type entity) const;
            void setAllEntitiesLocked(bool locked);

            // Destruction
            void destroyEntity( ECS::Entity::Type entity);
            void destroyAllEntities();

            // === Prefab System ===

            void setEntityPrefabID( ECS::Entity::Type entity, std::string const& prefab_id);
            std::string getEntityPrefabID( ECS::Entity::Type entity) const;
            void setEntityPrefabOverride( ECS::Entity::Type entity, nlohmann::json const& data);
            nlohmann::json getEntityPrefabOverride( ECS::Entity::Type entity) const;

            // === Tag System ===

            void registerTag(std::string const& tag);
            void unregisterTag(std::string const& tag);
            bool isTagValid(std::string const& tag) const;
            std::set<std::string> const& getRegisteredTags() const;

            void addEntityTag( ECS::Entity::Type entity, std::string const& tag);
            void removeEntityTag( ECS::Entity::Type entity, std::string const& tag);
            std::set<std::string> getEntityTags( ECS::Entity::Type entity) const;
            bool hasEntityTag( ECS::Entity::Type entity, std::string const& tag) const;
            std::set< ECS::Entity::Type> getEntitiesByTag(std::string const& tag) const;

            // === Layer System ===

            void setEntityLayerID( ECS::Entity::Type entity, unsigned int layer_id);
            unsigned int getEntityLayerID( ECS::Entity::Type entity) const;
            void setEntityLayerOrder( ECS::Entity::Type entity, size_t order);
            size_t getEntityLayerOrder( ECS::Entity::Type entity) const;

            // === Parent-Child Hierarchy ===

            void setEntityAsParent( ECS::Entity::Type entity);
            void setEntityAsChild( ECS::Entity::Type entity,  ECS::Entity::Type parent);
            void detachEntityFromParent( ECS::Entity::Type entity);
            bool isParent( ECS::Entity::Type entity) const;
            bool isChild( ECS::Entity::Type entity) const;
            std::optional< ECS::Entity::Type> getEntityParent( ECS::Entity::Type entity) const;
            std::set< ECS::Entity::Type> getEntityChildren( ECS::Entity::Type entity) const;
            std::variant<Parent, Child> getEntityRelation( ECS::Entity::Type entity) const;

            // === Group/Folder System ===

            // Group creation and deletion
            bool createGroup(std::string const& group_name,
                std::optional<std::string> parent_group = std::nullopt);
            bool deleteGroup(std::string const& group_name, bool remove_entities = false);
            bool renameGroup(std::string const& old_name, std::string const& new_name);
            bool groupExists(std::string const& group_name) const;

            // Group hierarchy
            bool setGroupParent(std::string const& group_name,
                std::optional<std::string> parent_group);
            std::optional<std::string> getGroupParent(std::string const& group_name) const;
            std::set<std::string> getGroupChildren(std::string const& group_name) const;
            std::set<std::string> getAllGroups() const;
            std::set<std::string> getRootGroups() const;

            // Entity-group assignment
            void assignEntityToGroup( ECS::Entity::Type entity, std::string const& group_name);
            void unassignEntityFromGroup( ECS::Entity::Type entity);
            std::optional<std::string> getEntityGroup( ECS::Entity::Type entity) const;

            // Query entities by group
            std::set< ECS::Entity::Type> getEntitiesInGroup(std::string const& group_name,
                bool recursive = false) const;

            // Group UI state
            void setGroupExpanded(std::string const& group_name, bool expanded);
            bool isGroupExpanded(std::string const& group_name) const;

            // === Data Access ===

            std::optional<EntityData> getEntityDataCopy( ECS::Entity::Type entity) const;
            EntityData* getEntityDataPtr( ECS::Entity::Type entity);
            std::map< ECS::Entity::Type, EntityData, EntitySorter> const& getEntitiesData() const;
             ECS::Entity::Type getFirstEntity() const;
            size_t getEntityCount() const;

            // === Cloning ===

            void cloneEntityData( ECS::Entity::Type source,  ECS::Entity::Type target);

            // === Serialization ===

            nlohmann::json serializeEntityData( ECS::Entity::Type entity) const;
            void deserializeEntityData( ECS::Entity::Type entity, nlohmann::json const& data);
            nlohmann::json serializeGroups() const;
            void deserializeGroups(nlohmann::json const& data);
            nlohmann::json serialize() const;
            void deserialize(nlohmann::json const& data);

            // === Reset ===

            void reset();
        };
	}
}

#endif