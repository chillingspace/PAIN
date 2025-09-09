/*****************************************************************//**
 * \file   components.h
 * \brief  All data components
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author 
 * \date   September 2025
 * All content � 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "../pch.h"

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/

	struct Transform {
		glm::f64quat rotation;
		glm::f64vec3 position;
		glm::f64vec3 scale{1, 1, 1};
	};
}



