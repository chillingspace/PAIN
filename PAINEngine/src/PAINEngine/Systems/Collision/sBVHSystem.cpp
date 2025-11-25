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

    // Implementation of the system's main update logic
    void sBVHSystem::onUpdate(AppTiming timing, entt::registry& registry)
    {
         // Get required services
         auto sceneService = getServices()->get<Scene::SceneManager>();
          if (!sceneService) {
             PN_CORE_WARN("Scene service not found in BVH System. Cannot process meshes.");
             return;
         }

        // --- Phase 1: Update World AABBs and Collect Items ---
        std::vector<std::pair<entt::entity, AABB>> bvhItems;
        
        // Estimate reservation based on the number of entities with a Transform component
        bvhItems.reserve(registry.storage<WorldTransform>().size());

        // Create a view for entities having a Transform component
        auto view = registry.view<WorldTransform>();

        for (auto entity : view) {
             auto& transform = view.get<WorldTransform>(entity); // Get transform component
             BoundingVolume* bvComponent = registry.try_get<BoundingVolume>(entity); // Try to get existing BV component

             // If no BV component, try to create one from ModelRenderer
             if (!bvComponent) {
                 auto* modelRenderer = registry.try_get<ModelRenderer>(entity);
                 if (modelRenderer) { // Check if ModelRenderer mesh_id
                     auto model_opt = services.lock()->get<Assets::Manager>()->getAsset<Assets::Model>(modelRenderer->modelGUID);
                     if (model_opt.has_value()) { // Check if model was found
                        // Add cBoundingVolume component to the entity
                        bvComponent = &registry.emplace<BoundingVolume>(entity);
                        
                        // Calculate local AABB from the model's vertices
                        bvComponent->localAABB = calculateLocalAABB(model_opt.value());
                        
                        bvComponent->needsUpdate = true; // Mark for world AABB update
                     } else {
                         // Mesh ID mesh_id but mesh not loaded/cached, skip entity
                         continue;
                     }
                 } else {
                     // Entity has transform but no BV or ModelRenderer, skip it
                     continue;
                 }
             }

            // Simple update trigger: assume transform changed every frame
            bool transformChanged = true;
            if (transformChanged) {
                 bvComponent->needsUpdate = true;
            }

            // Recalculate world AABB if marked for update
            if (bvComponent->needsUpdate) {
                glm::mat4 worldMatrix = transform.matrix;
                // Transform the local AABB to world space
                bvComponent->worldAABB = bvComponent->localAABB.transform(worldMatrix);
                bvComponent->needsUpdate = false; // Reset flag
            }

             // Add entity and its world AABB to the list for the BVH build input
             bvhItems.push_back({entity, bvComponent->worldAABB});
        }


        // --- Phase 2: Rebuild the BVH Tree ---
         m_bvh.build(bvhItems);

         // --- Phase 3: Update BVH Node Indices in Components ---
         const auto& nodes = m_bvh.getNodes();
         for(int i = 0; i < nodes.size(); ++i) { // Iterate all nodes in the pool
             const auto& node = nodes[i];
             // Check if it's an active leaf node associated with a valid entity
             if (node.isLeaf() && node.height != -1 && registry.valid(node.entity)) {
                 // Update the bvhNodeIndex in the entity's component
                 if (auto* bvComp = registry.try_get<BoundingVolume>(node.entity)) {
                     bvComp->bvhNodeIndex = i; // Store the index of this leaf node
                 }
             }
         }
         // Clear indices for components whose entities were not included in the last build
         auto bvView = registry.view<BoundingVolume>();
         for (auto entity : bvView) {
             bool foundInBvhItems = false;
             for(const auto& item : bvhItems) { // Check if the entity was part of the build input
                 if (item.first == entity) {
                     foundInBvhItems = true;
                     break;
                 }
             }
             if (!foundInBvhItems) { // Reset index if entity wasn't processed
                 bvView.get<BoundingVolume>(entity).bvhNodeIndex = -1;
             }
         }
    } // End of onUpdate

    // Implementation for event handling
    void sBVHSystem::onEvent(Event::Event& e)
    {
        // Placeholder for event responses
    }

} // namespace PAIN