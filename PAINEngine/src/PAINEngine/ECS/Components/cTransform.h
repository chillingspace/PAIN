/*****************************************************************//**
 * \file   components.h
 * \brief  All data components
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author 
 * \date   September 2025
 * All content � 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#ifndef C_TRANSFORM_H
#define C_TRANSFORM_H

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/

	struct Transform {
		glm::f32vec3 position{ 0 ,0 ,0 };
		glm::f32quat rotation;
		glm::f32vec3 scale{ 1, 1, 1 };

		glm::mat4 getMatrix() const {
			glm::mat4 T = glm::translate(glm::mat4(1.0f), position);
			glm::mat4 R = glm::mat4_cast(rotation);
			glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);
			return T * R * S;
		}
	};
	


}

#endif
