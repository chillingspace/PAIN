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
        //PN_CORE_INFO("Building BVH with {} items", items.size());

        // ========================================
        // 1. VALIDATE INPUT
        // ========================================
        if (items.empty()) {
            PN_CORE_INFO("No items to build BVH - resetting");
            m_rootIndex = -1;
            m_nodeCount = 0;
            return;
        }

        // Check for capacity overflow
        if (items.size() > static_cast<size_t>(m_nodeCapacity)) {
            PN_CORE_ERROR("Too many items ({}) for BVH capacity ({})",
                items.size(), m_nodeCapacity);

            // Could resize here, but for now just clamp
            PN_CORE_WARN("Clamping to capacity limit");
        }

        // ========================================
        // 2. RESET TREE STATE
        // ========================================
        m_rootIndex = -1;
        m_nodeCount = 0;
        m_freeListIndex = 0;

        // Rebuild free list
        for (int i = 0; i < m_nodeCapacity - 1; ++i) {
            m_nodes[i].child1Index = i + 1;
            m_nodes[i].child2Index = -1;
            m_nodes[i].parentIndex = -1;
            m_nodes[i].height = -1;
            m_nodes[i].entity = entt::null;
        }
        m_nodes[m_nodeCapacity - 1].child1Index = -1;
        m_nodes[m_nodeCapacity - 1].child2Index = -1;
        m_nodes[m_nodeCapacity - 1].parentIndex = -1;
        m_nodes[m_nodeCapacity - 1].height = -1;

        // ========================================
        // 3. CREATE MUTABLE COPY FOR SORTING
        // ========================================
        std::vector<std::pair<entt::entity, AABB>> buildItems = items;

        // Validate copy
        if (buildItems.size() != items.size()) {
            PN_CORE_ERROR("Failed to copy items for BVH build!");
            return;
        }

        // ========================================
        // 4. BUILD TREE RECURSIVELY
        // ========================================
        try {
            m_rootIndex = buildRecursive(buildItems, 0, static_cast<int>(buildItems.size()));

            if (m_rootIndex == -1) {
                PN_CORE_ERROR("BVH build failed - root index is -1");
            }
            else {
                PN_CORE_INFO("BVH built successfully, root index: {}, nodes used: {}",
                    m_rootIndex, m_nodeCount);
            }

        }
        catch (const std::exception& e) {
            PN_CORE_ERROR("Exception during BVH build: {}", e.what());
            m_rootIndex = -1;
            m_nodeCount = 0;
        }
    }

    int BVH::buildRecursive(std::vector<std::pair<entt::entity, AABB>>& items, int start, int end) {
        // ========================================
        // 1. VALIDATE PARAMETERS
        // ========================================
        if (start < 0 || end < 0 || start >= end) {
            PN_CORE_ERROR("Invalid range: start={}, end={}", start, end);
            return -1;
        }

        if (end > static_cast<int>(items.size())) {
            PN_CORE_ERROR("End index {} exceeds items size {}", end, items.size());
            return -1;
        }

        int count = end - start;

        if (count <= 0) {
            PN_CORE_ERROR("Invalid count: {}", count);
            return -1;
        }

        // ========================================
        // 2. ALLOCATE NODE (WITH VALIDATION)
        // ========================================
        int nodeIndex = allocateNode();

        if (nodeIndex < 0 || nodeIndex >= m_nodeCapacity) {
            PN_CORE_ERROR("Invalid node index: {} (capacity: {})", nodeIndex, m_nodeCapacity);
            return -1;
        }

        BVHNode& node = m_nodes[nodeIndex];

        // ========================================
        // 3. BASE CASE: LEAF NODE
        // ========================================
        if (count == 1) {
            // SAFE: Bounds-checked access
            if (start >= static_cast<int>(items.size())) {
                PN_CORE_ERROR("Start index {} out of range (size: {})", start, items.size());
                return -1;
            }

            node.aabb = items[start].second;
            node.entity = items[start].first;
            node.height = 0;
            node.child1Index = -1;
            node.child2Index = -1;
            //PN_CORE_TRACE("Leaf Created: Node {}, Entity {}", nodeIndex, (uint32_t)node.entity);
            return nodeIndex;
        }

        // ========================================
        // 4. RECURSIVE CASE: INTERNAL NODE
        // ========================================

        // Compute combined AABB for all items in range
        AABB combinedAABB;
        for (int i = start; i < end; ++i) {
            // SAFE: Bounds check
            if (i >= static_cast<int>(items.size())) {
                PN_CORE_ERROR("Index {} exceeds items size {} during AABB computation", i, items.size());
                return -1;
            }
            combinedAABB.expand(items[i].second);
        }

        // Choose split axis (longest dimension)
        glm::vec3 extents = combinedAABB.getExtents() * 2.0f;
        int axis = 0;
        if (extents.y > extents.x && extents.y > extents.z) {
            axis = 1;
        }
        else if (extents.z > extents.x && extents.z > extents.y) {
            axis = 2;
        }

        // ========================================
        // 5. SORT ITEMS ALONG AXIS (SAFE)
        // ========================================

        // Validate range before sorting
        if (start >= static_cast<int>(items.size()) || end > static_cast<int>(items.size())) {
            PN_CORE_ERROR("Invalid sort range: start={}, end={}, size={}",
                start, end, items.size());
            return -1;
        }

        try {
            std::sort(items.begin() + start, items.begin() + end,
                [axis](const auto& a, const auto& b) {
                    return a.second.getCenter()[axis] < b.second.getCenter()[axis];
                });
        }
        catch (const std::exception& e) {
            PN_CORE_ERROR("Sort failed: {}", e.what());
            return -1;
        }

        // ========================================
        // 6. FIND MIDPOINT
        // ========================================
        int mid = start + count / 2;

        // Validate midpoint
        if (mid <= start || mid >= end) {
            PN_CORE_ERROR("Invalid midpoint: start={}, mid={}, end={}", start, mid, end);
            return -1;
        }

        // ========================================
        // 7. RECURSIVELY BUILD CHILDREN
        // ========================================

        // Check recursion depth to prevent stack overflow
        static thread_local int recursionDepth = 0;
        const int MAX_RECURSION_DEPTH = 64;  // ~2^64 nodes

        if (recursionDepth >= MAX_RECURSION_DEPTH) {
            PN_CORE_ERROR("Maximum recursion depth reached: {}", recursionDepth);
            // Fallback: create leaf node
            node.aabb = combinedAABB;
            node.entity = items[start].first;  // Use first entity
            node.height = 0;
            node.child1Index = -1;
            node.child2Index = -1;
            return nodeIndex;
        }

        recursionDepth++;

        // Build left child
        //PN_CORE_TRACE("Recursing Left: [{}, {})", start, mid);
        int leftResult = buildRecursive(items, start, mid);
        //PN_CORE_TRACE("Recursing Right: [{}, {})", mid, end);
        int rightResult = buildRecursive(items, mid, end);

        recursionDepth--;

        // ========================================
        // 8. VALIDATE & LINK CHILDREN
        // ========================================
        
        // Check for failure in EITHER child
        if (leftResult == -1 || rightResult == -1) {
            freeNode(nodeIndex);
            if (leftResult != -1) freeNode(leftResult);
            if (rightResult != -1) freeNode(rightResult);
            return -1;
        }

        // Re-acquire the reference from the vector using the index.
        BVHNode& currentNode = m_nodes[nodeIndex];

        currentNode.child1Index = leftResult;
        currentNode.child2Index = rightResult;
        currentNode.aabb = combinedAABB;
        currentNode.entity = entt::null; // Internal nodes have no entity
        currentNode.height = std::max(m_nodes[leftResult].height, m_nodes[rightResult].height) + 1;

        // Link Parents
        m_nodes[leftResult].parentIndex = nodeIndex;
        m_nodes[rightResult].parentIndex = nodeIndex;

        return nodeIndex;
    }

    int BVH::insertLeaf(entt::entity entity, const AABB& aabb) {
        // Allocate new leaf node
        int leafIndex = allocateNode();
        if (leafIndex < 0 || leafIndex >= m_nodeCapacity) {
            PN_CORE_ERROR("[BVH] Failed to allocate leaf node");
            return -1;
        }

        // Initialize leaf
        BVHNode& leafNode = m_nodes[leafIndex];
        leafNode.aabb = aabb;
        leafNode.entity = entity;
        leafNode.height = 0;
        leafNode.child1Index = -1;
        leafNode.child2Index = -1;
        leafNode.parentIndex = -1;

        // If tree is empty, make this the root
        if (m_rootIndex == -1) {
            m_rootIndex = leafIndex;
            return leafIndex;
        }

        // Find best sibling for the new leaf using SAH
        int siblingIndex = findBestSibling(aabb);

        // Create new parent to hold sibling and new leaf
        int oldParentIndex = m_nodes[siblingIndex].parentIndex;
        int newParentIndex = allocateNode();

        if (newParentIndex < 0 || newParentIndex >= m_nodeCapacity) {
            PN_CORE_ERROR("[BVH] Failed to allocate internal node");
            freeNode(leafIndex);
            return -1;
        }

        BVHNode& newParent = m_nodes[newParentIndex];
        newParent.parentIndex = oldParentIndex;
        newParent.aabb = AABB::merge(aabb, m_nodes[siblingIndex].aabb);
        newParent.height = m_nodes[siblingIndex].height + 1;
        newParent.entity = entt::null;

        // Connect new parent to old parent (or make it root)
        if (oldParentIndex != -1) {
            // Sibling was not root
            if (m_nodes[oldParentIndex].child1Index == siblingIndex) {
                m_nodes[oldParentIndex].child1Index = newParentIndex;
            }
            else {
                m_nodes[oldParentIndex].child2Index = newParentIndex;
            }
        }
        else {
            // Sibling was root
            m_rootIndex = newParentIndex;
        }

        // Connect new parent to sibling and new leaf
        newParent.child1Index = siblingIndex;
        newParent.child2Index = leafIndex;
        m_nodes[siblingIndex].parentIndex = newParentIndex;
        leafNode.parentIndex = newParentIndex;

        // Refit AABBs up to root
        refitParentChain(leafIndex);

        //PN_CORE_TRACE("[BVH] Inserted leaf {} for entity {}", leafIndex, static_cast<uint32_t>(entity));
        return leafIndex;
    }

    void BVH::removeLeaf(int nodeIndex) {
        // Validate node
        if (nodeIndex < 0 || nodeIndex >= m_nodeCapacity) {
            PN_CORE_WARN("[BVH] Invalid node index for removal: {}", nodeIndex);
            return;
        }

        if (!isValidLeaf(nodeIndex)) {
            PN_CORE_WARN("[BVH] Node {} is not a valid leaf", nodeIndex);
            return;
        }

        // If removing root
        if (nodeIndex == m_rootIndex) {
            freeNode(nodeIndex);
            m_rootIndex = -1;
            return;
        }

        int parentIndex = m_nodes[nodeIndex].parentIndex;
        if (parentIndex == -1) {
            PN_CORE_ERROR("[BVH] Leaf {} has no parent but is not root", nodeIndex);
            freeNode(nodeIndex);
            return;
        }

        // Get sibling (the other child of parent)
        BVHNode& parent = m_nodes[parentIndex];
        int siblingIndex = (parent.child1Index == nodeIndex)
            ? parent.child2Index
            : parent.child1Index;

        if (siblingIndex < 0 || siblingIndex >= m_nodeCapacity) {
            PN_CORE_ERROR("[BVH] Invalid sibling index: {}", siblingIndex);
            freeNode(nodeIndex);
            return;
        }

        // Connect sibling to grandparent
        int grandParentIndex = parent.parentIndex;

        if (grandParentIndex != -1) {
            // Parent was not root - connect sibling to grandparent
            BVHNode& grandParent = m_nodes[grandParentIndex];

            if (grandParent.child1Index == parentIndex) {
                grandParent.child1Index = siblingIndex;
            }
            else {
                grandParent.child2Index = siblingIndex;
            }

            m_nodes[siblingIndex].parentIndex = grandParentIndex;

            // Free the parent and leaf
            freeNode(parentIndex);
            freeNode(nodeIndex);

            // Refit AABBs from grandparent up to root
            refitParentChain(siblingIndex);

        }
        else {
            // Parent was root - sibling becomes new root
            m_rootIndex = siblingIndex;
            m_nodes[siblingIndex].parentIndex = -1;

            // Free the parent and leaf
            freeNode(parentIndex);
            freeNode(nodeIndex);
        }

        PN_CORE_TRACE("[BVH] Removed leaf {}", nodeIndex);
    }

    std::vector<std::pair<int, entt::entity>> BVH::getLeafNodes() const {
        std::vector<std::pair<int, entt::entity>> leafNodes;

        if (m_rootIndex == -1 || !isBuilt()) {
            return leafNodes;
        }

        // Reserve approximate capacity (half of nodes are typically leaves)
        leafNodes.reserve(m_nodeCount / 2 + 1);

        // Iterative traversal using stack (avoids recursion overhead)
        std::vector<int> stack;
        stack.reserve(32); // Typical tree depth
        stack.push_back(m_rootIndex);

        while (!stack.empty()) {
            int nodeIndex = stack.back();
            stack.pop_back();

            // Validate node
            if (nodeIndex < 0 || nodeIndex >= m_nodeCapacity) {
                continue;
            }

            const BVHNode& node = m_nodes[nodeIndex];

            // Skip free nodes
            if (node.height == -1) {
                continue;
            }

            // If leaf, add to results
            if (node.isLeaf()) {
                if (node.entity != entt::null) {
                    leafNodes.push_back({ nodeIndex, node.entity });
                }
            }
            else {
                // Internal node - traverse children
                if (node.child1Index != -1) {
                    stack.push_back(node.child1Index);
                }
                if (node.child2Index != -1) {
                    stack.push_back(node.child2Index);
                }
            }
        }

        return leafNodes;
    }

    int BVH::findBestSibling(const AABB& leafAABB) {
        int bestSibling = m_rootIndex;
        float bestCost = AABB::merge(m_nodes[m_rootIndex].aabb, leafAABB).getSurfaceArea();

        // Use a priority queue or simple stack-based search
        std::vector<int> stack;
        stack.reserve(32);
        stack.push_back(m_rootIndex);

        while (!stack.empty()) {
            int nodeIndex = stack.back();
            stack.pop_back();

            if (nodeIndex < 0 || nodeIndex >= m_nodeCapacity) continue;

            const BVHNode& node = m_nodes[nodeIndex];
            if (node.height == -1) continue;

            // Calculate cost of creating new parent with this node
            AABB combinedAABB = AABB::merge(node.aabb, leafAABB);
            float combinedCost = combinedAABB.getSurfaceArea();

            // Cost of descending
            float costOfDescending = combinedCost - node.aabb.getSurfaceArea();

            // If this is better, update best
            if (combinedCost < bestCost) {
                bestCost = combinedCost;
                bestSibling = nodeIndex;
            }

            // Lower bound on cost of descending into children
            float lowerBoundCost = costOfDescending + leafAABB.getSurfaceArea();

            // Only descend if there's potential for improvement
            if (lowerBoundCost < bestCost && !node.isLeaf()) {
                if (node.child1Index != -1) {
                    stack.push_back(node.child1Index);
                }
                if (node.child2Index != -1) {
                    stack.push_back(node.child2Index);
                }
            }
        }

        return bestSibling;
    }

    float BVH::calculateSurfaceArea(const AABB& aabb) {
        return aabb.getSurfaceArea();
    }

    bool BVH::isValidLeaf(int nodeIndex) const {
        if (nodeIndex < 0 || nodeIndex >= m_nodeCapacity) {
            return false;
        }

        const BVHNode& node = m_nodes[nodeIndex];

        // Valid leaf: no children, height >= 0, valid entity
        return node.isLeaf() &&
            node.height != -1 &&
            node.entity != entt::null;
    }

    void BVH::updateLeaf(int leafIndex, const AABB& newAABB) {
        // Validate leaf index
        if (leafIndex < 0 || leafIndex >= m_nodeCapacity) {
            PN_CORE_WARN("Invalid leaf index for update: {}", leafIndex);
            return;
        }

        BVHNode& node = m_nodes[leafIndex];

        // Ensure it's a valid leaf
        if (!node.isLeaf()) {
            return;
        }

        if (node.height == -1) {
            return;
        }

        // Update leaf AABB
        node.aabb = newAABB;

        // Refit parent chain up to root
        refitParentChain(leafIndex);
    }

    void BVH::refitParentChain(int leafIndex) {
        int currentIndex = m_nodes[leafIndex].parentIndex;

        // Traverse up to root, recomputing AABBs
        while (currentIndex != -1) {
            // Validate index
            if (currentIndex < 0 || currentIndex >= m_nodeCapacity) {
                PN_CORE_ERROR("Invalid parent index during refit: {}", currentIndex);
                break;
            }

            // Recompute this node's AABB from its children
            computeAABB(currentIndex);

            // Move to parent
            currentIndex = m_nodes[currentIndex].parentIndex;
        }
    }

    void BVH::clear() {
        m_rootIndex = -1;
        m_nodeCount = 0;
        rebuildFreeList();
    }

    void BVH::rebuildFreeList() {
        m_freeListIndex = 0;
        for (int i = 0; i < m_nodeCapacity - 1; ++i) {
            m_nodes[i].child1Index = i + 1;
            m_nodes[i].child2Index = -1;
            m_nodes[i].parentIndex = -1;
            m_nodes[i].height = -1;
            m_nodes[i].entity = entt::null;
        }
        m_nodes[m_nodeCapacity - 1].child1Index = -1;
        m_nodes[m_nodeCapacity - 1].height = -1;
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
            #ifdef PN_PLATFORM_WINDOWS
             PN_CORE_WARN("BVH::computeAABB: Invalid child index for node {}. Child1: {}, Child2: {}", nodeIndex, child1Idx, child2Idx);
            #endif
             // Return a zero-sized box because default state is an invalid box
             node.aabb = AABB(glm::vec3(0), glm::vec3(0));
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