#include "pch.h" // Must be first

// Include BVH header after pch
#include "CoreSystems/Collision/BVH.h"

// Other necessary standard library includes
#include <algorithm> // for std::sort, std::max
#include <limits>    // for numeric_limits (needed by AABB init via BVHNode)

// Ensure code is within the namespace
namespace PAIN {

    BVH::BVH(size_t initialCapacity) {
        m_nodeCapacity = static_cast<int>(initialCapacity);
        m_nodes.resize(m_nodeCapacity);

        // Initialize the free list
        for (int i = 0; i < m_nodeCapacity - 1; ++i) {
            m_nodes[i].child1Index = i + 1; // Use child1 as 'next' pointer
            m_nodes[i].height = -1;
        }
        m_nodes[m_nodeCapacity - 1].child1Index = -1; // End of list marker
        m_nodes[m_nodeCapacity - 1].height = -1;
        m_freeListIndex = 0;
        m_nodeCount = 0;
        m_rootIndex = -1;
    }

    int BVH::allocateNode() {
        if (m_freeListIndex == -1) { // Free list empty, expand storage
            int oldCapacity = m_nodeCapacity;
            m_nodeCapacity *= 2;
            m_nodes.resize(m_nodeCapacity);

            // Link new nodes into the free list
            for (int i = oldCapacity; i < m_nodeCapacity - 1; ++i) {
                m_nodes[i].child1Index = i + 1;
                m_nodes[i].height = -1;
            }
            m_nodes[m_nodeCapacity - 1].child1Index = -1; // End of new segment
            m_freeListIndex = oldCapacity; // Point to start of new segment
        }

        // Get node from front of free list
        int nodeIndex = m_freeListIndex;
        m_freeListIndex = m_nodes[nodeIndex].child1Index; // Update head

        // Initialize the allocated node
        m_nodes[nodeIndex].parentIndex = -1;
        m_nodes[nodeIndex].child1Index = -1;
        m_nodes[nodeIndex].child2Index = -1;
        m_nodes[nodeIndex].height = 0; // Initial height (will be updated for internal nodes)
        m_nodes[nodeIndex].entity = entt::null; // Default to no entity
        m_nodeCount++;

        return nodeIndex;
    }

    void BVH::freeNode(int nodeIndex) {
        if (nodeIndex < 0 || nodeIndex >= m_nodeCapacity || m_nodes[nodeIndex].height == -1) {
            return; // Invalid index or node already free
        }
        // Add node back to the front of the free list
        m_nodes[nodeIndex].child1Index = m_freeListIndex;
        m_nodes[nodeIndex].height = -1; // Mark as free
        m_freeListIndex = nodeIndex;
        m_nodeCount--;
    }

    void BVH::build(const std::vector<std::pair<entt::entity, AABB>>& items) {
        // Reset the tree state by rebuilding the free list
        m_rootIndex = -1;
        m_nodeCount = 0;
        m_freeListIndex = 0;
        for (int i = 0; i < m_nodeCapacity - 1; ++i) {
            m_nodes[i].child1Index = i + 1;
            m_nodes[i].height = -1;
        }
        m_nodes[m_nodeCapacity - 1].child1Index = -1;
        m_nodes[m_nodeCapacity - 1].height = -1;

        if (items.empty()) {
            return; // Nothing to build
        }

        // Create a mutable copy for sorting
        std::vector<std::pair<entt::entity, AABB>> buildItems = items;

        // Start the recursive build
        m_rootIndex = buildRecursive(buildItems, 0, static_cast<int>(buildItems.size()));
    }

    int BVH::buildRecursive(std::vector<std::pair<entt::entity, AABB>>& items, int start, int end) {
        int count = end - start;

        int nodeIndex = allocateNode();
        BVHNode& node = m_nodes[nodeIndex];

        // Base case: Leaf node
        if (count == 1) {
            node.aabb = items[start].second;
            node.entity = items[start].first;
            node.height = 0;
            // children remain -1
            return nodeIndex;
        }

        // Recursive step: Internal node
        // 1. Compute bounds of items in range
        AABB combinedAABB;
        for (int i = start; i < end; ++i) {
            combinedAABB.expand(items[i].second);
        }

        // 2. Choose split axis (longest dimension)
        glm::vec3 extents = combinedAABB.getExtents() * 2.0f;
        int axis = 0;
        if (extents.y > extents.x && extents.y > extents.z) axis = 1;
        else if (extents.z > extents.x && extents.z > extents.y) axis = 2;

        // 3. Sort items along axis based on AABB center
        std::sort(items.begin() + start, items.begin() + end,
                  [axis](const auto& a, const auto& b) {
                      return a.second.getCenter()[axis] < b.second.getCenter()[axis];
                  });

        // 4. Find midpoint
        int mid = start + count / 2;

        // 5. Recursively build children
        node.child1Index = buildRecursive(items, start, mid);
        node.child2Index = buildRecursive(items, mid, end);

        // 6. Set parent pointers for children
        if (node.child1Index != -1) m_nodes[node.child1Index].parentIndex = nodeIndex;
        if (node.child2Index != -1) m_nodes[node.child2Index].parentIndex = nodeIndex;

        // 7. Update this node's AABB and height
        computeAABB(nodeIndex);

        return nodeIndex;
    }

    void BVH::computeAABB(int nodeIndex) {
        // Ensure valid internal node
        if (nodeIndex < 0 || nodeIndex >= m_nodeCapacity || m_nodes[nodeIndex].isLeaf() || m_nodes[nodeIndex].height == -1) {
            return;
        }

        BVHNode& node = m_nodes[nodeIndex];
        int child1Idx = node.child1Index;
        int child2Idx = node.child2Index;

        // Ensure child indices are valid
        if (child1Idx < 0 || child1Idx >= m_nodeCapacity || m_nodes[child1Idx].height == -1 ||
            child2Idx < 0 || child2Idx >= m_nodeCapacity || m_nodes[child2Idx].height == -1) {
             PN_CORE_WARN("BVH::computeAABB: Invalid child index for node {}. Child1: {}, Child2: {}", nodeIndex, child1Idx, child2Idx);
             // Assign a default state to prevent further issues
             node.aabb = AABB();
             node.height = 0; // Treat as leaf in error case?
             return;
        }

        const BVHNode& child1 = m_nodes[child1Idx];
        const BVHNode& child2 = m_nodes[child2Idx];

        // Merge child AABBs
        node.aabb = AABB::merge(child1.aabb, child2.aabb);
        // Set height
        node.height = 1 + std::max(child1.height, child2.height);
    }

} // namespace PAIN