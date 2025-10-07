/*****************************************************************//**
 * \file   sMetaData.h
 * \brief  Lightweight Metadata Query Service
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \date   October 2024
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/
#pragma once

#ifndef META_SERVICE_H
#define META_SERVICE_H

#include "Applications/AppSystem.h"

namespace PAIN {
    namespace MetaData {

        // Group data structure (not a component)
        struct GroupData {
            std::string name;
            std::optional<std::string> parent_group;
            std::set<std::string> child_groups;
            std::set<ECS::Entity::Type> entities;
            bool expanded;

            GroupData() : expanded(true) {}
            explicit GroupData(std::string const& group_name)
                : name(group_name), expanded(true) {
            }
        };

        class Service : public AppSystem {
        private:
            // Delete copy operations
            Service(Service const& copy) = delete;
            void operator=(Service const& copy) = delete;

            // Centralized registries only
            std::set<std::string> registered_tags;
            std::unordered_map<std::string, GroupData> groups;

            // Name uniqueness tracking
            std::unordered_map<std::string, ECS::Entity::Type> name_lookup;

            std::string generateUniqueName(std::string const& base_name) const;

        public:
            Service() = default;

            // AppSystem overrides
            void onAttach() override;
            void onUpdate(AppTiming timing) override;
            void onDetach() override;
            void onFixedUpdate(AppTiming timing) override {}
            void onAppPause() override {}
            void onAppResume() override {}
            void onEvent(Event::Event& e) override;

            // === Name Management ===
            void setEntityName(ECS::Entity::Type entity, std::string const& name);
            std::string getEntityName(ECS::Entity::Type entity) const;
            std::optional<ECS::Entity::Type> getEntityByName(std::string const& name) const;
            bool isNameValid(std::string const& name) const;

            // === Tag System ===
            void registerTag(std::string const& tag);
            void unregisterTag(std::string const& tag);
            bool isTagValid(std::string const& tag) const;
            std::set<std::string> const& getRegisteredTags() const;

            void addTag(ECS::Entity::Type entity, std::string const& tag);
            void removeTag(ECS::Entity::Type entity, std::string const& tag);
            std::vector<ECS::Entity::Type> getEntitiesByTag(std::string const& tag) const;
            bool hasTag(ECS::Entity::Type entity, std::string const& tag) const;

            // === Hierarchy System ===
            void setParent(ECS::Entity::Type child, ECS::Entity::Type parent);
            void addChild(ECS::Entity::Type parent, ECS::Entity::Type child);
            void removeChild(ECS::Entity::Type parent, ECS::Entity::Type child);
            bool hasChildren(ECS::Entity::Type entity) const;
            bool hasParent(ECS::Entity::Type entity) const;
            void detachFromParent(ECS::Entity::Type entity);
            std::optional<ECS::Entity::Type> getParent(ECS::Entity::Type entity) const;
            std::vector<ECS::Entity::Type> getChildren(ECS::Entity::Type entity) const;

            // === Editor Visibility ===
            void setVisible(ECS::Entity::Type entity, bool visible);
            bool isVisible(ECS::Entity::Type entity) const;
            void setLocked(ECS::Entity::Type entity, bool locked);
            bool isLocked(ECS::Entity::Type entity) const;

            // === Group System ===
            bool createGroup(std::string const& group_name,
                std::optional<std::string> parent_group = std::nullopt);
            bool deleteGroup(std::string const& group_name, bool remove_entities = false);
            void assignToGroup(ECS::Entity::Type entity, std::string const& group_name);
            void unassignFromGroup(ECS::Entity::Type entity);
            std::optional<std::string> getEntityGroup(ECS::Entity::Type entity) const;

            std::set<std::string> getAllGroups() const;
            std::set<std::string> getRootGroups() const;

            // === Serialization ===
            nlohmann::json serializeEntity(ECS::Entity::Type entity) const;
            nlohmann::json serialize() const;
            void deserialize(nlohmann::json const& data);
            void reset();
        };

    }
} // namespace PAIN::MetaData

#endif // META_SERVICE_H
