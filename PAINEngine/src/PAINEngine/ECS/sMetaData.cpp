/*****************************************************************//**
 * \file   sMetaData.cpp
 * \brief  Meta Data Management
 *
 * \author Ho Shu Hng, 2301339, shuhng.ho@digipen.edu (100%)
 * \date   September 2024
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sMetaData.h"
#include "Controller.h"

namespace PAIN {
	namespace MetaData {

#define PN_ECS_SERVICE services->get<ECS::Controller>()

		void Service::onAttach()
		{
			std::string const& def_entity_name = "entity_";
			def_name = def_entity_name;
		}

		void Service::onDetach() {

		}

		void  Service::onAppPause() {

		}
		void  Service::onAppResume() {

		}

		void Service::onUpdate(AppTiming timing) {
			if (!entities_to_destroy.empty()) {
				for (auto entity : entities_to_destroy) {
					if (PN_ECS_SERVICE->checkEntity(entity)) {

						auto it = entities.find(entity);
						if (it == entities.end()) continue;

						// Check if relation exists before accessing
						if (it->second.relation.has_value()) {
							auto* parent = std::get_if<Parent>(&it->second.relation.value());
							if (parent && !parent->childrens.empty()) {

								// Collect children first to avoid iterator issues
								std::vector<ECS::Entity::Type> child_entities;
								for (auto const& child : parent->childrens) {
									auto c_entity = getEntityByName(child);
									if (c_entity.has_value() &&
										PN_ECS_SERVICE->checkEntity(c_entity.value())) {
										child_entities.push_back(c_entity.value());
									}
								}

								// Destroy children
								for (auto child : child_entities) {
									PN_ECS_SERVICE->destroyEntity(child);
								}
							}
						}

						// Destroy entity
						PN_ECS_SERVICE->destroyEntity(entity);
					}
				}

				// Clear entities to destroy
				entities_to_destroy.clear();
			}
		}

		/****************************************************************
		* Helpers
		**************************************************************/

		std::string Service::generateUniqueName(std::string const& base_name) const {
			std::string candidate = base_name;
			int counter = 1;

			// Keep incrementing until we find an unused name
			while (entity_names.find(candidate) != entity_names.end()) {
				candidate = base_name + "_" + std::to_string(counter);
				counter++;
			}

			return candidate;
		}

		void Service::removeEntityFromGroup(ECS::Entity::Type entity) {
			auto it = entity_to_group.find(entity);
			if (it == entity_to_group.end()) {
				return; // Entity not in any group
			}

			std::string group_name = it->second;

			// Remove entity from the group's entity set
			auto group_it = groups.find(group_name);
			if (group_it != groups.end()) {
				group_it->second.entities.erase(entity);
			}

			// Remove from entity-to-group mapping
			entity_to_group.erase(it);
		}


		void Service::onEvent(Event::Event& e) {
#ifdef PN_PLATFORM_WINDOWS
			Event::Dispatcher dispatcher(e);
			// Check if it's an EntitiesChanged event
			if (e.getType() == Event::Type::EntitiesChange) {
				// Cast to the specific event type
				PAIN::ECS::EntitiesChanged& entities_changed = static_cast<PAIN::ECS::EntitiesChanged&>(e);

				// Update our copy of active entities
				ecs_entities = entities_changed.entities;

				// Remove entities that are no longer in the ECS
				for (auto it = entities.begin(); it != entities.end();) {
					if (ecs_entities.find(it->first) == ecs_entities.end()) {
						// Entity was destroyed in ECS
						// Clean up metadata

						// Remove from name mapping
						entity_names.erase(it->second.name);

						// Remove from group if assigned
						removeEntityFromGroup(it->first);

						// Erase from entities map
						it = entities.erase(it);
					}
					else {
						++it;
					}
				}

				// Update metadata for any new entities
				updateData();
			}
#endif
		}

		void Service::updateData() {
			// Clear entity names & repopulate them with updated names
			entity_names.clear();

			// Remove entities that no longer exist in ECS
			for (auto it = entities.begin(); it != entities.end(); ) {
				if (ecs_entities.find(it->first) == ecs_entities.end()) {
					removeEntityFromGroup(it->first);
					it = entities.erase(it);
				}
				else {
					++it;
				}
			}

			// Add new entities from the ECS that are not yet in the editor
			for (auto entity : ecs_entities) {

				auto it = entities.find(entity);
				bool needs_rename = false;

				if (it == entities.end()) {
					// New entity - create with default name
					needs_rename = true;
				}
				else if (it->second.name.find(def_name) != std::string::npos) {
					// Existing entity with default name pattern - regenerate
					needs_rename = true;
				}

				if (needs_rename) {
					// Use std::format (C++20) or create string properly
					std::string new_name = def_name + std::to_string(entity);

					// Ensure name is unique
					new_name = generateUniqueName(new_name);

					if (it == entities.end()) {
						// Insert new entity
						entities.emplace(entity, EntityData(new_name));
						//setEntityLayerID(entity, 0);
					}
					else {
						// Update existing entity
						it->second.name = new_name;
					}
				}

				// Populate entity name mapping
				entity_names[entities[entity].name] = entity;
			}
		}

		/****************************************************************
		* Name ops
		**************************************************************/

		bool MetaData::Service::isNameValid(std::string const& name) const {
			// Name is valid if:
			// 1. Not empty
			// 2. Not already taken by another entity
			return !name.empty() && entity_names.find(name) == entity_names.end();
		}

		void Service::setEntityName(ECS::Entity::Type entity, std::string const& name) {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			if (it->second.name == name) {
				return;
			}

			if (!isNameValid(name)) {
				PN_CORE_WARN("Name '{}' is already taken", name);
				return;
			}

			std::string old_name = it->second.name;

			// Update parent's children set if this is a child
			if (it->second.relation.has_value()) {
				if (auto* child_rel = std::get_if<Child>(&it->second.relation.value())) {
					if (!child_rel->parent.empty()) {
						auto parent_entity = getEntityByName(child_rel->parent);
						if (parent_entity.has_value()) {
							auto parent_it = entities.find(parent_entity.value());
							// Prevent crashes, check for optional first
							if (parent_it != entities.end() && parent_it->second.relation.has_value()) {
								if (auto* parent_rel = std::get_if<Parent>(&parent_it->second.relation.value())) {
									parent_rel->childrens.erase(old_name);
									parent_rel->childrens.insert(name);
								}
							}
						}
					}
				}

				// Update children's parent reference if this is a parent
				if (auto* parent_rel = std::get_if<Parent>(&it->second.relation.value())) {
					for (auto const& child_name : parent_rel->childrens) {
						auto child_entity = getEntityByName(child_name);
						if (child_entity.has_value()) {
							auto child_it = entities.find(child_entity.value());
							// Prevent crashes, check for optional first
							if (child_it != entities.end() && child_it->second.relation.has_value()) {
								if (auto* child_data = std::get_if<Child>(&child_it->second.relation.value())) {
									child_data->parent = name;
								}
							}
						}
					}
				}
			}

			// Remove old name from entity_names
			entity_names.erase(old_name);

			// Set new name
			it->second.name = name;

			// Add new name to entity_names
			entity_names[name] = entity;
		}

		std::string Service::getEntityName(ECS::Entity::Type entity) const
		{
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return "";
			}

			return it->second.name;  // Use iterator
		}
		std::optional<ECS::Entity::Type> Service::getEntityByName(std::string const& name) const
		{
			auto it = entity_names.find(name);
			if (it != entity_names.end()) {
				return it->second;
			}
			return std::nullopt;
		}

		void Service::destroyEntity(ECS::Entity::Type entity) {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			// Add to destroy queue
			entities_to_destroy.insert(entity);
		}

		void Service::destroyAllEntities() {
			for (auto& [entity_id, data] : entities) {
				entities_to_destroy.insert(entity_id);
			}
		}

		/****************************************************************
		* Visbiility
		**************************************************************/

		void Service::setEntityVisible(ECS::Entity::Type entity, bool visible) {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			it->second.b_visible = visible;
		}

		bool Service::isEntityVisible(ECS::Entity::Type entity) const {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return true; // Default to visible if not found
			}

			return it->second.b_visible;
		}

		/****************************************************************
		* Locking entity
		**************************************************************/

		void Service::setEntityLocked(ECS::Entity::Type entity, bool b_locked) {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			it->second.b_locked = b_locked;  
		}

		bool Service::isEntityLocked(ECS::Entity::Type entity) const
		{
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return false;
			}

			return it->second.b_locked;
		}

		void Service::setAllEntitiesLocked(bool b_locked) {
			for (auto& e_data : entities) {
				e_data.second.b_locked = b_locked;
			}
		}

		/****************************************************************
		* Tagging
		**************************************************************/

		bool Service::isTagValid(std::string const& tag) const {
			return entity_tags.find(tag) != entity_tags.end();
		}

		void MetaData::Service::registerTag(std::string const& tag) {
			entity_tags.insert(tag);
		}

		void MetaData::Service::unregisterTag(std::string const& tag) {
			for (auto& [entity, data] : entities) {
				data.tags.erase(tag);
			}

			// Remove from registered tags
			entity_tags.erase(tag);
		}

		std::set<std::string> const& MetaData::Service::getRegisteredTags() const {
			return entity_tags;
		}
		void Service::addEntityTag(ECS::Entity::Type entity, std::string const& tag)
		{
			// Check if entity exists
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			// Check if tag has been registered
			if (!isTagValid(tag)) {
				PN_CORE_WARN("Tag not registered yet.");
				return;
			}

			// Add tag using the iterator
			it->second.tags.insert(tag);
		}
		void Service::removeEntityTag(ECS::Entity::Type entity, std::string const& tag)
		{
			//Check if entity exists
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			//Check if tag has been registered
			if (!isTagValid(tag)) {
				PN_CORE_WARN("Tag not registered yet.");
				return;
			}

			//Set tag
			it->second.tags.erase(tag);
		}
		bool Service::hasEntityTag(ECS::Entity::Type entity, std::string const& tag) const
		{
			// Check if entity exists
			auto it = entities.find(entity);
			if (it == entities.end()) {
				return false;
			}

			// Check if the entity has the specific tag
			return it->second.tags.find(tag) != it->second.tags.end();
		}

		std::set<ECS::Entity::Type> Service::getEntitiesByTag(std::string const& tag) const
		{
			//Set of entity
			std::set<ECS::Entity::Type> type_entities;

			//Check if tag is present within entity
			for (auto const& entity_data : entities) {
				if (entity_data.second.tags.find(tag) != entity_data.second.tags.end()) {
					type_entities.insert(entity_data.first);
				}
			}

			return type_entities;
		}

		std::set<std::string> Service::getEntityTags(ECS::Entity::Type entity) const {
			// Check if entity exists
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return std::set<std::string>();
			}

			return it->second.tags;
		}

		/****************************************************************
		* Parent-child hierachy
		**************************************************************/
		void Service::setEntityAsParent(ECS::Entity::Type entity) {
			// Check if entity exists
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			// Check if relation exists and is a child
			if (it->second.relation.has_value()) {
				auto* child = std::get_if<Child>(&it->second.relation.value());
				if (!child) return; // Already a parent or no relation
			}

			// Set relation to Parent
			it->second.relation = Parent();

			// Update relation
			updateRelation();
		}

		void Service::setEntityAsChild(ECS::Entity::Type entity, ECS::Entity::Type parent) {
			// Check if entity exists
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			// Check if parent exists
			auto parent_it = entities.find(parent);
			if (parent_it == entities.end()) {
				PN_CORE_WARN("Parent entity does not exist");
				return;
			}

			// Ensure parent has Parent relation
			if (!parent_it->second.relation.has_value() ||
				!std::holds_alternative<Parent>(parent_it->second.relation.value())) {
				PN_CORE_WARN("Target entity is not a parent");
				return;
			}

			// Check if relation exists and is a parent
			if (it->second.relation.has_value()) {
				auto* parent_rel = std::get_if<Parent>(&it->second.relation.value());
				if (!parent_rel) return; // Already a child
			}

			// Set relation to Child with parent reference
			Child child_rel;
			child_rel.parent = parent_it->second.name;
			it->second.relation = child_rel;

			// Update relation
			updateRelation();
		}

		void Service::detachEntityFromParent(ECS::Entity::Type entity)
		{
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			// Check if relation exists and is a child
			if (it->second.relation.has_value()) {
				auto* child = std::get_if<Child>(&it->second.relation.value());
				if (!child) return; // Not a child, nothing to detach

				// Clear parent reference
				child->parent.clear();

				// Convert to parent (standalone entity)
				it->second.relation = Parent();

				// Update all relations
				updateRelation();
			}
		}

		bool Service::isParent(ECS::Entity::Type entity) const
		{
			auto it = entities.find(entity);
			if (it == entities.end()) {
				return false;
			}

			// Check if relation exists and is a Parent
			if (it->second.relation.has_value()) {
				// Std::holds_alternative ... Checks if the variant v holds the alternative T 
				return std::holds_alternative<Parent>(it->second.relation.value());
			}

			return false;
		}

		bool Service::isParent(std::string const& parent_name) const
		{
			auto entity = getEntityByName(parent_name);

			if (!entity.has_value()) {
				return false;
			}

			return isParent(entity.value());
		}

		bool Service::isChild(ECS::Entity::Type entity) const
		{
			auto it = entities.find(entity);
			if (it == entities.end()) {
				return false;
			}

			// Check if relation exists and is a Child
			if (it->second.relation.has_value()) {
				// Std::holds_alternative ... Checks if the variant v holds the alternative T 
				return std::holds_alternative<Child>(it->second.relation.value());
			}

			return false;
		}

		std::optional<ECS::Entity::Type> Service::getEntityParent(ECS::Entity::Type entity) const
		{
			auto it = entities.find(entity);
			if (it == entities.end()) {
				return std::nullopt;
			}

			// Check if relation exists and is a child
			if (it->second.relation.has_value()) {
				if (auto* child = std::get_if<Child>(&it->second.relation.value())) {
					if (!child->parent.empty()) {
						return getEntityByName(child->parent);
					}
				}
			}

			return std::nullopt;
		}

		std::set<ECS::Entity::Type> Service::getEntityChildren(ECS::Entity::Type entity) const
		{
			std::set<ECS::Entity::Type> children;

			auto it = entities.find(entity);
			if (it == entities.end()) {
				return children;
			}

			// Check if relation exists and is a parent
			if (it->second.relation.has_value()) {
				if (auto* parent = std::get_if<Parent>(&it->second.relation.value())) {
					// Convert child names to entity IDs
					for (const auto& child_name : parent->childrens) {
						auto child_entity = getEntityByName(child_name);
						if (child_entity.has_value()) {
							children.insert(child_entity.value());
						}
					}
				}
			}

			return children;
		}

		std::optional<std::variant<Parent, Child>> Service::getEntityRelation(ECS::Entity::Type entity) const
		{
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return std::nullopt;
			}

			return it->second.relation;
		}



		void Service::updateRelation() {
			// Set of parents & children
			std::unordered_map<std::string, Parent*> parents;
			std::unordered_map<std::string, Child*> childs;

			// Iterate through entities
			for (auto& [entity_id, data] : entities) {
				// Check if relation exists
				if (!data.relation.has_value()) {
					continue;
				}

				// Get parent
				if (auto* parent = std::get_if<Parent>(&data.relation.value())) {
					parent->childrens.clear();
					parents[data.name] = parent;
				}

				// Get child
				if (auto* child = std::get_if<Child>(&data.relation.value())) {
					childs[data.name] = child;
				}
			}

			// Update each set properly
			for (auto& [child_name, child_ptr] : childs) {
				if (!child_ptr->parent.empty()) {
					auto parent_it = parents.find(child_ptr->parent);
					if (parent_it != parents.end()) {
						parent_it->second->childrens.insert(child_name);
					}
				}
			}
		}

		/****************************************************************
		* Group system
		**************************************************************/

		bool Service::createGroup(std::string const& group_name, std::optional<std::string> parent_group) {
			// Check if group already exists
			if (groups.find(group_name) != groups.end()) {
				PN_CORE_WARN("Group already exists");
				return false;
			}

			// If parent specified, verify it exists
			if (parent_group.has_value()) {
				if (groups.find(parent_group.value()) == groups.end()) {
					PN_CORE_WARN("Parent group does not exist");
					return false;
				}
			}

			// Create new group
			Group new_group(group_name);
			new_group.parent_group = parent_group;
			groups[group_name] = new_group;

			// Add to parent's children if parent exists
			if (parent_group.has_value()) {
				groups[parent_group.value()].child_groups.insert(group_name);
			}

			return true;
		}

		bool Service::deleteGroup(std::string const& group_name, bool remove_entities) {
			auto it = groups.find(group_name);
			if (it == groups.end()) {
				PN_CORE_WARN("Group does not exist");
				return false;
			}

			// Handle child groups - move them to parent or root
			for (const auto& child_group_name : it->second.child_groups) {
				auto child_it = groups.find(child_group_name);
				if (child_it != groups.end()) {
					child_it->second.parent_group = it->second.parent_group;

					// Add to new parent's children if exists
					if (it->second.parent_group.has_value()) {
						groups[it->second.parent_group.value()].child_groups.insert(child_group_name);
					}
				}
			}

			// Handle entities in the group
			if (remove_entities) {
				// Destroy all entities in this group
				for (auto entity : it->second.entities) {
					destroyEntity(entity);
				}
			}
			else {
				// Unassign entities from this group
				for (auto entity : it->second.entities) {
					entity_to_group.erase(entity);
				}
			}

			// Remove from parent's children
			if (it->second.parent_group.has_value()) {
				auto parent_it = groups.find(it->second.parent_group.value());
				if (parent_it != groups.end()) {
					parent_it->second.child_groups.erase(group_name);
				}
			}

			// Remove the group
			groups.erase(it);
			return true;
		}

		bool Service::renameGroup(std::string const& old_name, std::string const& new_name) {
			auto it = groups.find(old_name);
			if (it == groups.end()) {
				PN_CORE_WARN("Group does not exist");
				return false;
			}

			// Check if new name already exists
			if (groups.find(new_name) != groups.end()) {
				PN_CORE_WARN("Group with new name already exists");
				return false;
			}

			// Copy group data
			Group group_data = it->second;
			group_data.name = new_name;

			// Update parent's children
			if (group_data.parent_group.has_value()) {
				auto parent_it = groups.find(group_data.parent_group.value());
				if (parent_it != groups.end()) {
					parent_it->second.child_groups.erase(old_name);
					parent_it->second.child_groups.insert(new_name);
				}
			}

			// Update child groups' parent reference
			for (const auto& child_name : group_data.child_groups) {
				auto child_it = groups.find(child_name);
				if (child_it != groups.end()) {
					child_it->second.parent_group = new_name;
				}
			}

			// Update entity-to-group mapping
			for (auto entity : group_data.entities) {
				entity_to_group[entity] = new_name;
			}

			// Remove old group and add new one
			groups.erase(it);
			groups[new_name] = group_data;

			return true;
		}

		bool Service::groupExists(std::string const& group_name) const {
			return groups.find(group_name) != groups.end();
		}

		/********************************
		* Group Hierachy
		***********************************/

		bool Service::setGroupParent(std::string const& group_name, std::optional<std::string> parent_group) {
			auto it = groups.find(group_name);
			if (it == groups.end()) {
				PN_CORE_WARN("Group does not exist");
				return false;
			}

			// Verify parent exists if specified
			if (parent_group.has_value()) {
				if (groups.find(parent_group.value()) == groups.end()) {
					PN_CORE_WARN("Parent group does not exist");
					return false;
				}

				// Prevent circular reference
				if (parent_group.value() == group_name) {
					PN_CORE_WARN("Cannot set group as its own parent");
					return false;
				}
			}

			// Remove from old parent's children
			if (it->second.parent_group.has_value()) {
				auto old_parent_it = groups.find(it->second.parent_group.value());
				if (old_parent_it != groups.end()) {
					old_parent_it->second.child_groups.erase(group_name);
				}
			}

			// Set new parent
			it->second.parent_group = parent_group;

			// Add to new parent's children
			if (parent_group.has_value()) {
				groups[parent_group.value()].child_groups.insert(group_name);
			}

			return true;
		}

		std::optional<std::string> Service::getGroupParent(std::string const& group_name) const {
			auto it = groups.find(group_name);
			if (it == groups.end()) {
				return std::nullopt;
			}

			return it->second.parent_group;
		}

		std::set<std::string> Service::getGroupChildren(std::string const& group_name) const {
			auto it = groups.find(group_name);
			if (it == groups.end()) {
				return std::set<std::string>();
			}

			return it->second.child_groups;
		}

		std::set<std::string> Service::getAllGroups() const {
			std::set<std::string> all_groups;
			for (const auto& [group_name, group_data] : groups) {
				all_groups.insert(group_name);
			}
			return all_groups;
		}

		std::set<std::string> Service::getRootGroups() const {
			std::set<std::string> root_groups;
			for (const auto& [group_name, group_data] : groups) {
				if (!group_data.parent_group.has_value()) {
					root_groups.insert(group_name);
				}
			}
			return root_groups;
		}

		/********************************************
		* Entity Groups
		*******************************************/

		void Service::assignEntityToGroup(ECS::Entity::Type entity, std::string const& group_name) {
			auto entity_it = entities.find(entity);
			if (entity_it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return;
			}

			auto group_it = groups.find(group_name);
			if (group_it == groups.end()) {
				PN_CORE_WARN("Group does not exist");
				return;
			}

			// Remove from old group if assigned
			removeEntityFromGroup(entity);

			// Assign to new group
			entity_it->second.group_id = group_name;
			group_it->second.entities.insert(entity);
			entity_to_group[entity] = group_name;
		}

		void Service::unassignEntityFromGroup(ECS::Entity::Type entity) {
			removeEntityFromGroup(entity);

			auto entity_it = entities.find(entity);
			if (entity_it != entities.end()) {
				entity_it->second.group_id = std::nullopt;
			}
		}

		std::optional<std::string> Service::getEntityGroup(ECS::Entity::Type entity) const {
			auto it = entity_to_group.find(entity);
			if (it == entity_to_group.end()) {
				return std::nullopt;
			}
			return it->second;
		}

		/*****************************************
		* Query entities by group
		*********************************************/

		std::set<ECS::Entity::Type> Service::getEntitiesInGroup(std::string const& group_name, bool recursive) const {
			std::set<ECS::Entity::Type> result;

			auto it = groups.find(group_name);
			if (it == groups.end()) {
				return result;
			}

			// Add entities from this group
			result.insert(it->second.entities.begin(), it->second.entities.end());

			// If recursive, add entities from child groups
			if (recursive) {
				for (const auto& child_group_name : it->second.child_groups) {
					auto child_entities = getEntitiesInGroup(child_group_name, true);
					result.insert(child_entities.begin(), child_entities.end());
				}
			}

			return result;
		}

		/**********************************************
		* Group UI Stuff
		*************************************************/

		void Service::setGroupExpanded(std::string const& group_name, bool expanded) {
			auto it = groups.find(group_name);
			if (it == groups.end()) {
				PN_CORE_WARN("Group does not exist");
				return;
			}

			it->second.expanded = expanded;
		}

		bool Service::isGroupExpanded(std::string const& group_name) const {
			auto it = groups.find(group_name);
			if (it == groups.end()) {
				return false;
			}

			return it->second.expanded;
		}

		/*******************************************
		* Data Access
		******************************************/

		std::optional<EntityData> Service::getEntityDataCopy(ECS::Entity::Type entity) const {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return std::nullopt;
			}

			return it->second;
		}

		EntityData* Service::getEntityDataPtr(ECS::Entity::Type entity) {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				PN_CORE_WARN("Entity does not exist");
				return nullptr;
			}

			return &it->second;
		}

		std::map<ECS::Entity::Type, EntityData, Service::EntitySorter> const& Service::getEntitiesData() const {
			return entities;
		}

		ECS::Entity::Type Service::getFirstEntity() const {
			if (entities.empty()) {
				PN_CORE_WARN("No entities exist");
				return 0;
			}
			return entities.begin()->first;
		}

		size_t Service::getEntityCount() const {
			return entities.size();
		}

		/*****************************
		* Cloning
		******************************/

		void Service::cloneEntityData(ECS::Entity::Type source, ECS::Entity::Type target) {
			auto source_it = entities.find(source);
			if (source_it == entities.end()) {
				PN_CORE_WARN("Source entity does not exist");
				return;
			}

			auto target_it = entities.find(target);
			if (target_it == entities.end()) {
				PN_CORE_WARN("Target entity does not exist");
				return;
			}

			// Clone metadata (but keep target's name)
			std::string target_name = target_it->second.name;
			target_it->second = source_it->second;
			target_it->second.name = target_name;

			// Update entity_names mapping
			entity_names[target_name] = target;
		}

		/*****************************
		* Serialize
		******************************/

		nlohmann::json Service::serializeEntityData(ECS::Entity::Type entity) const {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				return nlohmann::json();
			}

			return it->second.serialize();
		}

		void Service::deserializeEntityData(ECS::Entity::Type entity, nlohmann::json const& data) {
			auto it = entities.find(entity);
			if (it == entities.end()) {
				return;
			}

			it->second.deserialize(data);
		}

		nlohmann::json Service::serializeGroups() const {
			nlohmann::json groups_json = nlohmann::json::array();

			for (const auto& [group_name, group_data] : groups) {
				groups_json.push_back(group_data.serialize());
			}

			return groups_json;
		}

		void Service::deserializeGroups(nlohmann::json const& data) {
			if (!data.is_array()) {
				return;
			}

			groups.clear();

			for (const auto& group_json : data) {
				Group group;
				group.deserialize(group_json);
				groups[group.name] = group;
			}
		}

		nlohmann::json Service::serialize() const {
			return {
				{"Entity_Tags", entity_tags},
				{"Groups", serializeGroups()}
			};
		}

		void Service::deserialize(nlohmann::json const& data) {
			// Deserialize tags
			if (data.contains("Entity_Tags")) {
				for (const auto& tag : data.at("Entity_Tags").items()) {
					entity_tags.insert(tag.value());
				}
			}

			// Deserialize groups
			if (data.contains("Groups")) {
				deserializeGroups(data.at("Groups"));
			}
		}

		/******************************************
		* Reset
		*****************************************/

		void Service::reset() {
			entity_tags.clear();
			entities.clear();
			entity_names.clear();
			ecs_entities.clear();
			entities_to_destroy.clear();
			groups.clear();
			entity_to_group.clear();
		}


	}
}