#include "pch.h"
#include "sysTransform.h"

namespace PAIN {
	namespace Transform {

		void System::onUpdate(AppTiming timing, entt::registry& registry) {
			//Ensure local with world
			// Use view for single component iteration (faster/safer for exclusion checks)
			auto localOnly = registry.view<LocalTransform>(entt::exclude<WorldTransform>);
			for (auto entity : localOnly) {
				registry.emplace<WorldTransform>(entity);
			}

			//Ensure world with local
			// Use view for single component iteration
			auto worldOnly = registry.view<WorldTransform>(entt::exclude<LocalTransform>);
			for (auto entity : worldOnly) {
				registry.emplace<LocalTransform>(entity); // Default-initialized
			}

			// Use group to iterate entities with all required components for the main update
			auto view = registry.view<LocalTransform, WorldTransform, Entity::GUID, Entity::Hierarchy>();

			//View each entity to update
			for (auto [entity, local, world, guid, hierarchy] : view.each()) {
				if (!hierarchy.parentGUID.IsValid()) {
					updateRecursive(entity, glm::mat4(1.0f), registry);
				}
				else {
					// Entity has a parent - try to find it and register as child
					findOrphansAHome(entity, guid, hierarchy, registry);
				}
			}
		}

		void System::updateRecursive(entt::entity e, const glm::mat4& parentWorld, entt::registry& registry) {
			auto& local = registry.get<LocalTransform>(e);
			auto& world = registry.get<WorldTransform>(e);
			auto* hierarchy = registry.try_get<Entity::Hierarchy>(e);

			//Only update if world or child is dirty
			if (!world.dirty)
				return;

			glm::mat4 localMat = glm::translate(glm::mat4(1.0f), local.position)
				* glm::mat4(local.rotation)
				* glm::scale(glm::mat4(1.0f), local.scale);


			world.matrix = parentWorld * localMat;
			world.dirty = false;

			//Try get BV and mark update needed
			if (auto* bv = registry.try_get<BoundingVolume>(e)) {
				bv->needsUpdate = true;
			}

			// Recursively update children
			if (hierarchy) {
				for (const auto& childGUID : hierarchy->childrenGUIDs) {
					auto ecs = services.lock()->get<ECS::Controller>();
					ECS::RegistryID regID = getRegistryIDFromRef(registry);
					entt::entity childEntity = ecs->getGUIDRegistry(regID).resolveGUID(childGUID);
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

			markAncestorsDirty(e, registry);

			if (auto* hierarchy = registry.try_get<Entity::Hierarchy>(e)) {
				for (const auto& childGUID : hierarchy->childrenGUIDs) {
					auto ecs = services.lock()->get<ECS::Controller>();
					ECS::RegistryID regID = getRegistryIDFromRef(registry);
					entt::entity child = ecs->getGUIDRegistry(regID).resolveGUID(childGUID);
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
					auto ecs = services.lock()->get<ECS::Controller>();
					ECS::RegistryID regID = getRegistryIDFromRef(registry);
					entt::entity oldParent = ecs->getGUIDRegistry(regID).resolveGUID(hierarchy->parentGUID);
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
				registry.emplace<Entity::Hierarchy>(parent, Entity::Hierarchy{ Assets::GUID(), {guidChild.guid} });
			}

			markDirty(child, registry);
		}

		void System::removeParent(entt::entity child, entt::registry& registry) {
			auto* hierarchy = registry.try_get<Entity::Hierarchy>(child);
			if (!hierarchy)
				return;

			// Remove from current parent's children list
			if (hierarchy->parentGUID.IsValid()) {
				auto ecs = services.lock()->get<ECS::Controller>();
				ECS::RegistryID regID = getRegistryIDFromRef(registry);
				entt::entity parent = ecs->getGUIDRegistry(regID).resolveGUID(hierarchy->parentGUID);
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
					auto ecs = services.lock()->get<ECS::Controller>();
					ECS::RegistryID regID = getRegistryIDFromRef(registry);
					entt::entity child = ecs->getGUIDRegistry(regID).resolveGUID(childGUID);
					if (child != entt::null && registry.valid(child)) {
						propagateDirty(child, registry);
					}
				}
			}
		}

		void System::markAncestorsDirty(entt::entity e, entt::registry& registry) {
			auto* hierarchy = registry.try_get<Entity::Hierarchy>(e);
			if (!hierarchy || !hierarchy->parentGUID.IsValid()) {
				return; // No parent, stop
			}

			auto ecs = services.lock()->get<ECS::Controller>();
			ECS::RegistryID regID = getRegistryIDFromRef(registry);
			entt::entity parent = ecs->getGUIDRegistry(regID).resolveGUID(hierarchy->parentGUID);
			if (parent == entt::null || !registry.valid(parent)) {
				return; // Invalid parent
			}

			// Mark parent dirty
			auto* parent_world = registry.try_get<WorldTransform>(parent);
			if (parent_world) {
				parent_world->dirty = true;
			}

			// Recursively mark grandparents, great-grandparents, etc.
			markAncestorsDirty(parent, registry);
		}

		void System::findOrphansAHome(entt::entity e, Entity::GUID const& guid, Entity::Hierarchy& hierarchy, entt::registry& registry) {
			//Get ECS controller to resolve GUID
			auto ecs = services.lock()->get<ECS::Controller>();
			ECS::RegistryID regID = getRegistryIDFromRef(registry);
			entt::entity parentEntity = ecs->getGUIDRegistry(regID).resolveGUID(hierarchy.parentGUID);

			if (parentEntity != entt::null && registry.valid(parentEntity)) {
				//Parent exists - check if parent knows about this child
				auto* parentHierarchy = registry.try_get<Entity::Hierarchy>(parentEntity);

				if (parentHierarchy) {
					//Check if this entity is already in parent's children list
					bool isRegistered = false;
					for (const auto& childGUID : parentHierarchy->childrenGUIDs) {
						if (childGUID == guid.guid) {
							isRegistered = true;
							break;
						}
					}

					// If orphaned (parent doesn't know about child), register it
					if (!isRegistered) {
						parentHierarchy->childrenGUIDs.push_back(guid.guid);
						PN_CORE_WARN("Repaired orphan entity: {} registered to parent: {}",
							guid.guid.ToString(),
							hierarchy.parentGUID.ToString());
					}
				}
			}
			else {
				//arent not found - treat as root for now
				PN_CORE_WARN("Entity {} has invalid parent GUID: {} - treating as root",
					guid.guid.ToString(),
					hierarchy.parentGUID.ToString());

				////Clear invalid parent reference
				//hierarchy.parentGUID = Assets::GUID();

				////Update as root entity
				//updateRecursive(e, glm::mat4(1.0f), registry);

				PN_CORE_WARN("Parent not found for {}, keeping parentGUID={}",
					guid.guid.ToString(), hierarchy.parentGUID.ToString());
				return;
			}
		}
		ECS::RegistryID System::getRegistryIDFromRef(entt::registry& targetReg)
		{
			auto ecs = services.lock()->get<ECS::Controller>();
			for (const auto& id : ecs->getAllRegistryIDs()) {
				if (&ecs->getRegistry(id) == &targetReg) {
					return id;
				}
			}
			// Fallback to main registry if somehow not found
			return ECS::MAIN_REGISTRY_ID;

		}
	}
}
