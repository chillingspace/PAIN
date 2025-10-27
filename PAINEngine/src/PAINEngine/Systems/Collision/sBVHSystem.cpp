#include "pch.h" // Must be first

// Include own header
#include "Systems/Collision/sBVHSystem.h" // Adjust path if needed

// Include necessary headers for implementation details
#include "ECS/Controller.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cMeshRenderer.h"
#include "ECS/Components/cBoundingVolume.h"
#include "ECS/Components/cMetadata.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Scene/Scene.h"

// Ensure code is within the namespace
namespace PAIN {

    sBVHSystem::sBVHSystem(std::shared_ptr<Services> svc)
        : ECS::System::ISystem(svc), m_bvh(128) // Initialize base class and BVH
    {
        PN_CORE_INFO("BVH System Initialized.");
    }

    AABB sBVHSystem::calculateLocalAABB(const std::shared_ptr<Mesh>& mesh) {
        AABB localAABB;
        if (!mesh) {
            PN_CORE_WARN("Attempted to calculate AABB for a null mesh. Using default small box.");
            localAABB.min = glm::vec3(-0.01f);
            localAABB.max = glm::vec3(0.01f);
            return localAABB;
        }

        // Assumes Mesh has getVertices() method
        const std::vector<Vertex>& vertices = mesh->getVertices();
        if (vertices.empty()) {
             PN_CORE_WARN("Mesh has no vertices. Using default small box.");
             localAABB.min = glm::vec3(-0.01f);
             localAABB.max = glm::vec3(0.01f);
             return localAABB;
        }

        // Calculate bounds from vertices
        for (const auto& vertex : vertices) {
            localAABB.expand(vertex.pos);
        }

        // Add epsilon for degenerate cases
         glm::vec3 extents = localAABB.getExtents();
         const float minExtent = 0.01f;
         if (extents.x < minExtent) { localAABB.min.x -= minExtent; localAABB.max.x += minExtent; }
         if (extents.y < minExtent) { localAABB.min.y -= minExtent; localAABB.max.y += minExtent; }
         if (extents.z < minExtent) { localAABB.min.z -= minExtent; localAABB.max.z += minExtent; }

        return localAABB;
    }


    void sBVHSystem::onUpdate(AppTiming timing, entt::registry& registry)
    {
         auto sceneService = getServices()->get<Scene>();
          if (!sceneService) {
             PN_CORE_WARN("Scene service not found in BVH System.");
             return;
         }

        // --- Phase 1: Update World AABBs and Collect Items ---
        std::vector<std::pair<entt::entity, AABB>> bvhItems;
        bvhItems.reserve(registry.size<cTransform>() / 2);

        // View entities with cTransform
        auto view = registry.view<cTransform>(/*entt::exclude<MetaData::EditorVisible>*/);

        for (auto entity : view) {
             auto& transform = view.get<cTransform>(entity);
             cBoundingVolume* bvComponent = registry.try_get<cBoundingVolume>(entity);

             // Ensure component exists or create from MeshRenderer
             if (!bvComponent) {
                 auto* meshRenderer = registry.try_get<MeshRenderer>(entity);
                 if (meshRenderer) {
                     auto mesh = sceneService->getMesh(meshRenderer->mesh_id);
                     if (mesh) {
                        bvComponent = &registry.emplace<cBoundingVolume>(entity);
                        bvComponent->localAABB = calculateLocalAABB(mesh);
                        bvComponent->needsUpdate = true;
                     } else {
                         continue; // Skip if mesh not found
                     }
                 } else {
                     continue; // Skip if no BV and no mesh
                 }
             }

            // TODO: Add proper transform changed check
            bool transformChanged = true;
            if (transformChanged) {
                 bvComponent->needsUpdate = true;
            }

            // Update World AABB if needed
            if (bvComponent->needsUpdate) {
                glm::mat4 worldMatrix = transform.getMatrix();
                bvComponent->worldAABB = bvComponent->localAABB.transform(worldMatrix);
                bvComponent->needsUpdate = false;
            }

             bvhItems.push_back({entity, bvComponent->worldAABB});
        }


        // --- Phase 2: Rebuild BVH ---
         m_bvh.build(bvhItems);

         // --- Phase 3: Update node indices in components ---
         const auto& nodes = m_bvh.getNodes();
         for(int i = 0; i < nodes.size(); ++i) {
             const auto& node = nodes[i];
             if (node.isLeaf() && node.height != -1 && registry.valid(node.entity)) {
                 if (auto* bvComp = registry.try_get<cBoundingVolume>(node.entity)) {
                     bvComp->bvhNodeIndex = i;
                 }
             }
         }
         // Clear stale indices
         auto bvView = registry.view<cBoundingVolume>();
         for (auto entity : bvView) {
             bool foundInBvhItems = false;
             for(const auto& item : bvhItems) {
                 if (item.first == entity) {
                     foundInBvhItems = true;
                     break;
                 }
             }
             if (!foundInBvhItems) {
                 bvView.get<cBoundingVolume>(entity).bvhNodeIndex = -1;
             }
         }
    }

    void sBVHSystem::onEvent(Event::Event& e)
    {
        // Placeholder
    }

} // namespace PAIN