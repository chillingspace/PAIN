/*****************************************************************//**
 * \file   cPhysics.h
 * \brief  All physics data components
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "Core/pch.h"

namespace PAIN {

	/*****************************************************************//**
	* Physics Components
	*********************************************************************/

	struct RigidBody3D {
		glm::f32vec3 velocity;
		glm::f32vec3 angular_velocity;
		glm::f32 mass;
		JPH::BodyID bodyID;
		bool b_is_dynamic;
	};

	struct CollisionInfo {
		// Entity::Type otherEntity;
		glm::f32vec3 contact_point;
		glm::f32vec3 normal;
		glm::f32 penetration;
	};

	enum class SHAPE { Box, Sphere, Capsule, Mesh };

	struct Collider {

		// For Box/Capsule
		glm::f32vec3 size;
		// For Sphere
		glm::i32 radius{ 0.0f };

		glm::i32 collision_layer = 0;

		SHAPE shape;

		// For pickup and stuff (Like unity's pickup item)
		bool b_is_trigger = false;

	};

	// To be changed to use MAX_ENTITY, to be put in collision manager, not wise for each entity to carry so much bytes
	std::array<CollisionInfo, 100> current_collisions;


	enum class JOINT_TYPE { FIXED,HINGE
	};

	struct Joint {
		// For hinge points
		glm::f32vec3 anchor;
		glm::f32vec3 axis;
		// For hinge limits
		glm::f32 limit_min;
		glm::f32 limit_max;
		JOINT_TYPE joint_type;
		// Entity::Type connectedEntity;
	};
}