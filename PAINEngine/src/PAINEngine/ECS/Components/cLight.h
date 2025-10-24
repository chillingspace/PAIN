#pragma once

#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h"

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/
    enum class SHADOW_TYPES {
        NONE,
        MAPPED,			// expensive
        SCREEN_SPACE,
        NUM_SHADOW_TYPES,
    };

    enum class TYPES {
        POINT,
        DIRECTIONAL,
        SPOTLIGHT,
        NUM_TYPES,
    };

	struct Lighting {
		glm::vec3 position;
		glm::vec3 light_intensity;
		TYPES light_type;

		glm::vec3 forward; // Direction the light points, relevant for non - point lights
		// float fov; 
		//	float aspect_ratio;
		//	float near_plane;
		//	float far_plane;
		SHADOW_TYPES shadow_type;
	};

#ifdef _DEBUG
	// UI Registration function
	inline void RegisterLightUI(Editor::Panel::ComponentsPanel& panel) {
		panel.registerCompUIFunc<Lighting>([](Editor::Panel::ComponentsPanel& comp_panel, Lighting& light) {
            ImGui::Text("Lighting");
            ImGui::Separator();

            // Position
            ImGui::Text("Position");
            ImGui::DragFloat3("Position", &light.position.x, 0.1f);

            // Light Intensity
            ImGui::Text("Light Intensity");
            ImGui::ColorEdit3("Intensity", &light.light_intensity.x);

            // Light Type
            const char* types[] = { "Point", "Directional", "Spotlight" };
            int current_type = static_cast<int>(light.light_type);
            if (ImGui::BeginCombo("Type", types[current_type])) {
                for (int n = 0; n < IM_ARRAYSIZE(types); n++) {
                    bool is_selected = (current_type == n);
                    if (ImGui::Selectable(types[n], is_selected)) {
                        current_type = n;
                        light.light_type = static_cast<TYPES>(n);
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // Forward (only relevant for non-point lights)
            if (light.light_type != TYPES::POINT) {
                ImGui::Text("Forward Direction");
                ImGui::DragFloat3("Forward", &light.forward.x, 0.1f);
                if (ImGui::IsItemDeactivatedAfterEdit()) {
                    light.forward = glm::normalize(light.forward);
                }
            }

            // Shadow Type
            const char* shadow_types[] = { "None", "Mapped", "Screen Space" };
            int current_shadow = static_cast<int>(light.shadow_type);
            if (ImGui::BeginCombo("Shadow Type", shadow_types[current_shadow])) {
                for (int n = 0; n < IM_ARRAYSIZE(shadow_types); n++) {
                    bool is_selected = (current_shadow == n);
                    if (ImGui::Selectable(shadow_types[n], is_selected)) {
                        current_shadow = n;
                        light.shadow_type = static_cast<SHADOW_TYPES>(n);
                    }
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();
			});

	}

#endif

}

