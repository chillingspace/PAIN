/*****************************************************************//**
 * \file   sCollision.h
 * \brief  Declaration of collision service
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once
#ifndef COLLISION_SERVICE_H
#define COLLISION_SERVICE_H

#include "PAINEngine/Components/cTransform.h"
#include "PAINEngine/Components/cPhysics.h"

namespace PAIN {
	namespace Collision {

		struct CollisionInfo {
			// Entity::Type otherEntity;
			glm::f32vec3 contact_point;
			glm::f32vec3 normal;
			glm::f32 penetration;
			bool b_is_trigger;
		};

		class Service
		{
		public:

			// temp cap, it will then be MAX_ENTITY of such
			static constexpr size_t MAX_COLLISIONS = 100; 

			Service() = default;
			~Service() = default;

			void clearPreviousCollisions();

		private:
			// To be changed to use MAX_ENTITY, to be put in collision manager, not wise for each entity to carry so much bytes
			std::array<CollisionInfo, 100> current_collisions;
		};
	}
}

#endif