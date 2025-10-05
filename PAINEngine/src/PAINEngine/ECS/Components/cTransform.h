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

#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h"

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

#ifdef _DEBUG
	// UI Registration function
	inline void RegisterTransformUI(Editor::Panel::ComponentsPanel& panel) {
		panel.registerCompUIFunc<Transform>([](Editor::Panel::ComponentsPanel& comp_panel, Transform& transform) {
			ImGui::Text("Transform");
			ImGui::Separator();

			// Position - use glm::value_ptr to get float pointer
			ImGui::DragFloat3("Position", glm::value_ptr(transform.position), 0.1f);

			// Rotation - convert quaternion to Euler angles for editing
			glm::vec3 euler = glm::degrees(glm::eulerAngles(transform.rotation));
			if (ImGui::DragFloat3("Rotation", glm::value_ptr(euler), 1.0f)) {
				// Convert back to quaternion
				transform.rotation = glm::quat(glm::radians(euler));
			}

			// Scale - use glm::value_ptr to get float pointer
			ImGui::DragFloat3("Scale", glm::value_ptr(transform.scale), 0.1f);
		});
	}
	
#endif

}

#endif
