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
#include "ECS/Components/cMetadata.h"
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

            registerTag("letter_collectible");
            registerTag("letter_carried");
            registerTag("letter_collection");
            registerTag("hiding_spot");
            registerTag("enemy_patrol_start");
            registerTag("enemy_patrol_end");
            registerTag("android_ui");
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

        void Service::setEntityName(entt::entity entity, std::string const& name) {
            auto name_comp_opt = PN_ECS_SERVICE->getEntityComponent<Entity::Name>(entity);

            if (!name_comp_opt.has_value()) {
                Entity::Name new_name;
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

            b_entity_changed = true;
        }

        std::string Service::getEntityName(entt::entity entity) const {
            auto name_comp_opt = PN_ECS_SERVICE->getEntityComponent<Entity::Name>(entity);
            return name_comp_opt.has_value() ? name_comp_opt->get().name : "";
        }

        std::optional<entt::entity> Service::getEntityByName(std::string const& name) const {
            auto it = name_lookup.find(name);
            if (it != name_lookup.end()) {
                return it->second;
            }
            return std::nullopt;
        }

        bool Service::entityNameChanged() {
            // atomically read+clear
            if (b_entity_changed) {
                b_entity_changed = false;
                return true;
            }
            return false;
        }

        /****************************************************************
        * Tag System
        **************************************************************/

        void Service::registerTag(std::string const& tag) {
            registered_tags.insert(tag);
        }

        void Service::unregisterTag(std::string const& tag) {
            registered_tags.erase(tag);

            // Remove tag from all entities
            auto& registry = PN_ECS_SERVICE->getRegistry();
            auto view = registry.view<MetaData::Tag>();

            for (auto entity : view) {
                auto& tag_comp = view.get<MetaData::Tag>(entity);
                tag_comp.tags.erase(tag);
            }
        }

        bool Service::isTagValid(std::string const& tag) const {
            return registered_tags.find(tag) != registered_tags.end();
        }

        std::set<std::string> const& Service::getRegisteredTags() const {
            return registered_tags;
        }

        void Service::addTag(entt::entity entity, std::string const& tag) {
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

        void Service::removeTag(entt::entity entity, std::string const& tag) {
            auto tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);
            if (tag_comp_opt.has_value()) {
                tag_comp_opt->get().tags.erase(tag);
            }
        }

        bool Service::hasTag(entt::entity entity, std::string const& tag) const {
            auto tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);
            if (tag_comp_opt.has_value()) {
                return tag_comp_opt->get().tags.find(tag) != tag_comp_opt->get().tags.end();
            }
            return false;
        }

        std::vector<entt::entity> Service::getEntitiesByTag(std::string const& tag) const {
            std::vector<entt::entity> result;

            auto& registry = PN_ECS_SERVICE->getRegistry();
            auto view = registry.view<MetaData::Tag>();

            for (auto entity : view) {
                auto& tag_comp = view.get<MetaData::Tag>(entity);
                if (tag_comp.tags.find(tag) != tag_comp.tags.end()) {
                    result.push_back(entity);
                }
            }

            return result;
        }

        void Service::setEntityTag(entt::entity entity, const std::string& new_tag) {
            // Remove all existing tags from this entity
            auto tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity);
            if (tag_comp_opt.has_value()) {
                tag_comp_opt->get().tags.clear();
            }
            else {
                MetaData::Tag new_tag_comp;
                PN_ECS_SERVICE->addEntityComponent(entity, std::move(new_tag_comp));
                tag_comp_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Tag>(entity); // refresh
            }
            // Add the new tag
            if (!new_tag.empty()) {
                tag_comp_opt->get().tags.insert(new_tag);
                registered_tags.insert(new_tag);
            }
        }


        /****************************************************************
        * Hierarchy System
        **************************************************************/

        //bool Service::hasParent(entt::entity entity) const {
        //    auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
        //    return relation_opt.has_value() &&
        //        relation_opt->get().parent != entt::null;
        //}

        //bool Service::hasChildren(entt::entity entity) const {
        //    auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
        //    return relation_opt.has_value() &&
        //        !relation_opt->get().children.empty();
        //}

        //void Service::addChild(entt::entity parent, entt::entity child) {
        //    auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(parent);

        //    if (!relation_opt.has_value()) {
        //        MetaData::Relation new_relation;
        //        new_relation.parent = entt::null;
        //        new_relation.children.push_back(child);
        //        PN_ECS_SERVICE->addEntityComponent(parent, std::move(new_relation));
        //    }
        //    else {
        //        auto& children = relation_opt->get().children;
        //        if (std::find(children.begin(), children.end(), child) == children.end()) {
        //            children.push_back(child);
        //        }
        //    }
        //}

        //void Service::removeChild(entt::entity parent, entt::entity child) {
        //    auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(parent);
        //    if (relation_opt.has_value()) {
        //        auto& children = relation_opt->get().children;
        //        children.erase(
        //            std::remove(children.begin(), children.end(), child),
        //            children.end()
        //        );
        //    }
        //}

        //void Service::setParent(entt::entity child, entt::entity parent) {
        //    auto child_relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(child);

        //    if (!child_relation_opt.has_value()) {
        //        MetaData::Relation new_relation;
        //        new_relation.parent = parent;
        //        PN_ECS_SERVICE->addEntityComponent(child, std::move(new_relation));
        //    }
        //    else {
        //        if (child_relation_opt->get().parent != entt::null) {
        //            removeChild(child_relation_opt->get().parent, child);
        //        }
        //        child_relation_opt->get().parent = parent;
        //    }

        //    addChild(parent, child);
        //}

        //void Service::detachFromParent(entt::entity entity) {
        //    auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
        //    if (!relation_opt.has_value()) return;

        //    auto& relation = relation_opt->get();
        //    if (relation.parent == entt::null) return;

        //    removeChild(relation.parent, entity);
        //    relation.parent = entt::null;
        //}

        //std::optional<entt::entity> Service::getParent(entt::entity entity) const {
        //    auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
        //    if (relation_opt.has_value() &&
        //        relation_opt->get().parent != entt::null) {
        //        return relation_opt->get().parent;
        //    }
        //    return std::nullopt;
        //}

        //std::vector<entt::entity> Service::getChildren(entt::entity entity) const {
        //    auto relation_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Relation>(entity);
        //    return relation_opt.has_value() ?
        //        relation_opt->get().children :
        //        std::vector<entt::entity>{};
        //}

        /****************************************************************
        * Editor Visibility
        **************************************************************/

        void Service::setVisible(entt::entity entity, bool visible) {
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

        bool Service::isVisible(entt::entity entity) const {
            auto vis_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EditorVisible>(entity);
            return vis_opt.has_value() ? vis_opt->get().visible : true;
        }

        void Service::setLocked(entt::entity entity, bool locked) {
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

        bool Service::isLocked(entt::entity entity) const {
            auto vis_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::EditorVisible>(entity);
            return vis_opt.has_value() ? vis_opt->get().locked : false;
        }

        /****************************************************************
        * Group System
        **************************************************************/

        //bool Service::createGroup(std::string const& group_name, std::optional<std::string> parent_group) {
        //    if (groups.find(group_name) != groups.end()) return false;

        //    GroupData group;
        //    group.name = group_name;
        //    group.parent_group = parent_group;
        //    group.expanded = true;

        //    if (parent_group.has_value()) {
        //        auto parent_it = groups.find(parent_group.value());
        //        if (parent_it != groups.end()) {
        //            parent_it->second.child_groups.insert(group_name);
        //        }
        //    }

        //    groups[group_name] = group;
        //    return true;
        //}

        //bool Service::deleteGroup(std::string const& group_name, bool remove_entities) {
        //    auto it = groups.find(group_name);
        //    if (it == groups.end()) return false;

        //    if (remove_entities) {

        //        auto& registry = PN_ECS_SERVICE->getRegistry();
        //        auto view = registry.view<MetaData::Group>();

        //        for (auto entity : view) {
        //            auto& group_comp = view.get<MetaData::Group>(entity);
        //            if (group_comp.group_name == group_name) {
        //                PN_ECS_SERVICE->removeEntityComponent<MetaData::Group>(entity);
        //            }
        //        }
        //        
        //    }

        //    groups.erase(it);
        //    return true;
        //}

        //void Service::assignToGroup(entt::entity entity, std::string const& group_name) {
        //    auto group_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Group>(entity);

        //    if (!group_opt.has_value()) {
        //        MetaData::Group new_group;
        //        new_group.group_name = group_name;
        //        PN_ECS_SERVICE->addEntityComponent(entity, std::move(new_group));
        //    }
        //    else {
        //        group_opt->get().group_name = group_name;
        //    }

        //    auto it = groups.find(group_name);
        //    if (it != groups.end()) {
        //        it->second.entities.insert(entity);
        //    }
        //}

        //void Service::unassignFromGroup(entt::entity entity) {
        //    auto group_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Group>(entity);
        //    if (group_opt.has_value()) {
        //        auto it = groups.find(group_opt->get().group_name);
        //        if (it != groups.end()) {
        //            it->second.entities.erase(entity);
        //        }
        //        PN_ECS_SERVICE->removeEntityComponent<MetaData::Group>(entity);
        //    }
        //}

        //std::optional<std::string> Service::getEntityGroup(entt::entity entity) const {
        //    auto group_opt = PN_ECS_SERVICE->getEntityComponent<MetaData::Group>(entity);
        //    if (group_opt.has_value()) {
        //        return group_opt->get().group_name;
        //    }
        //    return std::nullopt;
        //}

        //std::set<std::string> Service::getAllGroups() const {
        //    std::set<std::string> result;
        //    for (auto const& [name, group] : groups) {
        //        result.insert(name);
        //    }
        //    return result;
        //}

        //std::set<std::string> Service::getRootGroups() const {
        //    std::set<std::string> result;
        //    for (auto const& [name, group] : groups) {
        //        if (!group.parent_group.has_value()) {
        //            result.insert(name);
        //        }
        //    }
        //    return result;
        //}

        /****************************************************************
        * Serialization
        **************************************************************/


        nlohmann::json Service::serializeServiceState() const {
            nlohmann::json data;
            data["registered_tags"] = registered_tags;

            nlohmann::json groups_data;
            for (auto const& [name, group] : groups) {
                // Convert set<entt::entity> to vector<uint32_t> manually
                std::vector<uint32_t> entity_ids;
                entity_ids.reserve(group.entities.size());
                for (auto entity : group.entities) {
                    // Cast each entity
                    entity_ids.push_back(static_cast<uint32_t>(entity));  
                }

                groups_data[name] = {
                    {"name", group.name},
                    {"parent_group", group.parent_group.has_value() ? group.parent_group.value() : ""},
                    {"child_groups", group.child_groups},
                    // Use converted entity vector
                    {"entities", entity_ids},  
                    {"expanded", group.expanded}
                };
            }

            data["groups"] = groups_data;
            return data;
        }


        void Service::deserializeServiceState(nlohmann::json const& data) {
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

                        std::transform(
                            entity_array.begin(),
                            entity_array.end(),
                            std::inserter(group.entities, group.entities.end()),
                            [](uint32_t id) { return static_cast<entt::entity>(id); }
                        );
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
