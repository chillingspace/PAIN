/*****************************************************************//**
 * \file   mEntity.h
 * \brief  Entity manager for ECS architecture
 *
 * \author Ho Shu Hng, 2301339, shuhng.ho@digipen.edu (100%)
 * \date   September 2024
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/
#pragma once

#ifndef M_ENTITY_HPP
#define M_ENTITY_HPP

#include "../ECSTypes.h"

namespace PAIN {
	namespace ECS {
		namespace Entity {

			//Entity Management
			class Service {
			private:

				//Delete Copy Constructor & Copy Assignment
				Service(Service const& copy) = delete;
				void operator=(Service const& copy) = delete;

				//Map of create entities and their component signatures
				std::unordered_map<PAIN::ECS::Entity::Type, PAIN::ECS::Component::Signature> entities;

				//Entity indexes waiting to be used
				std::queue<PAIN::ECS::Entity::Type> avail_entities;

			public:

				//Default constructor
				Service();

				//Create Entity
				PAIN::ECS::Entity::Type createEntity();

				//Destroy Entity
				void destroyEntity(PAIN::ECS::Entity::Type entity);

				//Check entity is present
				bool checkEntity(PAIN::ECS::Entity::Type entity) const;

				//Set signature
				void setSignature(PAIN::ECS::Entity::Type entity, PAIN::ECS::Component::Signature signature);

				//Get signature
				PAIN::ECS::Component::Signature const& getSignature(PAIN::ECS::Entity::Type entity) const;

				//Get entity component count
				int getEntityComponentCount(PAIN::ECS::Entity::Type entity) const;

				//Get number of active entities
				int getEntitiesCount() const;

				//Get all active entity
				std::set<PAIN::ECS::Entity::Type> getAllEntities() const;
			};
		}
	}
}

#endif //!M_ENTITY_HPP