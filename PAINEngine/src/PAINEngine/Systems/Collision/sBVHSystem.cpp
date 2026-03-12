#include "pch.h" // Must be first

// Include own header
#include "Systems/Collision/sBVHSystem.h"

// Include necessary headers for implementation details
#include "ECS/Controller.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cMeshRenderer.h"
#include "ECS/Components/cBoundingVolume.h"
#include "ECS/Components/cMetadata.h"
#include "CoreSystems/Assets/sAssets.h" // For Assets::Model and Assets::Vertex
#include "CoreSystems/Scene/Scene.h"
#include <limits> // For std::numeric_limits

// Ensure code is within the namespace
namespace PAIN {

    // Constructor implementation
    sBVHSystem::sBVHSystem(std::shared_ptr<Services> svc)
        : ECS::System::ISystem(svc), m_bvh(128) // Initialize base class and member BVH
    {
        PN_CORE_INFO("BVH System Initialized.");
    }

    // AABB calculation, using Assets::Model
    AABB sBVHSystem::calculateLocalAABB(const std::shared_ptr<Assets::Model>& model) {
        AABB localAABB; // Initializes with max/lowest bounds
        if (!model) {
            PN_CORE_WARN("Attempted to calculate AABB for a null model. Using default small box.");
            localAABB.min = glm::vec3(-0.01f);
            localAABB.max = glm::vec3(0.01f);
            return localAABB;
        }

        // Explicitly use PAIN::Assets::Vertex, which is the type in model->vertices
        const std::vector<PAIN::Assets::Vertex>& vertices = model->vertices;
        if (vertices.empty()) {
             PN_CORE_WARN("Model has no vertices. Using default small box.");
             localAABB.min = glm::vec3(-0.01f);
             localAABB.max = glm::vec3(0.01f);
             return localAABB;
        }

        // Iterate through all vertices and expand the AABB to include them
        for (const auto& vertex : vertices) {
            localAABB.expand(vertex.pos);
        }

        // Add a small epsilon to avoid degenerate AABBs (lines or points)
         glm::vec3 extents = localAABB.getExtents();
         const float minExtent = 0.01f; // Minimum size threshold
         if (extents.x < minExtent) { localAABB.min.x -= minExtent; localAABB.max.x += minExtent; }
         if (extents.y < minExtent) { localAABB.min.y -= minExtent; localAABB.max.y += minExtent; }
         if (extents.z < minExtent) { localAABB.min.z -= minExtent; localAABB.max.z += minExtent; }

        return localAABB;
    }

    AABB sBVHSystem::calculateSkinnedLocalAABB(const std::shared_ptr<Assets::Model>& model, const std::vector<glm::mat4>& boneTransforms) {
        AABB localAABB;
        if (!model || model->vertices.empty()) {
            localAABB.min = glm::vec3(-0.01f);
            localAABB.max = glm::vec3(0.01f);
            return localAABB;
        }

        const float minWeight = 1e-6f;

        for (const auto& vertex : model->vertices) {
            glm::vec3 skinnedPos(0.0f);
            float totalWeight = 0.0f;

            for (int i = 0; i < 4; ++i) {
                const int boneIndex = vertex.boneIndices[i];
                const float weight = vertex.boneWeights[i];

                if (weight <= minWeight || boneIndex < 0 || boneIndex >= static_cast<int>(boneTransforms.size())) {
                    continue;
                }

                const glm::vec4 transformed = boneTransforms[boneIndex] * glm::vec4(vertex.pos, 1.0f);
                skinnedPos += glm::vec3(transformed) * weight;
                totalWeight += weight;
            }

            if (totalWeight > minWeight) {
                skinnedPos /= totalWeight;
            }
            else {
                skinnedPos = vertex.pos;
            }

            localAABB.expand(skinnedPos);
        }

        glm::vec3 extents = localAABB.getExtents();
        const float minExtent = 0.01f;
        if (extents.x < minExtent) { localAABB.min.x -= minExtent; localAABB.max.x += minExtent; }
        if (extents.y < minExtent) { localAABB.min.y -= minExtent; localAABB.max.y += minExtent; }
        if (extents.z < minExtent) { localAABB.min.z -= minExtent; localAABB.max.z += minExtent; }

        return localAABB;
    }

    // CoreSystems/BVH/sBVHSystem.cpp

