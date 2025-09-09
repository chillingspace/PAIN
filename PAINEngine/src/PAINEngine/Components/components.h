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
	struct transform {
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale{1, 1, 1};
	};

	/*****************************************************************//**
	* Physics Components
	*********************************************************************/

	struct rigidBody3D {
		float mass;
		bool b_is_dynamic;
		glm::vec3 velocity;
		glm::vec3 angular_velocity;
		JPH::BodyID bodyID;
	};

	enum class Joint_Type {
		FIXED,
		HINGE
	};

	struct joint {
		Joint_Type joint_type;
		// Entity::Type connectedEntity;
		// for hinge stuff
		glm::vec3 anchor;
		glm::vec3 axis; 
		// For hinge limits
		float limit_min;
		float limit_max; 
	};
}



