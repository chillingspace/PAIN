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

        // ============================================
        // Camera Collision Settings
        // ============================================
        bool collisionEnabled = false;          // Toggle collision on/off
        float collisionRadius = 0.5f;           // Sphere/capsule radius
        float collisionOffset = 0.1f;           // Minimum distance from surfaces
        float capsuleHeight = 1.8f;             // Height for capsule collision
        bool useCapsuleCollision = false;       // false = sphere, true = capsule
        bool showCollisionGizmo = false;        // Show collision visualization in editor

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
REFL_FIELD(collisionEnabled)
REFL_FIELD(collisionRadius)
REFL_FIELD(collisionOffset)
REFL_FIELD(capsuleHeight)
REFL_FIELD(useCapsuleCollision)
REFL_FIELD(showCollisionGizmo)
REFL_END
