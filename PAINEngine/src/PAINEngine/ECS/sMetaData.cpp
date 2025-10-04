/*****************************************************************//**
 * \file   sMetaData.cpp
 * \brief  Meta Data Management Implementation
 *
 * \author Ho Shu Hng, 2301339, [shuhng.ho@digipen.edu](mailto:shuhng.ho@digipen.edu) (100%)
 * \date   October 2024
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sMetaData.h"
#include "ECS/Components/cMetaData.h"
#include "Controller.h"
#include <algorithm>
#include <sstream>

namespace PAIN {
    namespace MetaData {

#define PN_ECS_SERVICE services->get<ECS::Controller>()

        void Service::onAttach()
        {
            // Initialize default tags
            registerTag("Untagged");
            registerTag("Player");
            registerTag("Enemy");
            registerTag("Environment");
        }

        void Service::onDetach() {
            reset();
        }

        void Service::onUpdate(AppTiming timing) {
            // Update name lookup cache if needed
        }

        void Service::onEvent(Event::Event& e) {
            // Handle entity creation/destruction events
        }

        std::string Service::generateUniqueName(std::string const& base_name) const {
            if (name_lookup.find(base_name) == name_lookup.end()) {
                return base_name;
            }

            int counter = 1;
            std::string unique_name;
            do {
                std::ostringstream oss;
                oss << base_name << " (" << counter++ << ")";
                unique_name = oss.str();
            } while (name_lookup.find(unique_name) != name_lookup.end());

            return unique_name;
        }

        /****************************************************************
        * Name operations
        **************************************************************/

        bool Service::isNameValid(std::string const& name) const {
            return !name.empty() && name_lookup.find(name) == name_lookup.end();
        }

        void Service::setEntityName(ECS::Entity::Type entity, std::string const& name) {
            auto name_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EntityName>(entity);

            if (!name_comp_opt.has_value()) {
                MetaData::EntityName new_name;
                new_name.name = generateUniqueName(name);
                name_lookup[new_name.name] = entity;
                PN_ECS_SERVICE->addEntityComponent(entity, std::move(new_name));
                return;
            }

            // Remove old mapping
            name_lookup.erase(name_comp_opt->get().name);

            // Ensure uniqueness
            std::string unique = generateUniqueName(name);
            name_comp_opt->get().name = unique;
            name_lookup[unique] = entity;
        }

        std::string Service::getEntityName(ECS::Entity::Type entity) const {
            auto name_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EntityName>(entity);
            return name_comp_opt.has_value() ? name_comp_opt->get().name : "";
        }

        std::optional<ECS::Entity::Type> Service::getEntityByName(std::string const& name) const {
            auto it = name_lookup.find(name);
            if (it != name_lookup.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        /****************************************************************
        * Tag System
        **************************************************************/

        void Service::registerTag(std::string const& tag) {
            registered_tags.insert(tag);
        }

        void Service::unregisterTag(std::string const& tag) {
            registered_tags.erase(tag);

            auto tag_type_opt = PN_ECS_SERVICE->getComponentType<MetaData::Tag>();
            if (!tag_type_opt.has_value()) return;

            auto entities_with_tag = PN_ECS_SERVICE->getAllComponentEntities(tag_type_opt.value());

            for (auto entity : entities_with_tag) {
                auto tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);
                if (tag_comp_opt.has_value()) {
                    tag_comp_opt->get().tags.erase(tag);
                }
            }
        }

        bool Service::isTagValid(std::string const& tag) const {
            return registered_tags.find(tag) != registered_tags.end();
        }

        std::set<std::string> const& Service::getRegisteredTags() const {
            return registered_tags;
        }

        void Service::addTag(ECS::Entity::Type entity, std::string const& tag) {
            auto tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);

            if (!tag_comp_opt.has_value()) {
                MetaData::Tag new_tag;
                new_tag.tags.insert(tag);
                PN_ECS_SERVICE->addEntityComponent(entity, std::move(new_tag));
            }
            else {
                tag_comp_opt->get().tags.insert(tag);
            }

            registered_tags.insert(tag);
        }

        void Service::removeTag(ECS::Entity::Type entity, std::string const& tag) {
            auto tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);
            if (tag_comp_opt.has_value()) {
                tag_comp_opt->get().tags.erase(tag);
            }
        }

        bool Service::hasTag(ECS::Entity::Type entity, std::string const& tag) const {
            auto tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);
            if (tag_comp_opt.has_value()) {
                return tag_comp_opt->get().tags.find(tag) != tag_comp_opt->get().tags.end();
            }
            return false;
        }

        std::vector<ECS::Entity::Type> Service::getEntitiesByTag(std::string const& tag) const {
            std::vector<ECS::Entity::Type> result;

            auto tag_type_opt = PN_ECS_SERVICE->getComponentType<MetaData::Tag>();
            if (!tag_type_opt.has_value()) return result;

            auto entities_with_tag = PN_ECS_SERVICE->getAllComponentEntities(tag_type_opt.value());

            for (auto entity : entities_with_tag) {
                auto tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);
                if (tag_comp_opt.has_value()) {
                    auto const& tags = tag_comp_opt->get().tags;
                    if (tags.find(tag) != tags.end()) {
                        result.push_back(entity);
                    }
                }
            }

            return result;
        }

        /****************************************************************
        * Hierarchy System
        **************************************************************/

        bool Service::hasParent(ECS::Entity::Type entity) const {
            auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
            return relation_opt.has_value() &&
                relation_opt->get().parent != ECS::Entity::INVALID;
        }

        bool Service::hasChildren(ECS::Entity::Type entity) const {
            auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
            return relation_opt.has_value() &&
                !relation_opt->get().children.empty();
        }

        void Service::addChild(ECS::Entity::Type parent, ECS::Entity::Type child) {
            auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(parent);

            if (!relation_opt.has_value()) {
                MetaData::Relation new_relation;
                new_relation.parent = ECS::Entity::INVALID;
                new_relation.children.push_back(child);
                PN_ECS_SERVICE->addEntityComponent(parent, std::move(new_relation));
            }
            else {
                auto& children = relation_opt->get().children;
                if (std::find(children.begin(), children.end(), child) == children.end()) {
                    children.push_back(child);
                }
            }
        }

        void Service::removeChild(ECS::Entity::Type parent, ECS::Entity::Type child) {
            auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(parent);
            if (relation_opt.has_value()) {
                auto& children = relation_opt->get().children;
                children.erase(
                    std::remove(children.begin(), children.end(), child),
                    children.end()
                );
            }
        }

        void Service::setParent(ECS::Entity::Type child, ECS::Entity::Type parent) {
            auto child_relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(child);

            if (!child_relation_opt.has_value()) {
                MetaData::Relation new_relation;
                new_relation.parent = parent;
                PN_ECS_SERVICE->addEntityComponent(child, std::move(new_relation));
            }
            else {
                if (child_relation_opt->get().parent != ECS::Entity::INVALID) {
                    removeChild(child_relation_opt->get().parent, child);
                }
                child_relation_opt->get().parent = parent;
            }

            addChild(parent, child);
        }

        void Service::detachFromParent(ECS::Entity::Type entity) {
            auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
            if (!relation_opt.has_value()) return;

            auto& relation = relation_opt->get();
            if (relation.parent == ECS::Entity::INVALID) return;

            removeChild(relation.parent, entity);
            relation.parent = ECS::Entity::INVALID;
        }

        std::optional<ECS::Entity::Type> Service::getParent(ECS::Entity::Type entity) const {
            auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
            if (relation_opt.has_value() &&
                relation_opt->get().parent != ECS::Entity::INVALID) {
                return relation_opt->get().parent;
            }
            return std::nullopt;
        }

        std::vector<ECS::Entity::Type> Service::getChildren(ECS::Entity::Type entity) const {
            auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
            return relation_opt.has_value() ?
                relation_opt->get().children :
                std::vector<ECS::Entity::Type>{};
        }

        /****************************************************************
        * Editor Visibility
        **************************************************************/

        void Service::setVisible(ECS::Entity::Type entity, bool visible) {
            auto vis_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EditorVisible>(entity);

            if (!vis_opt.has_value()) {
                MetaData::EditorVisible new_visible;
                new_visible.visible = visible;
                new_visible.locked = false;
                PN_ECS_SERVICE->addEntityComponent(entity, std::move(new_visible));
            }
            else {
                vis_opt->get().visible = visible;
            }
        }

        bool Service::isVisible(ECS::Entity::Type entity) const {
            auto vis_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EditorVisible>(entity);
            return vis_opt.has_value() ? vis_opt->get().visible : true;
        }

        void Service::setLocked(ECS::Entity::Type entity, bool locked) {
            auto vis_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EditorVisible>(entity);

            if (!vis_opt.has_value()) {
                MetaData::EditorVisible new_visible;
                new_visible.visible = true;
                new_visible.locked = locked;
                PN_ECS_SERVICE->addEntityComponent(entity, std::move(new_visible));
            }
            else {
                vis_opt->get().locked = locked;
            }
        }

        bool Service::isLocked(ECS::Entity::Type entity) const {
            auto vis_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EditorVisible>(entity);
            return vis_opt.has_value() ? vis_opt->get().locked : false;
        }

        /****************************************************************
        * Group System
        **************************************************************/

        bool Service::createGroup(std::string const& group_name, std::optional<std::string> parent_group) {
            if (groups.find(group_name) != groups.end()) return false;

            GroupData group;
            group.name = group_name;
            group.parent_group = parent_group;
            group.expanded = true;

            if (parent_group.has_value()) {
                auto parent_it = groups.find(parent_group.value());
                if (parent_it != groups.end()) {
                    parent_it->second.child_groups.insert(group_name);
                }
            }

            groups[group_name] = group;
            return true;
        }

        bool Service::deleteGroup(std::string const& group_name, bool remove_entities) {
            auto it = groups.find(group_name);
            if (it == groups.end()) return false;

            if (remove_entities) {
                auto group_type_opt = PN_ECS_SERVICE->getComponentType<MetaData::Group>();
                if (group_type_opt.has_value()) {
                    auto entities = PN_ECS_SERVICE->getAllComponentEntities(group_type_opt.value());

                    for (auto entity : entities) {
                        auto group_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Group>(entity);
                        if (group_opt.has_value() && group_opt->get().group_name == group_name) {
                            PN_ECS_SERVICE->removeEntityComponent<MetaData::Group>(entity);
                        }
                    }
                }
            }

            groups.erase(it);
            return true;
        }

        void Service::assignToGroup(ECS::Entity::Type entity, std::string const& group_name) {
            auto group_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Group>(entity);

            if (!group_opt.has_value()) {
                MetaData::Group new_group;
                new_group.group_name = group_name;
                PN_ECS_SERVICE->addEntityComponent(entity, std::move(new_group));
            }
            else {
                group_opt->get().group_name = group_name;
            }

            auto it = groups.find(group_name);
            if (it != groups.end()) {
                it->second.entities.insert(entity);
            }
        }

        void Service::unassignFromGroup(ECS::Entity::Type entity) {
            auto group_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Group>(entity);
            if (group_opt.has_value()) {
                auto it = groups.find(group_opt->get().group_name);
                if (it != groups.end()) {
                    it->second.entities.erase(entity);
                }
                PN_ECS_SERVICE->removeEntityComponent<MetaData::Group>(entity);
            }
        }

        std::optional<std::string> Service::getEntityGroup(ECS::Entity::Type entity) const {
            auto group_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Group>(entity);
            if (group_opt.has_value()) {
                return group_opt->get().group_name;
            }
            return std::nullopt;
        }

        std::set<std::string> Service::getAllGroups() const {
            std::set<std::string> result;
            for (auto const& [name, group] : groups) {
                result.insert(name);
            }
            return result;
        }

        std::set<std::string> Service::getRootGroups() const {
            std::set<std::string> result;
            for (auto const& [name, group] : groups) {
                if (!group.parent_group.has_value()) {
                    result.insert(name);
                }
            }
            return result;
        }

        /****************************************************************
        * Serialization
        **************************************************************/

        nlohmann::json Service::serializeEntity(ECS::Entity::Type entity) const {
            nlohmann::json data;

            auto name_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EntityName>(entity);
            if (name_opt.has_value()) {
                data["name"] = name_opt->get().name;
            }

            auto tag_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);
            if (tag_opt.has_value()) {
                data["tags"] = tag_opt->get().tags;
            }

            auto vis_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EditorVisible>(entity);
            if (vis_opt.has_value()) {
                data["visible"] = vis_opt->get().visible;
                data["locked"] = vis_opt->get().locked;
            }

            auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
            if (relation_opt.has_value()) {
                data["parent"] = static_cast<uint32_t>(relation_opt->get().parent);
                data["children"] = std::vector<uint32_t>(
                    relation_opt->get().children.begin(),
                    relation_opt->get().children.end()
                );
            }

            auto group_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Group>(entity);
            if (group_opt.has_value()) {
                data["group"] = group_opt->get().group_name;
            }

            return data;
        }

        nlohmann::json Service::serialize() const {
            nlohmann::json data;
            data["registered_tags"] = registered_tags;

            nlohmann::json groups_data;
            for (auto const& [name, group] : groups) {
                groups_data[name] = {
                    {"name", group.name},
                    {"parent_group", group.parent_group.has_value() ? group.parent_group.value() : ""},
                    {"child_groups", group.child_groups},
                    {"entities", std::vector<uint32_t>(group.entities.begin(), group.entities.end())},
                    {"expanded", group.expanded}
                };
            }
            data["groups"] = groups_data;

            return data;
        }

        void Service::deserialize(nlohmann::json const& data) {
            if (data.contains("registered_tags")) {
                registered_tags = data["registered_tags"].get<std::set<std::string>>();
            }

            if (data.contains("groups")) {
                auto groups_data = data["groups"];
                for (auto const& [name, group_json] : groups_data.items()) {
                    GroupData group;
                    group.name = group_json.value("name", "");

                    std::string parent = group_json.value("parent_group", "");
                    group.parent_group = parent.empty() ? std::nullopt : std::optional<std::string>(parent);

                    if (group_json.contains("child_groups")) {
                        group.child_groups = group_json["child_groups"].get<std::set<std::string>>();
                    }

                    if (group_json.contains("entities")) {
                        auto entity_array = group_json["entities"].get<std::vector<uint32_t>>();
                        group.entities = std::set<ECS::Entity::Type>(entity_array.begin(), entity_array.end());
                    }

                    group.expanded = group_json.value("expanded", true);
                    groups[name] = group;
                }
            }
        }

        void Service::reset() {
            registered_tags.clear();
            groups.clear();
            name_lookup.clear();
        }

    }
} // namespace PAIN::MetaData
