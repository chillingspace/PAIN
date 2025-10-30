#pragma once

#ifndef BVH_H
#define BVH_H

// Include dependencies outside namespace
#include <vector>
#include <utility> // For std::pair
#include "CoreSystems/Collision/BVHNode.h" // Includes BVHNode which includes AABB, entt::entity

namespace PAIN {

#include "pch.h" // Include pch inside namespace

// Bounding Volume Hierarchy Tree implementation
class BVH {
public:
    BVH(size_t initialCapacity = 128);
    ~BVH() = default;

    void build(const std::vector<std::pair<entt::entity, AABB>>& items);

    const std::vector<BVHNode>& getNodes() const { return m_nodes; }
    int getRootIndex() const { return m_rootIndex; }
    bool isEmpty() const { return m_rootIndex == -1; }

private:
    std::vector<BVHNode> m_nodes;
    int m_rootIndex = -1;
    int m_nodeCount = 0;
    int m_nodeCapacity = 0;
    int m_freeListIndex = -1;

    int allocateNode();
    void freeNode(int nodeIndex);
    int buildRecursive(std::vector<std::pair<entt::entity, AABB>>& items, int start, int end);
    void computeAABB(int nodeIndex);
};

} // namespace PAIN

#endif // BVH_H