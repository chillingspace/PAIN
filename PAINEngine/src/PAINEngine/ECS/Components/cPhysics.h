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

#include "pch.h"

 #include "Jolt/Jolt.h"
 #include <Jolt/Core/Factory.h>          
 #include <Jolt/RegisterTypes.h>         
 #include <Jolt/Physics/PhysicsSystem.h> 
 #include <Jolt/Physics/Body/Body.h>     
 #include <Jolt/Core/TempAllocator.h>
 #include <Jolt/Core/JobSystemThreadPool.h> 
 #include <Jolt/Physics/Collision/ObjectLayer.h>
 #include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
 //#include "LayeredSystems/LevelEditor/Panels/ReflectionUI.h" 
 #include "GLMSerialization.h"
 #include <refl.hpp>

namespace PAIN {

	/*****************************************************************//**
	* Physics Components
	*********************************************************************/
	namespace Physics {

		struct RigidBody3D {
			glm::f32vec3 velocity;
			glm::f32vec3 angular_velocity;
			glm::f32 mass;
			JPH::BodyID bodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
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

// Enum Serializers 
NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::SHAPE, {
	{PAIN::SHAPE::Box, "box"},
	{PAIN::SHAPE::Sphere, "sphere"},
	{PAIN::SHAPE::Capsule, "capsule"},
	{PAIN::SHAPE::Mesh, "mesh"}
	})

NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::JOINT_TYPE, {
	{PAIN::JOINT_TYPE::FIXED, "fixed"},
	{PAIN::JOINT_TYPE::HINGE, "hinge"}
	})

	namespace nlohmann {
    // RigidBody3D serializer
    template<>
    struct adl_serializer<PAIN::Physics::RigidBody3D> {
        static void to_json(json& j, const PAIN::Physics::RigidBody3D& rb) {
			// There is no need to serialize BodyID directly, as it is managed by Jolt internally. A new one will be created upon loading
            j["velocity"] = rb.velocity;
            j["angular_velocity"] = rb.angular_velocity;
            j["mass"] = rb.mass;
            j["is_dynamic"] = rb.b_is_dynamic;
        }
        
        static void from_json(const json& j, PAIN::Physics::RigidBody3D& rb) {
			// Likewise, no need to read BodyID here as Jolt will create a new one
            rb.velocity = j["velocity"].get<glm::vec3>();
            rb.angular_velocity = j["angular_velocity"].get<glm::vec3>();
            rb.mass = j["mass"].get<float>();
            rb.b_is_dynamic = j["is_dynamic"].get<bool>();
        }
    };

    // Collider serializer (handles union carefully)
    template<>
    struct adl_serializer<PAIN::Collision::Collider> {
        static void to_json(json& j, const PAIN::Collision::Collider& col) {
            j["shape"] = col.shape;
            
            // Serialize union based on shape type
            switch (col.shape) {
                case PAIN::SHAPE::Box:
                    j["box_size"] = col.box_size;
                    break;
                case PAIN::SHAPE::Sphere:
                    j["sphere_radius"] = col.sphere_radius;
                    break;
                case PAIN::SHAPE::Capsule:
                    j["capsule_radius"] = col.capsule.radius;
                    j["capsule_height"] = col.capsule.height;
                    break;
                case PAIN::SHAPE::Mesh:
                    // Mesh might not need shape data
                    break;
            }
            
            j["friction"] = col.friction;
            j["restitution"] = col.restitution;
            j["collision_layer"] = col.collision_layer;
            j["is_trigger"] = col.is_trigger;
        }
        
        static void from_json(const json& j, PAIN::Collision::Collider& col) {
            col.shape = j["shape"].get<PAIN::SHAPE>();
            
            // Deserialize union based on shape type
            switch (col.shape) {
                case PAIN::SHAPE::Box:
                    col.box_size = j["box_size"].get<glm::vec3>();
                    break;
                case PAIN::SHAPE::Sphere:
                    col.sphere_radius = j["sphere_radius"].get<float>();
                    break;
                case PAIN::SHAPE::Capsule:
                    col.capsule.radius = j["capsule_radius"].get<float>();
                    col.capsule.height = j["capsule_height"].get<float>();
                    break;
                case PAIN::SHAPE::Mesh:
                    break;
            }
            
            col.friction = j["friction"].get<float>();
            col.restitution = j["restitution"].get<float>();
            col.collision_layer = j["collision_layer"].get<uint16_t>();
            col.is_trigger = j["is_trigger"].get<bool>();
        }
    };

    // Joint serializer
    template<>
    struct adl_serializer<PAIN::Joint> {
        static void to_json(json& j, const PAIN::Joint& joint) {
            j["anchor"] = joint.anchor;
            j["axis"] = joint.axis;
            j["limit_min"] = joint.limit_min;
            j["limit_max"] = joint.limit_max;
            j["joint_type"] = joint.joint_type;
        }
        
        static void from_json(const json& j, PAIN::Joint& joint) {
            joint.anchor = j["anchor"].get<glm::vec3>();
            joint.axis = j["axis"].get<glm::vec3>();
            joint.limit_min = j["limit_min"].get<float>();
            joint.limit_max = j["limit_max"].get<float>();
            joint.joint_type = j["joint_type"].get<PAIN::JOINT_TYPE>();
        }
    };
}

// Reflections
// Joint
REFL_TYPE(PAIN::Joint)
REFL_FIELD(anchor)
REFL_FIELD(axis)
REFL_FIELD(limit_min)
REFL_FIELD(limit_max)
REFL_FIELD(joint_type)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Joint>);

// Rigid Body 3D
REFL_TYPE(PAIN::Physics::RigidBody3D)
REFL_FIELD(velocity)
REFL_FIELD(angular_velocity)
REFL_FIELD(mass)
//REFL_FIELD(bodyID)   // Uneditable; Uncommenting this causes issues with the serialization
REFL_FIELD(b_is_dynamic)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Physics::RigidBody3D>);

#endif