    std::vector<entt::entity> sBVHSystem::queryAABB(
        const AABB& queryBox,
        entt::registry& registry,
        int queryLayerID
    ) {
        std::vector<entt::entity> results;

        if (!m_bvh.isBuilt()) {
            PN_CORE_WARN("BVH not built - cannot query");
            return results;
        }

        // Get scene layers
        auto serialization = getServices()->get<Serialization::Service>();
        auto scnManager = getServices()->get<Scene::SceneManager>();

        // Recursive query function
        std::function<void(int)> queryNode = [&](int nodeIndex) {
            if (nodeIndex < 0 || nodeIndex >= m_bvh.getNodes().size()) {
                return;
            }

            const auto& node = m_bvh.getNodes()[nodeIndex];

            // Check if query box intersects this node
            if (!queryBox.intersects(node.aabb)) {
                return;  // No intersection - prune this branch
            }

            // Leaf node - check entity
            if (node.isLeaf()) {
                if (!registry.valid(node.entity)) {
                    return;
                }

                // Layer filtering
                if (queryLayerID != -1) {
                    auto* entityLayer = registry.try_get<Entity::Layer>(node.entity);
                    if (entityLayer) {
                        // Check if layers can collide
                        if (!scnManager->canLayersInteract(queryLayerID, entityLayer->layer_id)) {
                            return;  // Layers cannot collide - skip
                        }
                    }
                }

                // Check precise AABB intersection
                auto* bv = registry.try_get<BoundingVolume>(node.entity);
                if (bv && queryBox.intersects(bv->worldAABB)) {
                    results.push_back(node.entity);
                }

                return;
            }

            // Internal node - recurse to children
            if (node.child1Index != -1) {
                queryNode(node.child1Index);
            }
            if (node.child2Index != -1) {
                queryNode(node.child2Index);
            }
            };

        // Start query from root
        queryNode(m_bvh.getRootIndex());

        return results;
    }

    bool sBVHSystem::shouldCollide(
        entt::entity entityA,
        entt::entity entityB
    ) {
        auto controller = getServices()->get<ECS::Controller>();
        if (!controller) return true;

        auto& registry = controller->getRegistry();

        // Get layer components
        auto layerA = registry.try_get<Entity::Layer>(entityA);
        auto layerB = registry.try_get<Entity::Layer>(entityB);

        // No layer component = default layer (always collide)
        if (!layerA || !layerB) return true;

        // Get scene manager
        auto sceneManager = getServices()->get<Scene::SceneManager>();
        if (!sceneManager) return true;

        const auto& layers = sceneManager->getLayers();

        // Check if layers exist and are enabled
        if (layerA->layer_id < layers.size() && !layers[layerA->layer_id].enabled) {
            return false;
        }
        if (layerB->layer_id < layers.size() && !layers[layerB->layer_id].enabled) {
            return false;
        }

        // Check collision matrix
        return sceneManager->canLayersInteract(layerA->layer_id, layerB->layer_id);
    }

    std::vector<sBVHSystem::RaycastHit> sBVHSystem::raycastAll(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        entt::registry& registry,
        int layerMask
    ) {
        std::vector<RaycastHit> allHits;

        if (!m_bvh.isBuilt()) {
            return allHits; // Return empty vector
        }

        glm::vec3 rayDir = glm::normalize(direction);

        // Recursive raycast function
        std::function<void(int)> raycastNode = [&](int nodeIndex) {
            if (nodeIndex < 0 || nodeIndex >= m_bvh.getNodes().size()) {
                return;
            }

            const auto& node = m_bvh.getNodes()[nodeIndex];

            // Ray-AABB intersection test
            float tMin, tMax;
            if (!rayAABBIntersect(origin, rayDir, node.aabb, tMin, tMax)) {
                return;  // Ray doesn't hit this node
            }

            if (tMin > maxDistance) return;

            // Leaf node - check entity
            if (node.isLeaf()) {
                if (!registry.valid(node.entity)) {
                    return;
                }

                // Layer filtering
                auto* entityLayer = registry.try_get<Entity::Layer>(node.entity);
                if (entityLayer && layerMask != -1) {
                    // Check if layer is in mask
                    if ((layerMask & entityLayer->layer_mask) == 0) {
                        return;  // Layer not in mask - skip
                    }
                }

                // Check distance validity
                if (tMin < maxDistance) {
                    RaycastHit hit;
                    hit.entity = node.entity;
                    hit.distance = (tMin < 0.0f) ? 0.0f : tMin;

                    hit.point = origin + rayDir * tMin;
                    hit.normal = calculateAABBNormal(node.aabb, hit.point);
                    if (entityLayer) hit.layer = entityLayer->layer_id;

                    // Add to hits
                    allHits.push_back(hit);
                }

                return;
            }

            // Internal node - recurse to children
            if (node.child1Index != -1) {
                raycastNode(node.child1Index);
            }
            if (node.child2Index != -1) {
                raycastNode(node.child2Index);
            }
            };

        raycastNode(m_bvh.getRootIndex());

        std::sort(allHits.begin(), allHits.end(), [](const auto& a, const auto& b) {
            return a.distance < b.distance;
            });

        PN_CORE_INFO("Raycast found {} hits. Filtering...", allHits.size());

        for (const auto& hit : allHits) {
            auto* vol = registry.try_get<BoundingVolume>(hit.entity);
            float volume = (vol) ? vol->worldAABB.getVolume() : 0.0f;

            // Log every candidate
            //PN_CORE_INFO(" - Hit Entity {}: Dist={:.2f}, Vol={:.2f}",
            //    (uint32_t)hit.entity, hit.distance, volume);

        }

        return allHits;
    }

