#pragma once

#ifndef S_BVH_SYSTEM_H
#define S_BVH_SYSTEM_H

// Includes needed for definitions used in this header, placed before namespace
#include "Applications/AppSystem.h" // Defines Services, AppTiming
#include "ECS/System/ISystem.h"     // Defines ECS::System::ISystem base class
#include "CoreSystems/Events/Event.h" // Defines Event::Event
#include "CoreSystems/Collision/BVH.h" // Defines BVH and includes BVHNode, AABB
#include <vector>
#include <utility>
#include <memory>
#include <string>

// Forward declarations for types used as pointers/references
namespace PAIN {
    class Scene;
    struct Mesh;
    struct cTransform;
    struct MeshRenderer;
    struct cBoundingVolume; // Declaration is sufficient here
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

private:
    BVH m_bvh;
    // Parameter types Mesh and AABB should be defined via includes above or pch
    AABB calculateLocalAABB(const std::shared_ptr<Mesh>& mesh);
};

} // namespace PAIN

#endif // S_BVH_SYSTEM_H