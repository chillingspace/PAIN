/*****************************************************************/ /**
                                                                     * \file
                                                                     * cCompoundCollider.h
                                                                     * \brief
                                                                     * Compound
                                                                     * collider
                                                                     * component
                                                                     * for
                                                                     * custom
                                                                     * multi-shape
                                                                     * collision
                                                                     *
                                                                     * \date
                                                                     * January
                                                                     * 2026 All
                                                                     * content
                                                                     * DigiPen
                                                                     * Institute
                                                                     * of
                                                                     * Technology
                                                                     * Singapore,
                                                                     * all
                                                                     * rights
                                                                     * reserved.
                                                                     *********************************************************************/

#pragma once

#ifndef C_COMPOUND_COLLIDER_H
#define C_COMPOUND_COLLIDER_H

#include "GLMSerialization.h"
#include "pch.h"


namespace PAIN {

// Shape types for compound collider sub-shapes
enum class ColliderShapeType { Box = 0, Sphere = 1, Capsule = 2 };

// Individual sub-collider definition
struct ColliderShape {
  ColliderShapeType type = ColliderShapeType::Box;
  glm::vec3 offset = glm::vec3(0.0f); // Position relative to entity origin
  glm::quat rotation = glm::quat(1, 0, 0, 0); // Local rotation

  // Box parameters (used when type == Box)
  glm::vec3 boxHalfExtents = glm::vec3(0.5f);

  // Sphere parameters (used when type == Sphere)
  float sphereRadius = 0.5f;

  // Capsule parameters (used when type == Capsule)
  float capsuleRadius = 0.25f;
  float capsuleHalfHeight = 0.5f;

  // Default constructor
  ColliderShape() = default;

  // Box constructor
  static ColliderShape CreateBox(const glm::vec3 &halfExtents,
                                 const glm::vec3 &off = glm::vec3(0.0f)) {
    ColliderShape shape;
    shape.type = ColliderShapeType::Box;
    shape.boxHalfExtents = halfExtents;
    shape.offset = off;
    return shape;
  }

  // Sphere constructor
  static ColliderShape CreateSphere(float radius,
                                    const glm::vec3 &off = glm::vec3(0.0f)) {
    ColliderShape shape;
    shape.type = ColliderShapeType::Sphere;
    shape.sphereRadius = radius;
    shape.offset = off;
    return shape;
  }

  // Capsule constructor
  static ColliderShape CreateCapsule(float radius, float halfHeight,
                                     const glm::vec3 &off = glm::vec3(0.0f)) {
    ColliderShape shape;
    shape.type = ColliderShapeType::Capsule;
    shape.capsuleRadius = radius;
    shape.capsuleHalfHeight = halfHeight;
    shape.offset = off;
    return shape;
  }
};

// Component: Compound collider containing multiple sub-shapes
struct CompoundCollider {
  bool useCompoundCollider = false;  // Toggle between compound and default AABB
  std::vector<ColliderShape> shapes; // List of sub-colliders

  // Serialization flag
  static constexpr bool ShouldSerialize = true;

  // Helper to add shapes
  void addBox(const glm::vec3 &halfExtents,
              const glm::vec3 &off = glm::vec3(0.0f)) {
    shapes.push_back(ColliderShape::CreateBox(halfExtents, off));
  }

  void addSphere(float radius, const glm::vec3 &off = glm::vec3(0.0f)) {
    shapes.push_back(ColliderShape::CreateSphere(radius, off));
  }

  void addCapsule(float radius, float halfHeight,
                  const glm::vec3 &off = glm::vec3(0.0f)) {
    shapes.push_back(ColliderShape::CreateCapsule(radius, halfHeight, off));
  }

  void clear() { shapes.clear(); }
};

} // namespace PAIN

// Reflection for ColliderShape
REFL_TYPE(PAIN::ColliderShape)
REFL_FIELD(type)
REFL_FIELD(offset)
REFL_FIELD(rotation)
REFL_FIELD(boxHalfExtents)
REFL_FIELD(sphereRadius)
REFL_FIELD(capsuleRadius)
REFL_FIELD(capsuleHalfHeight)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::ColliderShape>);

// Reflection for CompoundCollider
REFL_TYPE(PAIN::CompoundCollider)
REFL_FIELD(useCompoundCollider)
REFL_FIELD(shapes)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::CompoundCollider>);

#endif // C_COMPOUND_COLLIDER_H
