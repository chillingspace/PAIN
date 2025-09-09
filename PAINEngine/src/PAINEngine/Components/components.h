/*****************************************************************//**
 * \file   components.h
 * \brief  All data components
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author 
 * \date   September 2025
 * All content © 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "../pch.h"

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	*****************************************************************************************/

	struct Transform {
		glm::f64vec3 position;
		glm::f64quat rotation;
		glm::f64vec3 scale{1, 1, 1};
	};

	/*****************************************************************//**
	* Physics Components
	*********************************************************************/

	struct RigidBody3D {
		std::int64_t mass;
		bool b_is_dynamic;
		glm::f64vec3 velocity;
		glm::f64vec3 angular_velocity;
		JPH::BodyID bodyID;
	};

	struct CollisionInfo {
		// Entity::Type otherEntity;
		glm::f64vec3 contact_point;
		glm::f64vec3 normal;
		std::int64_t penetration = 0.1f;
	};

	enum class SHAPE { Box, Sphere, Capsule, Mesh };

	struct ColliderComponent {
		SHAPE shape;
		// For Box/Capsule
		glm::f64vec3 size;           
		// For Sphere
		std::int64_t radius{ 0.0f };    

		int collision_layer = 0; 

		// For pickup and stuff (Like unity's pickup item)
		bool b_is_trigger = false; 

		// Filled every physics step by CollisionManager
		// To be changed to use MAX_ENTITY
		std::array<CollisionInfo, 100> current_collisions;
	};


	enum class JOINT_TYPE {
		FIXED,
		HINGE
	};

	struct joint {
		JOINT_TYPE joint_type;
		// Entity::Type connectedEntity;
		// for hinge stuff
		glm::f64vec3 anchor;
		glm::f64vec3 axis; 
		// For hinge limits
		glm::f64 limit_min;
		glm::f64 limit_max; 
	};
}



