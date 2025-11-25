#pragma once

#include "pch.h"


namespace PAIN {

    struct Cam {
        glm::vec3 trans_offset{ 0.f, 0.f, 0.f };
        glm::vec3 rot_offset{ 0.f, 0.f, 0.f };
        glm::vec3 scale_offset{ 0.f, 0.f, 0.f };
        //glm::vec3 forward{ -glm::normalize(pos) };
        //glm::vec3 up{ 0.f, 1.f, 0.f };

        float near_plane{ 0.1f };
        float far_plane{ 100.f };
        float width_ratio{ 16.f };
        float height_ratio{ 9.f };
    };
}
// Reflection
REFL_TYPE(PAIN::Cam)
REFL_FIELD(trans_offset)
REFL_FIELD(rot_offset)
REFL_FIELD(scale_offset)
REFL_FIELD(near_plane)
REFL_FIELD(far_plane)
REFL_FIELD(width_ratio)
REFL_FIELD(height_ratio)
REFL_END