    std::optional<sBVHSystem::RaycastHit> sBVHSystem::raycast(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        entt::registry& registry,
        int layerMask
    ) {
        if (!m_bvh.isBuilt()) {
            return std::nullopt;
        }

        RaycastHit closestHit;
        closestHit.distance = maxDistance;

        glm::vec3 rayDir = glm::normalize(direction);

        // Recursive raycast function
        std::function<void(int)> raycastNode = [&](int nodeIndex) {
            if (nodeIndex < 0 || nodeIndex >= m_bvh.getNodes().size()) {
                return;
            }

            const auto& node = m_bvh.getNodes()[nodeIndex];

            // Ray-AABB intersection test
            float tMin, tMax;
            if (!rayAABBIntersect(origin, rayDir, node.aabb, tMin, tMax)) {
                return;  // Ray doesn't hit this node
            }

            if (tMin > closestHit.distance) {
                return;  // Already found closer hit
            }

            // Leaf node - check entity
            if (node.isLeaf()) {
                if (!registry.valid(node.entity)) {
                    return;
                }

                // Layer filtering
                auto* entityLayer = registry.try_get<Entity::Layer>(node.entity);
                if (entityLayer && layerMask != -1) {
                    // Check if layer is in mask
                    if ((layerMask & entityLayer->layer_mask) == 0) {
                        return;  // Layer not in mask - skip
                    }

                }

                // Get entity's AABB
                auto* bv = registry.try_get<BoundingVolume>(node.entity);

                auto* worldTransform = registry.try_get<WorldTransform>(node.entity);
                if (!bv || !worldTransform) return;

                // Check if this intersection is closer than current best
                if (tMin >= 0) {
                    closestHit.entity = node.entity;
                    closestHit.distance = tMin;
                    closestHit.point = origin + rayDir * tMin;

                    // Calculate normal based on World AABB
                    closestHit.normal = calculateAABBNormal(node.aabb, closestHit.point);

                    // Optional: Layer ID
                    if (entityLayer) closestHit.layer = entityLayer->layer_id;
                }

                return;
            }

            // Internal node - recurse to children
            if (node.child1Index != -1) {
                raycastNode(node.child1Index);
            }
            if (node.child2Index != -1) {
                raycastNode(node.child2Index);
            }
            };

        raycastNode(m_bvh.getRootIndex());

        if (closestHit.entity != entt::null) {
            return closestHit;
        }

        return std::nullopt;
    }

    // Helper: Ray-AABB intersection
    bool sBVHSystem::rayAABBIntersect(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const AABB& aabb,
        float& tMin,
        float& tMax
    ) {
        tMin = 0.0f;
        tMax = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; ++i) {
            if (std::abs(direction[i]) < 1e-6f) {
                // Ray is parallel to slab
                if (origin[i] < aabb.min[i] || origin[i] > aabb.max[i]) {
                    return false;
                }
            }
            else {
                // Compute intersection with slab
                float invD = 1.0f / direction[i];
                float t0 = (aabb.min[i] - origin[i]) * invD;
                float t1 = (aabb.max[i] - origin[i]) * invD;

                if (t0 > t1) std::swap(t0, t1);

                tMin = std::max(tMin, t0);
                tMax = std::min(tMax, t1);

                if (tMin > tMax) {
                    return false;
                }
            }
        }

