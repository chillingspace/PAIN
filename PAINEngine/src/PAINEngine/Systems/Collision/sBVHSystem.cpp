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
                if (!bv) return;

                // Precise ray-AABB intersection
                float t;
                if (rayAABBIntersect(origin, rayDir, bv->worldAABB, tMin, tMax)) {
                    if (tMin >= 0 && tMin < closestHit.distance) {
                        closestHit.entity = node.entity;
                        closestHit.distance = tMin;
                        closestHit.point = origin + rayDir * tMin;
                        closestHit.normal = calculateAABBNormal(bv->worldAABB, closestHit.point);
                        closestHit.layer = entityLayer ? entityLayer->layer_id : 0;
                    }
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

            // Update AABB if transform dirty (Fix #5)
            if (bvComponent.needsUpdate) {
                AABB newWorldAABB = bvComponent.localAABB.transform(transform.matrix);

                if (!(bvComponent.worldAABB == newWorldAABB)) {
                    bvComponent.worldAABB = newWorldAABB;
                    aabbUpdateCount++;
                    anyEntityMoved = true;

                    // Incremental update if possible
                    if (m_bvh.isBuilt() &&
                        !m_needsFullRebuild &&
                        bvComponent.bvhNodeIndex != -1) {
                        m_bvh.updateLeaf(bvComponent.bvhNodeIndex, bvComponent.worldAABB);
                    }
                }

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

            // Incremental insert (Fix #2)
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

        // Rebuild only if necessary (Fix #2, #8)
        if (m_needsFullRebuild || !m_bvh.isBuilt()) {
            PN_CORE_INFO("BVH full rebuild: {} items", m_bvhItems.size());

            auto rebuildStart = std::chrono::high_resolution_clock::now();
            m_bvh.build(m_bvhItems);
            auto rebuildEnd = std::chrono::high_resolution_clock::now();

            auto rebuildTime = std::chrono::duration_cast<std::chrono::microseconds>(rebuildEnd - rebuildStart).count();
            PN_CORE_INFO("BVH rebuild complete: {} μs", rebuildTime);

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