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
//namespace nlohmann {
//template<>
//    struct adl_serializer<PAIN::Lighting> {
//        static void to_json(json& j, const PAIN::Lighting& light) {
//            j["position"] = light.offset;
//            j["light_intensity"] = light.light_intensity;
//            j["light_type"] = light.light_type;
//            j["forward"] = light.forward;
//            j["shadow_type"] = light.shadow_type;
//        }
//
//        static void from_json(const json& j, PAIN::Lighting& light) {
//            light.offset = j["offset"].get<glm::vec3>();
//            light.light_intensity = j["light_intensity"].get<glm::vec3>();
//            light.light_type = j["light_type"].get<PAIN::TYPES>();
//            light.forward = j["forward"].get<glm::vec3>();
//            light.shadow_type = j["shadow_type"].get<PAIN::SHADOW_TYPES>();
//        }
//    };
//}

// Reflection
REFL_TYPE(PAIN::Lighting)
REFL_FIELD(offset)
REFL_FIELD(light_intensity)
REFL_FIELD(light_type)
REFL_FIELD(forward)
REFL_FIELD(shadow_type)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Lighting>);