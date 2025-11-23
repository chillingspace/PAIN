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
            std::set<entt::entity> entities;
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
            std::unordered_map<std::string, entt::entity> name_lookup;

            std::string generateUniqueName(std::string const& base_name) const;

            bool b_entity_changed = false;

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
            void setEntityName(entt::entity entity, std::string const& name);
            std::string getEntityName(entt::entity entity) const;
            std::optional<entt::entity> getEntityByName(std::string const& name) const;
            bool isNameValid(std::string const& name) const;
            bool entityNameChanged();

            // === Tag System ===
            void registerTag(std::string const& tag);
            void unregisterTag(std::string const& tag);
            bool isTagValid(std::string const& tag) const;
            std::set<std::string> const& getRegisteredTags() const;

            void addTag(entt::entity entity, std::string const& tag);
            void removeTag(entt::entity entity, std::string const& tag);
            std::vector<entt::entity> getEntitiesByTag(std::string const& tag) const;
            bool hasTag(entt::entity entity, std::string const& tag) const;
            void setEntityTag(entt::entity entity, const std::string& new_tag);

            // === Hierarchy System ===
            //void setParent(entt::entity child, entt::entity parent);
            //void addChild(entt::entity parent, entt::entity child);
            //void removeChild(entt::entity parent, entt::entity child);
            //bool hasChildren(entt::entity entity) const;
            //bool hasParent(entt::entity entity) const;
            //void detachFromParent(entt::entity entity);
            //std::optional<entt::entity> getParent(entt::entity entity) const;
            //std::vector<entt::entity> getChildren(entt::entity entity) const;

            // === Editor Visibility ===
            void setVisible(entt::entity entity, bool visible);
            bool isVisible(entt::entity entity) const;
            void setLocked(entt::entity entity, bool locked);
            bool isLocked(entt::entity entity) const;

            // === Group System ===
            //bool createGroup(std::string const& group_name,
            //    std::optional<std::string> parent_group = std::nullopt);
            //bool deleteGroup(std::string const& group_name, bool remove_entities = false);
            //void assignToGroup(entt::entity entity, std::string const& group_name);
            //void unassignFromGroup(entt::entity entity);
            //std::optional<std::string> getEntityGroup(entt::entity entity) const;

            //std::set<std::string> getAllGroups() const;
            //std::set<std::string> getRootGroups() const;

            // === Serialization ===
            nlohmann::json serializeServiceState() const;
            void deserializeServiceState(nlohmann::json const& data);
            void reset();
        };

    }
} // namespace PAIN::MetaData

#endif // META_SERVICE_H
