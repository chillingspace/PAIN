#pragma once

#ifndef BVH_NODE_H
#define BVH_NODE_H

// Include dependencies outside namespace
#include "CoreSystems/Collision/BoundingVolume.h" // Includes AABB
#include "entt/entt.hpp" // For entt::entity

namespace PAIN {

#include "pch.h" // Include pch inside namespace if needed by AABB or other PAIN types

// Represents a node within the Bounding Volume Hierarchy tree
struct BVHNode {
    AABB aabb;
    int parentIndex = -1;
    int child1Index = -1;
    int child2Index = -1;
    entt::entity entity = entt::null;
    int height = -1;

    // Returns true if this node is a leaf node (has no children).
    bool isLeaf() const {
        return child1Index == -1;
    }
};

} // namespace PAIN

#endif // BVH_NODE_H