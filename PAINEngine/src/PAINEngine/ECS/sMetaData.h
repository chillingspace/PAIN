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

namespace PAIN {
	namespace MetaData {
        // Parent-child relationship for entities
        struct Parent {
            std::set<std::string> childrens;
            Parent() = default;
        };

        struct Child {
            std::string parent;

            Child() : parent{ "" } {}

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

            // In Group struct implementation
            nlohmann::json serialize() const {
                nlohmann::json data;
                data["name"] = name;
                data["parent_group"] = parent_group.has_value() ? parent_group.value() : "";
                data["child_groups"] = child_groups;
                data["entities"] = entities;
                data["expanded"] = expanded;
                return data;
            }

            void deserialize(nlohmann::json const& data) {
                name = data.value("name", "");

                std::string parent = data.value("parent_group", "");
                parent_group = parent.empty() ? std::nullopt : std::optional<std::string>(parent);

                if (data.contains("child_groups")) {
                    child_groups = data["child_groups"].get<std::set<std::string>>();
                }

                if (data.contains("entities")) {
                    entities = data["entities"].get<std::set<ECS::Entity::Type>>();
                }

                expanded = data.value("expanded", true);
            }
        };

        // Core entity metadata
        struct EntityData {
            // Basic identity
            std::string name;

            // Prefab system
            std::string prefab_id;
            nlohmann::json prefab_override;

            // Editor properties
            //Boolean for locking entities in editor
            bool b_locked;
            bool b_visible;

            // Layer system
            unsigned int layer_id;
            size_t layer_order;

            // Tagging system
            std::set<std::string> tags;

            // Parent-child hierarchy
            std::optional<std::variant<Parent, Child>> relation;

            // Group assignment
            std::optional<std::string> group_id;

            // Constructors
            EntityData()
                : name("entity_"), prefab_id(""), b_locked(false),
                b_visible(true), layer_id(0), layer_order(0),
                relation(Parent()) {
            }

            explicit EntityData(std::string const& n)
                : name(n), prefab_id(""), b_locked(false),
                b_visible(true), layer_id(0), layer_order(0),
                relation(Parent()) {
            }

            // Serialization
            nlohmann::json serialize() const {
                nlohmann::json data;
                data["Name"] = name;
                data["Prefab_ID"] = prefab_id;
                data["Prefab_Override"] = prefab_override;
                data["B_Locked"] = b_locked;
                data["B_Visible"] = b_visible;
                data["Layer_ID"] = layer_id;
                data["Layer_Order"] = layer_order;
                data["Tags"] = tags;

                // Serialize relation
                if (relation.has_value()) {
                    if (auto* child = std::get_if<Child>(&relation.value())) {
                        data["Parent"] = child->parent;
                    }
                }

                return data;
            }

            void deserialize(nlohmann::json const& data) {
                name = data.value("Name", "entity_");
                prefab_id = data.value("Prefab_ID", "");
                prefab_override = data.value("Prefab_Override", nlohmann::json());
                b_locked = data.value("B_Locked", false);
                b_visible = data.value("B_Visible", true);
                layer_id = data.value("Layer_ID", static_cast<unsigned int>(0));
                layer_order = data.value("Layer_Order", static_cast<size_t>(0));

                // Deserialize tags
                if (data.contains("Tags") && data["Tags"].is_array()) {
                    tags = data["Tags"].get<std::set<std::string>>();
                }

                // Deserialize relation
                if (data.contains("Parent")) {
                    Child temp_child;
                    temp_child.parent = data["Parent"].get<std::string>();
                    relation = temp_child;
                }
                else {
                    relation = Parent();
                }
            }

        };

        // Metadata Service
        class Service : public AppSystem{
        private:
            // Delete copy operations
            Service(Service const& copy) = delete;
            void operator=(Service const& copy) = delete;

            // Entity storage
            struct EntitySorter {
                bool operator()( ECS::Entity::Type const& e1,  ECS::Entity::Type const& e2) const {
                    return e1 < e2;
                }
            };
            std::map< ECS::Entity::Type, EntityData, EntitySorter> entities;

            // Name mapping for quick lookup
            std::unordered_map<std::string,  ECS::Entity::Type> entity_names;

            //Ecs entities
            std::set<ECS::Entity::Type> ecs_entities;

            //Entity Types
            std::set<std::string> entity_tags;

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
            void onAttach() override; 

            // Update
            void onUpdate(AppTiming timing) override;

            void onDetach() override;
            void onFixedUpdate(AppTiming timing) override {}
            void onAppPause() override;
            void onAppResume() override;

            // === Entity Management ===

            void onEvent(Event::Event& e) override;

            // Name operations
            bool isNameValid(std::string const& name) const;
            void setEntityName( ECS::Entity::Type entity, std::string const& name);
            std::string getEntityName( ECS::Entity::Type entity) const;
            std::optional< ECS::Entity::Type> getEntityByName(std::string const& name) const;

            // Visibility
            void setEntityVisible(ECS::Entity::Type entity, bool visible);
            bool isEntityVisible(ECS::Entity::Type entity) const;

            // Locking
            void setEntityLocked( ECS::Entity::Type entity, bool locked);

            bool isEntityLocked( ECS::Entity::Type entity) const;
            void setAllEntitiesLocked(bool locked);

            // Destruction
            void destroyEntity( ECS::Entity::Type entity);
            void destroyAllEntities();

            // === Prefab System ===

            //void setEntityPrefabID( ECS::Entity::Type entity, std::string const& prefab_id);
            //std::string getEntityPrefabID( ECS::Entity::Type entity) const;
            //void setEntityPrefabOverride( ECS::Entity::Type entity, nlohmann::json const& data);
            //nlohmann::json getEntityPrefabOverride( ECS::Entity::Type entity) const;

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

            //void setEntityLayerID( ECS::Entity::Type entity, unsigned int layer_id);
            //unsigned int getEntityLayerID( ECS::Entity::Type entity) const;
            //void setEntityLayerOrder( ECS::Entity::Type entity, size_t order);
            //size_t getEntityLayerOrder( ECS::Entity::Type entity) const;

            // === Parent-Child Hierarchy ===

            void setEntityAsParent( ECS::Entity::Type entity);
            void setEntityAsChild(ECS::Entity::Type entity, ECS::Entity::Type parent);
            void detachEntityFromParent( ECS::Entity::Type entity);
            bool isParent( ECS::Entity::Type entity) const;

            //Check if parent is valid
            bool isParent(std::string const& parent_name) const;
            bool isChild( ECS::Entity::Type entity) const;
            std::optional< ECS::Entity::Type> getEntityParent( ECS::Entity::Type entity) const;
            std::set< ECS::Entity::Type> getEntityChildren( ECS::Entity::Type entity) const;
            std::optional<std::variant<Parent, Child>> getEntityRelation( ECS::Entity::Type entity) const;

            void updateRelation();

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