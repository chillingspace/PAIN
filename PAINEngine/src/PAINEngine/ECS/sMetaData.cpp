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

namespace PAIN {

	void MetaData::Service::init(std::string const& def_entity_name)
	{
		def_name = def_entity_name;
	}

	//void MetaData::Service::onUpdate(AppTiming timing) {
	//	//Empty destroy entities queue
	//	if (!entities_to_destroy.empty()) {
	//		for (auto entity : entities_to_destroy) {
	//			if (NIKE_ECS_MANAGER->checkEntity(entity)) {

	//				//Check if entity has childs and delete them
	//				auto* parent = std::get_if<Parent>(&entities.at(entity).relation);
	//				if (parent && !parent->childrens.empty()) {

	//					//Destroy childrens
	//					for (auto const& child : parent->childrens) {

	//						//Destroy children
	//						auto c_entity = getEntityByName(child);
	//						if (c_entity.has_value()) {
	//							if (NIKE_ECS_MANAGER->checkEntity(c_entity.value())) NIKE_ECS_MANAGER->destroyEntity(c_entity.value());
	//						}
	//					}
	//				}

	//				//Destroy entity
	//				services.get<ECS::Controller>()->
	//			}
	//		}

	//		//Clear entities to destroy
	//		entities_to_destroy.clear();
	//	}
	//}

	void MetaData::Service::onEvent(Event::Event & e) {
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

	void MetaData::Service::updateData()
	{

		//Clear entity names & repopulate them with updated names
		entity_names.clear();

		//Add new entities from the ECS that are not yet in the editor
		for (auto& entity : ecs_entities) {

			//Update entities ref
			if (entities.find(entity) == entities.end()) {

				//Create identifier for entity
				char entity_name[32];
				snprintf(entity_name, sizeof(entity_name), (def_name + "%04d").data(), entity);
				entities[entity].name = entity_name;

				//Set a proper layer ID
				setEntityLayerID(entity, 0);
			}
			else if (entities.at(entity).name.find(def_name) != std::string::npos) {

				//Create identifier for entity
				char entity_name[32];
				snprintf(entity_name, sizeof(entity_name), (def_name + "%04d").data(), entity);
				entities[entity].name = entity_name;
			}

			//Populate entity name
			entity_names[entities[entity].name] = entity;
		}
	}
}