        return true;
    }

    // Helper: Calculate normal at AABB hit point
    glm::vec3 sBVHSystem::calculateAABBNormal(const AABB& aabb, const glm::vec3& point) {
        const float epsilon = 0.001f;

        // Check which face was hit
        if (std::abs(point.x - aabb.min.x) < epsilon) return glm::vec3(-1, 0, 0);
        if (std::abs(point.x - aabb.max.x) < epsilon) return glm::vec3(1, 0, 0);
        if (std::abs(point.y - aabb.min.y) < epsilon) return glm::vec3(0, -1, 0);
        if (std::abs(point.y - aabb.max.y) < epsilon) return glm::vec3(0, 1, 0);
        if (std::abs(point.z - aabb.min.z) < epsilon) return glm::vec3(0, 0, -1);
        if (std::abs(point.z - aabb.max.z) < epsilon) return glm::vec3(0, 0, 1);

        return glm::vec3(0, 1, 0);  // Default up
    }

    std::vector<std::pair<entt::entity, entt::entity>> sBVHSystem::detectCollisions(
        entt::registry& registry
    ) {
        std::vector<std::pair<entt::entity, entt::entity>> collisionPairs;

        if (!m_bvh.isBuilt()) {
            return collisionPairs;
        }

        // Get all entities with bounding volumes
        auto view = registry.view<BoundingVolume>();

        for (auto entityA : view) {
            // Query BVH for potential collisions
            const auto& bvA = view.get<BoundingVolume>(entityA);
            auto potentialCollisions = queryAABB(bvA.worldAABB, registry, -1);

            for (auto entityB : potentialCollisions) {
                // Skip self-collision
                if (entityA == entityB) continue;

                // Check if already in pair (avoid duplicates)
                if (entityA > entityB) continue;

                // Layer-based filtering
                if (!shouldCollide(entityA, entityB)) {
                    continue;
                }

                // Check actual AABB intersection
                auto* bvB = registry.try_get<BoundingVolume>(entityB);
                if (bvB && bvA.worldAABB.intersects(bvB->worldAABB)) {
                    collisionPairs.push_back({ entityA, entityB });
                }
            }
        }

        return collisionPairs;
    }

    void sBVHSystem::onUpdate(AppTiming timing, entt::registry& registry) {
        auto sceneService = getServices()->get<Scene::SceneManager>();
        if (!sceneService) return;
        // Reuse allocations (Fix #1)
        m_currentFrameEntities.clear();
        m_bvhItems.clear();

        bool anyEntityMoved = false;
        int aabbUpdateCount = 0;

        // Single-pass entity processing with better cache locality (Fix #4)
        // Optimized using group
        auto bvGroup = registry.group<BoundingVolume>(entt::get<WorldTransform>);

        for (auto [entity, bvComponent, transform] : bvGroup.each()) {

            m_currentFrameEntities.insert(entity);

            // Update AABB if transform dirty 
            if (bvComponent.needsUpdate) {

                if (auto* modelRenderer = registry.try_get<ModelRenderer>(entity)) {
                    std::shared_ptr<Assets::Model> modelForBounds;

                    if (modelRenderer->cachedModelAsset) {
                        modelForBounds = std::const_pointer_cast<Assets::Model>(modelRenderer->cachedModelAsset);
                    }
                    else if (modelRenderer->modelGUID.IsValid()) {
                        auto modelOpt = getServices()->get<Assets::Manager>()->getAsset<Assets::Model>(modelRenderer->modelGUID);
                        if (modelOpt.has_value()) {
                            modelForBounds = modelOpt.value();
                        }
                    }

                    if (modelForBounds && !modelRenderer->boneTransforms.empty()) {
                        bvComponent.localAABB = calculateSkinnedLocalAABB(modelForBounds, modelRenderer->boneTransforms);
                    }
                }

                // Calculate new aabb if is dirty
                AABB newWorldAABB = bvComponent.localAABB.transform(transform.matrix);
                bvComponent.worldAABB = newWorldAABB;

                // Only touch BVH if entity has escaped its fat AABB
                bool escapedFat = !bvComponent.fatAABB.contains(newWorldAABB);

                if (escapedFat && m_bvh.isBuilt() && bvComponent.bvhNodeIndex != -1) {
                    // Reinsert with new fat AABB (refit only, no rebuild)
                    bvComponent.fatAABB = newWorldAABB.expanded(BVH_FAT_AABB_PADDING);
                    m_bvh.updateLeaf(bvComponent.bvhNodeIndex, bvComponent.fatAABB);
                }
                // If still inside fat AABB, BVH needs zero work this frame

                bvComponent.needsUpdate = false;
            }

            m_bvhItems.push_back({ entity, bvComponent.worldAABB });
        }

        // Separate pass for new entities (Fix #6)
        // Use view to avoid group ownership conflict with sysRender
        auto needsBVView = registry.view<ModelRenderer, WorldTransform>(entt::exclude<BoundingVolume>);

        for (auto [entity, modelRenderer, transform] : needsBVView.each()) {

            // Early validation (Fix #6)
            if (!modelRenderer.modelGUID.IsValid()) continue;

            auto model_opt = getServices()->get<Assets::Manager>()->getAsset<Assets::Model>(modelRenderer.modelGUID);
            if (!model_opt.has_value() || model_opt.value()->vertices.empty()) continue;

            // Create BV component
            auto& bvComponent = registry.emplace<BoundingVolume>(entity);
            bvComponent.localAABB = calculateLocalAABB(model_opt.value());
            bvComponent.worldAABB = bvComponent.localAABB.transform(transform.matrix);
            bvComponent.needsUpdate = false;
            bvComponent.bvhNodeIndex = -1;

            m_currentFrameEntities.insert(entity);
            m_bvhItems.push_back({ entity, bvComponent.worldAABB });

            // Incremental insert 
            if (m_bvh.isBuilt() && m_bvhItems.size() < m_lastEntityCount * 1.2) {
                bvComponent.bvhNodeIndex = m_bvh.insertLeaf(entity, bvComponent.worldAABB);
            }
            else {
                m_needsFullRebuild = true;
            }
        }

        // Improved removal detection (Fix #3)
        std::vector<entt::entity> removedEntities;
        for (auto trackedEntity : m_trackedEntities) {
            if (m_currentFrameEntities.find(trackedEntity) == m_currentFrameEntities.end()) {
                removedEntities.push_back(trackedEntity);
            }
        }

        if (!removedEntities.empty()) {
            if (removedEntities.size() < m_trackedEntities.size() * 0.05) {
                // Incremental removal (Fix #2)
                for (auto entity : removedEntities) {
                    auto* bv = registry.try_get<BoundingVolume>(entity);
                    if (bv && bv->bvhNodeIndex != -1) {
                        m_bvh.removeLeaf(bv->bvhNodeIndex);
                    }
                }
            }
            else {
                m_needsFullRebuild = true;
            }
        }

        m_trackedEntities = std::move(m_currentFrameEntities);

        m_framesSinceRebuild++;
        bool periodicRebuild = (m_framesSinceRebuild >= BVH_REBUILD_INTERVAL);

        if (m_needsFullRebuild || !m_bvh.isBuilt() || periodicRebuild) {
            m_framesSinceRebuild = 0;
            //PN_CORE_INFO("BVH full rebuild: {} items", m_bvhItems.size());

            auto rebuildStart = std::chrono::high_resolution_clock::now();
            m_bvh.build(m_bvhItems);


            // Iterate over all nodes in the new BVH to update component references
            const auto& nodes = m_bvh.getNodes();
            for (int i = 0; i < nodes.size(); ++i) {
                // Only care about leaf nodes with valid entities
                if (nodes[i].isLeaf() && nodes[i].entity != entt::null) {
                    if (registry.valid(nodes[i].entity)) {
                        auto* bv = registry.try_get<BoundingVolume>(nodes[i].entity);
                        if (bv) {
                            bv->bvhNodeIndex = i; 
                        }
                    }
                }
            }

            auto rebuildEnd = std::chrono::high_resolution_clock::now();
            auto rebuildTime = std::chrono::duration_cast<std::chrono::microseconds>(rebuildEnd - rebuildStart).count();
            // PN_CORE_INFO("BVH rebuild complete: {} microseconds", rebuildTime);

            m_needsFullRebuild = false;
            m_lastEntityCount = m_bvhItems.size();
        }
    }


    // Implementation for event handling
    void sBVHSystem::onEvent(Event::Event& e)
    {
        // Placeholder for event responses
    }

} // namespace PAIN
