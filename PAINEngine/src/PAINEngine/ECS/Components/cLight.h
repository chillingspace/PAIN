#pragma once

#include "pch.h"
#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h"
#include "GLMSerialization.h"

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
		glm::vec3 offset;
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
            ImGui::Text("Offset");
            ImGui::DragFloat3("Offset", &light.offset.x, 0.1f);

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

NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::SHADOW_TYPES, {
{PAIN::SHADOW_TYPES::NONE, "none"},
{PAIN::SHADOW_TYPES::MAPPED, "mapped"},
{PAIN::SHADOW_TYPES::SCREEN_SPACE, "screen_space"}
    })

NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::TYPES, {
        {PAIN::TYPES::POINT, "point"},
        {PAIN::TYPES::DIRECTIONAL, "directional"},
        {PAIN::TYPES::SPOTLIGHT, "spotlight"}
    })

// This is needed as json still does not now how to handle seri for the custom comps,
// These types not supported by refl, so we need add struct-level seri 
namespace nlohmann {
template<>
    struct adl_serializer<PAIN::Lighting> {
        static void to_json(json& j, const PAIN::Lighting& light) {
            j["position"] = light.offset;
            j["light_intensity"] = light.light_intensity;
            j["light_type"] = light.light_type;
            j["forward"] = light.forward;
            j["shadow_type"] = light.shadow_type;
        }

        static void from_json(const json& j, PAIN::Lighting& light) {
            light.offset = j["offset"].get<glm::vec3>();
            light.light_intensity = j["light_intensity"].get<glm::vec3>();
            light.light_type = j["light_type"].get<PAIN::TYPES>();
            light.forward = j["forward"].get<glm::vec3>();
            light.shadow_type = j["shadow_type"].get<PAIN::SHADOW_TYPES>();
        }
    };
}

// Reflection
REFL_TYPE(PAIN::Lighting)
REFL_FIELD(offset)
REFL_FIELD(light_intensity)
REFL_FIELD(light_type)
REFL_FIELD(forward)
REFL_FIELD(shadow_type)
REFL_END