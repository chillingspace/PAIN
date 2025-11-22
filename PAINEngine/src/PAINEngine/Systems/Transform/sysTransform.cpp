#include "pch.h"
#include "sysTransform.h"

namespace PAIN {
    namespace Transform {

        void System::onUpdate(AppTiming timing, entt::registry& registry) {
            auto view = registry.view<WorldTransform, LocalTransform, Entity::GUID, Entity::Hierarchy>();

            //View each entity to update
            for (auto [entity, world, local, guid, hierarchy] : view.each()) {
                if (!hierarchy.parentGUID.IsValid()) {
                    updateRecursive(entity, glm::mat4(1.0f), registry);
                }
            }
        }

        void System::updateRecursive(entt::entity e, const glm::mat4& parentWorld, entt::registry& registry) {
            auto& local = registry.get<LocalTransform>(e);
            auto& world = registry.get<WorldTransform>(e);

            // Only update if dirty
            if (!world.dirty)
                return;

            glm::mat4 localMat = glm::translate(glm::mat4(1.0f), local.position)
                * glm::mat4(local.rotation)
                * glm::scale(glm::mat4(1.0f), local.scale);

            world.matrix = parentWorld * localMat;
            world.dirty = false;

            // Recursively update children
            if (auto* hierarchy = registry.try_get<Entity::Hierarchy>(e)) {
                for (const auto& childGUID : hierarchy->childrenGUIDs) {
                    entt::entity childEntity = services.lock()->get<ECS::Controller>()->getGUIDRegistry().resolveGUID(childGUID);
                    if (childEntity != entt::null && registry.valid(childEntity)) {
                        updateRecursive(childEntity, world.matrix, registry);
                    }
                }
            }
        }

        void System::markDirty(entt::entity e, entt::registry& registry) {
            auto* world = registry.try_get<WorldTransform>(e);
            if (world)
                world->dirty = true;

            if (auto* hierarchy = registry.try_get<Entity::Hierarchy>(e)) {
                for (const auto& childGUID : hierarchy->childrenGUIDs) {
                    entt::entity child = services.lock()->get<ECS::Controller>()->getGUIDRegistry().resolveGUID(childGUID);
                    if (child != entt::null && registry.valid(child)) {
                        markDirty(child, registry);
                    }
                }
            }
        }

        void System::setParent(entt::entity child, entt::entity parent, entt::registry& registry) {
            auto& guidParent = registry.get<Entity::GUID>(parent);
            auto& guidChild = registry.get<Entity::GUID>(child);

            // Remove from old parent's children if any
            if (auto* hierarchy = registry.try_get<Entity::Hierarchy>(child)) {
                if (hierarchy->parentGUID.IsValid()) {
                    entt::entity oldParent = services.lock()->get<ECS::Controller>()->getGUIDRegistry().resolveGUID(hierarchy->parentGUID);
                    if (oldParent != entt::null) {
                        if (auto* oldParentHierarchy = registry.try_get<Entity::Hierarchy>(oldParent)) {
                            auto& v = oldParentHierarchy->childrenGUIDs;
                            v.erase(std::remove(v.begin(), v.end(), guidChild.guid), v.end());
                        }
                    }
                }
                hierarchy->parentGUID = guidParent.guid;
            }
            else {
                registry.emplace<Entity::Hierarchy>(child, Entity::Hierarchy{ guidParent.guid });
            }

            // Add to new parent's children
            if (auto* parentHierarchy = registry.try_get<Entity::Hierarchy>(parent)) {
                parentHierarchy->childrenGUIDs.push_back(guidChild.guid);
            }
            else {
                registry.emplace<Entity::Hierarchy>(parent, Entity::Hierarchy{ Assets::GUID(), {guidChild.guid}});
            }

            markDirty(child, registry);
        }

        void System::removeParent(entt::entity child, entt::registry& registry) {
            auto* hierarchy = registry.try_get<Entity::Hierarchy>(child);
            if (!hierarchy)
                return;

            // Remove from current parent's children list
            if (hierarchy->parentGUID.IsValid()) {
                entt::entity parent = services.lock()->get<ECS::Controller>()->getGUIDRegistry().resolveGUID(hierarchy->parentGUID);
                if (parent != entt::null) {
                    if (auto* parentHierarchy = registry.try_get<Entity::Hierarchy>(parent)) {
                        auto& v = parentHierarchy->childrenGUIDs;
                        v.erase(std::remove(v.begin(), v.end(), registry.get<Entity::GUID>(child).guid), v.end());
                    }
                }
            }

            hierarchy->parentGUID = Assets::GUID();
            markDirty(child, registry);
        }

        void System::propagateDirty(entt::entity e, entt::registry& registry) {
            //Mark world transform as dirty
            if (auto* world = registry.try_get<WorldTransform>(e)) {
                world->dirty = true;
            }

            //Get hierarchy and propagate to children if exists
            if (auto* hierarchy = registry.try_get<Entity::Hierarchy>(e)) {
                for (const auto& childGUID : hierarchy->childrenGUIDs) {
                    entt::entity child = services.lock()->get<ECS::Controller>()->getGUIDRegistry().resolveGUID(childGUID);
                    if (child != entt::null && registry.valid(child)) {
                        propagateDirty(child, registry);
                    }
                }
            }
        }
    } 
} 
