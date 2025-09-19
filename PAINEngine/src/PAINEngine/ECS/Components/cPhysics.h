/*****************************************************************//**
 * \file   cPhysics.h
 * \brief  All physics data components
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#ifndef C_PYSHICS_H
#define C_PYSHICS_H

 #include "Jolt/Jolt.h"
 #include <Jolt/Core/Factory.h>          
 #include <Jolt/RegisterTypes.h>         
 #include <Jolt/Physics/PhysicsSystem.h> 
 #include <Jolt/Physics/Body/Body.h>     
 #include <Jolt/Core/TempAllocator.h>
 #include <Jolt/Core/JobSystemThreadPool.h> 
 #include <Jolt/Physics/Collision/ObjectLayer.h>
 #include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace PAIN {

	/*****************************************************************//**
	* Physics Components
	*********************************************************************/
	namespace Physics {

		struct RigidBody3D {
			glm::f32vec3 velocity;
			glm::f32vec3 angular_velocity;
			glm::f32 mass;
			JPH::BodyID bodyID;
			bool b_is_dynamic;
		};
	}

	/*****************************************************************//**
	* Collision Components
	*********************************************************************/

	enum class SHAPE { Box, Sphere, Capsule, Mesh };

	namespace Collision {
		struct Collider {

			// Default to be box
			SHAPE shape = SHAPE::Box;

			// To save memory, only one member is valid at a time, rest would be garbage values if un init. 
			// Set the shape type first, then set the collidor respective sizes. 
			union
			{
				glm::vec3 box_size;     
				glm::f32 sphere_radius;
				struct { glm::f32 radius; glm::f32 height; } capsule;
			};

			// Optional physical props (future)
			glm::f32 friction = 0.5f;
			glm::f32 restitution = 0.1f;

			// Jolt object layer
			uint16_t collision_layer = 0; 
			bool is_trigger = false;

		};
	}


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

#endif
