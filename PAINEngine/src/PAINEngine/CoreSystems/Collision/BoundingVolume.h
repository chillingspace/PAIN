#pragma once

#ifndef BOUNDING_VOLUME_H
#define BOUNDING_VOLUME_H

#include "pch.h" // Include pch first
#include <limits> // Required for numeric_limits

// Undefine min/max macros JUST FOR THIS FILE
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace PAIN {

    // Axis-Aligned Bounding Box
    struct AABB {
        // Use parentheses around std::numeric_limits calls to prevent macro expansion
        glm::vec3 min = glm::vec3((std::numeric_limits<float>::max)());
        glm::vec3 max = glm::vec3((std::numeric_limits<float>::lowest)());

        AABB() = default;

        // Constructor initializer list uses member names directly
        AABB(const glm::vec3& pMin, const glm::vec3& pMax) : min(pMin), max(pMax) {}

        // Expand the AABB to include a point
        void expand(const glm::vec3& point) {
            // Use glm::min/max which are safe from macros
            min = glm::min(min, point);
            max = glm::max(max, point);
        }

        // Expand the AABB to include another AABB
        void expand(const AABB& other) {
            min = glm::min(min, other.min);
            max = glm::max(max, other.max);
        }

        // Calculate the center of the AABB
        glm::vec3 getCenter() const {
            return (min + max) * 0.5f;
        }

        // Calculate the extents (half-dimensions) of the AABB
        glm::vec3 getExtents() const {
            return (max - min) * 0.5f;
        }

        // Calculate the surface area of the AABB
        float getSurfaceArea() const {
            glm::vec3 diff = max - min;
            // Handle cases where AABB might be degenerate (a plane or line)
            // Use a small epsilon for floating point comparison
            const float epsilon = 1e-6f;
            if (diff.x <= epsilon || diff.y <= epsilon || diff.z <= epsilon) {
                 if (diff.x > epsilon && diff.y > epsilon) return 2.0f * (diff.x * diff.y);
                 if (diff.x > epsilon && diff.z > epsilon) return 2.0f * (diff.x * diff.z);
                 if (diff.y > epsilon && diff.z > epsilon) return 2.0f * (diff.y * diff.z);
                 return 0.0f; // It's effectively a line or point
            }
            return 2.0f * (diff.x * diff.y + diff.x * diff.z + diff.y * diff.z);
        }

        // Check if this AABB intersects with another AABB
        bool intersects(const AABB& other) const {
            if (max.x < other.min.x || min.x > other.max.x) return false;
            if (max.y < other.min.y || min.y > other.max.y) return false;
            if (max.z < other.min.z || min.z > other.max.z) return false;
            return true; // Overlapping on all axes
        }

        // Transform the AABB by a matrix
        AABB transform(const glm::mat4& matrix) const {
            // Check for uninitialized or degenerate AABB before transforming corners
            if (min.x > max.x || min.y > max.y || min.z > max.z) {
                 // Return an AABB that reflects this invalid state if needed, or a default one
                 return AABB(); // Default constructor gives max/lowest bounds
            }
            glm::vec3 corners[8] = {
                glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z),
                glm::vec3(min.x, max.y, min.z), glm::vec3(min.x, min.y, max.z),
                glm::vec3(max.x, max.y, min.z), glm::vec3(min.x, max.y, max.z),
                glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z)
            };
            AABB transformed_aabb; // Initializes to max/lowest
            for (int i = 0; i < 8; ++i) {
                glm::vec4 transformed_corner = matrix * glm::vec4(corners[i], 1.0f);
                transformed_aabb.expand(glm::vec3(transformed_corner)); // Expand using vec3 part
            }
            return transformed_aabb;
        }

        // Merge two AABBs
        static AABB merge(const AABB& a, const AABB& b) {
            AABB result;
            result.min = glm::min(a.min, b.min); // Use glm::min
            result.max = glm::max(a.max, b.max); // Use glm::max
            return result;
        }
    };

} // namespace PAIN

#endif // BOUNDING_VOLUME_H