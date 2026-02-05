#pragma once

#include "pch.h"
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

        glm::vec3 direction = { 0 , -1, 0 }; // Direction the light points, relevant for non - point lights (previously named forward)
        // float fov; 
        //	float aspect_ratio;
        //	float near_plane;
        //	float far_plane;
        SHADOW_TYPES shadow_type;

        //spotlight
        float inner_angle = 12.5f;
        float outer_angle = 17.5f;

        static constexpr const char* LightTypeNames[] = { "Point", "Directional", "Spotlight" };
        static constexpr const char* ShadowTypeNames[] = { "None", "Mapped", "Screen Space" };

        //Serialization flag
        static constexpr bool ShouldSerialize = true;
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


// Reflection
REFL_TYPE(PAIN::Lighting)
REFL_FIELD(offset)
REFL_FIELD(light_intensity)
REFL_FIELD(light_type)
REFL_FIELD(shadow_type)
REFL_FIELD(direction)
REFL_FIELD(inner_angle)
REFL_FIELD(outer_angle)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Lighting>);