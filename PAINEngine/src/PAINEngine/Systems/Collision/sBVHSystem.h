#pragma once

#ifndef S_BVH_SYSTEM_H
#define S_BVH_SYSTEM_H

// Includes needed for definitions used in this header, placed before namespace
#include "Applications/AppSystem.h" // Defines Services, AppTiming
#include "ECS/System/ISystem.h"     // Defines ECS::System::ISystem base class
#include "CoreSystems/Events/Event.h" // Defines Event::Event
#include "CoreSystems/Collision/BVH.h" // Defines BVH and includes BVHNode, AABB
#include "CoreSystems/Scene/Scene.h"
#include <vector>
#include <utility>
#include <memory>
#include <string>

// Forward declarations for types used as pointers/references within the PAIN namespace
namespace PAIN {
    namespace Assets { class Model; } // Forward-declare Assets::Model
    struct LocalTransform;
    struct ModelRenderer;
    struct cBoundingVolume;
    namespace MetaData { struct EditorVisible; }
} // namespace PAIN


namespace PAIN {

#include "pch.h" // Include pch inside namespace

class sBVHSystem : public ECS::System::ISystem {
public:
    explicit sBVHSystem(std::shared_ptr<Services> svc);
    ~sBVHSystem() override = default;

    std::string getSysName() const override { return "BVH System"; }
    // Parameters use types defined/included above or via pch.h
    void onUpdate(AppTiming timing, entt::registry& reg) override;
    void onEvent(Event::Event& e) override;

    const BVH& getBVH() const { return m_bvh; }


    // Query for collisions with layer filtering
    std::vector<entt::entity> queryAABB(
        const AABB& queryBox,
        entt::registry& registry,
        int queryLayerID = -1  // -1 = ignore layers
    );

    // Raycast with layer filtering
    struct RaycastHit {
        entt::entity entity = entt::null;
        glm::vec3 point;
        glm::vec3 normal;
        float distance = std::numeric_limits<float>::max();
        int layer = 0;
    };

    std::optional<RaycastHit> raycast(
        const glm::vec3& origin,
        const glm::vec3& direction,
        float maxDistance,
        entt::registry& registry,
        int layerMask = -1  // -1 = all layers
    );

private:
    // Track scene state
    size_t m_lastEntityCount = 0;
    bool m_needsFullRebuild = true;

    // Track which entities are in BVH
    std::unordered_set<entt::entity> m_trackedEntities;
    BVH m_bvh;
    std::unordered_set<entt::entity> m_currentFrameEntities;
    std::vector<std::pair<entt::entity, AABB>> m_bvhItems;

    // Helper to compute AABB from model vertices
    AABB calculateLocalAABB(const std::shared_ptr<Assets::Model>& model);

    // Check if collision should happen based on layers
    bool shouldCollide(
        entt::entity entityA,
        entt::entity entityB
    );

    bool rayAABBIntersect(
        const glm::vec3& origin,
        const glm::vec3& direction,
        const AABB& aabb,
        float& tMin,
        float& tMax
    );

    glm::vec3 calculateAABBNormal(const AABB& aabb, const glm::vec3& point);

    // Detect all collisions with layer filtering
    std::vector<std::pair<entt::entity, entt::entity>> detectCollisions(
        entt::registry& registry
    );

};

} // namespace PAIN

#endif // S_BVH_SYSTEM_H