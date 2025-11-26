/*****************************************************************//**
 * \\file   Controller.cpp
 * \\brief  ECS Controller
 *
 * \\author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \\date   October 2024
 * All content 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/
#include "pch.h"
#include "Controller.h"
#include "ECS/Components/GLMSerialization.h"
 // Systems
#include "Systems/Physics/sysPhysics.h"
#include "Systems/AI/sysAI.h" 
#include "Systems/Animation/sysAnimation.h" 
#include "Systems/Logic/sysLogic.h"
#include "Systems/Audio/sysAudio.h"
#include "Systems/Collision/sBVHSystem.h"
#include "Systems/Scripting/GameScriptingSystem.h"
#include "Systems/Transform/sysTransform.h"
// UI Systems
#include "Systems/UI/sysUILayout.h"
#include "Systems/UI/sysUIInput.h"
#include "Systems/UI/sysUIAnimation.h"
namespace PAIN {
    namespace ECS {
        /*****************************************************************//**
        * EntityGUIDRegistry Implementation
        *********************************************************************/

        Assets::GUID EntityGUIDRegistry::getOrCreateGUID(entt::entity e, entt::registry& registry) {
            // Check if entity already has GUID component
            if (auto* guidComp = registry.try_get<Entity::GUID>(e)) {
                // Update registry mapping
                guid_to_entity[guidComp->guid] = e;
                entity_to_guid[e] = guidComp->guid;
                return guidComp->guid;
            }
            // Generate new GUID
            Assets::GUID newGuid = Assets::GUID::Generate();
            // Add component
            registry.emplace<Entity::GUID>(e, newGuid);
            // Register mapping
            guid_to_entity[newGuid] = e;
            entity_to_guid[e] = newGuid;
            return newGuid;
        }
        entt::entity EntityGUIDRegistry::resolveGUID(const Assets::GUID& guid) const {
            auto it = guid_to_entity.find(guid);
            if (it != guid_to_entity.end()) {
                return it->second;
            }
            return entt::null;
        }
        void EntityGUIDRegistry::remapGUID(const Assets::GUID& oldGuid, const Assets::GUID& newGuid) {
            auto it = guid_to_entity.find(oldGuid);
            if (it != guid_to_entity.end()) {
                entt::entity e = it->second;
                // Remove old mapping
                guid_to_entity.erase(it);
                // Add new mapping
                guid_to_entity[newGuid] = e;
                entity_to_guid[e] = newGuid;
                PN_CORE_INFO("[GUID Registry] Remapped entity {} from {} to {}",
                    static_cast<uint32_t>(e),
                    oldGuid.ToString(),
                    newGuid.ToString());
            }
        }
        void EntityGUIDRegistry::registerEntity(entt::entity e, const Assets::GUID& guid) {
            guid_to_entity[guid] = e;
            entity_to_guid[e] = guid;
        }
        void EntityGUIDRegistry::unregisterEntity(entt::entity e) {
            auto it = entity_to_guid.find(e);
            if (it != entity_to_guid.end()) {
                Assets::GUID guid = it->second;
                guid_to_entity.erase(guid);
                entity_to_guid.erase(it);
            }
        }
        bool EntityGUIDRegistry::hasGUID(const Assets::GUID& guid) const {
            return guid_to_entity.find(guid) != guid_to_entity.end();
        }
        bool EntityGUIDRegistry::hasEntity(entt::entity e) const {
            return entity_to_guid.find(e) != entity_to_guid.end();
        }
        void EntityGUIDRegistry::clear() {
            guid_to_entity.clear();
            entity_to_guid.clear();
        }
        /*****************************************************************//**
        * Controller Implementation
        *********************************************************************/
        Controller::Controller(std::shared_ptr<Services> svc) {
            services = svc;

            // Initialize main registry
            RegistryContext mainContext;
            mainContext.name = "MainRegistry";
            mainContext.auto_simulate = true;  // Main registry always auto-simulates
            registries[MAIN_REGISTRY_ID] = std::move(mainContext);

            PN_CORE_INFO("[ECS Controller] Initialized with main registry (ID: {})", MAIN_REGISTRY_ID);
        }
        /*****************************************************************//**
        * Helper Methods
        *********************************************************************/
        RegistryContext* Controller::getRegistryContext(RegistryID id) {
            auto it = registries.find(id);
            if (it == registries.end()) {
                PN_CORE_ERROR("[ECS Controller] Registry {} not found", id);
                return nullptr;
            }
            return &it->second;
        }
        const RegistryContext* Controller::getRegistryContext(RegistryID id) const {
            auto it = registries.find(id);
            if (it == registries.end()) {
                PN_CORE_ERROR("[ECS Controller] Registry {} not found", id);
                return nullptr;
            }
            return &it->second;
        }
        entt::registry& Controller::getRegistry(RegistryID id) {
            auto* ctx = getRegistryContext(id);
            if (!ctx) {
                PN_CORE_ERROR("[ECS Controller] Returning main registry as fallback");
                return registries[MAIN_REGISTRY_ID].registry;
            }
            return ctx->registry;
        }
        const entt::registry& Controller::getRegistry(RegistryID id) const {
            auto* ctx = getRegistryContext(id);
            if (!ctx) {
                PN_CORE_ERROR("[ECS Controller] Returning main registry as fallback");
                return registries.at(MAIN_REGISTRY_ID).registry;
            }
            return ctx->registry;
        }
        /*****************************************************************//**
        * Registry Management Methods
        *********************************************************************/
        RegistryID Controller::createRegistry(const std::string& name, bool autoSimulate) {
            RegistryID newId = next_registry_id++;

            RegistryContext ctx;
            ctx.name = name.empty() ? ("Registry_" + std::to_string(newId)) : name;
            ctx.auto_simulate = autoSimulate;

            registries[newId] = std::move(ctx);

            PN_CORE_INFO("[ECS Controller] Created registry '{}' (ID: {}, AutoSimulate: {})",
                ctx.name, newId, autoSimulate);

            return newId;
        }
        void Controller::destroyRegistry(RegistryID id) {
            if (id == MAIN_REGISTRY_ID) {
                PN_CORE_ERROR("[ECS Controller] Cannot destroy main registry");
                return;
            }

            auto it = registries.find(id);
            if (it != registries.end()) {
                PN_CORE_INFO("[ECS Controller] Destroying registry '{}' (ID: {})", it->second.name, id);
                registries.erase(it);
            }
            else {
                PN_CORE_WARN("[ECS Controller] Attempted to destroy non-existent registry {}", id);
            }
        }
        bool Controller::hasRegistry(RegistryID id) const {
            return registries.find(id) != registries.end();
        }
        void Controller::setRegistryAutoSimulate(RegistryID id, bool autoSimulate) {
            if (id == MAIN_REGISTRY_ID) {
                PN_CORE_WARN("[ECS Controller] Cannot change auto-simulate for main registry");
                return;
            }

            auto* ctx = getRegistryContext(id);
            if (ctx) {
                ctx->auto_simulate = autoSimulate;
                PN_CORE_INFO("[ECS Controller] Registry {} auto-simulate set to {}", id, autoSimulate);
            }
        }
        bool Controller::isRegistryAutoSimulate(RegistryID id) const {
            auto* ctx = getRegistryContext(id);
            return ctx ? ctx->auto_simulate : false;
        }
        std::string Controller::getRegistryName(RegistryID id) const {
            auto* ctx = getRegistryContext(id);
            return ctx ? ctx->name : "Unknown";
        }
        /*****************************************************************//**
        * GUID Registry Methods
        *********************************************************************/
        EntityGUIDRegistry& Controller::getGUIDRegistry(RegistryID registryId) {
            auto* ctx = getRegistryContext(registryId);
            if (!ctx) {
                PN_CORE_ERROR("[ECS Controller] Returning main GUID registry as fallback");
                return registries[MAIN_REGISTRY_ID].guid_registry;
            }
            return ctx->guid_registry;
        }
        const EntityGUIDRegistry& Controller::getGUIDRegistry(RegistryID registryId) const {
            auto* ctx = getRegistryContext(registryId);
            if (!ctx) {
                PN_CORE_ERROR("[ECS Controller] Returning main GUID registry as fallback");
                return registries.at(MAIN_REGISTRY_ID).guid_registry;
            }
            return ctx->guid_registry;
        }
        Assets::GUID Controller::getOrCreateEntityGUID(entt::entity e, RegistryID registryId) {
            return getGUIDRegistry(registryId).getOrCreateGUID(e, getRegistry(registryId));
        }
        entt::entity Controller::resolveGUID(const Assets::GUID& guid, RegistryID registryId) const {
            return getGUIDRegistry(registryId).resolveGUID(guid);
        }
        int Controller::getEntitiesCount(RegistryID registryId) const {
            auto* ctx = getRegistryContext(registryId);
            return ctx ? static_cast<int>(ctx->entity_count) : 0;
        }
        /*****************************************************************//**
        * Event Dispatching
        *********************************************************************/
        void Controller::dispatchToLayers(Event::Event& e) {
            for (auto& sys : systems) {
                if (sys && sys->enabled) {
                    sys->onEvent(e);
                    if (e.checkHandled()) break;
                }
            }
        }
        void Controller::dispatchToLayersReversed(Event::Event& e) {
            if (systems.empty()) return;
            for (auto it = systems.rbegin(); it != systems.rend(); ++it) {
                if (*it && (*it)->enabled) {
                    (*it)->onEvent(e);
                    if (e.checkHandled()) break;
                }
            }
        }
        /*****************************************************************//**
        * System Update Methods
        *********************************************************************/
        void Controller::updateSystemsForRegistry(RegistryID id, AppTiming timing, bool isFixed) {
            auto* ctx = getRegistryContext(id);
            if (!ctx) return;

            for (auto& sys : systems) {
                if (!sys || !sys->enabled) {
                    continue;
                }
                try {
                    if (isFixed) {
                        sys->onFixedUpdate(timing, ctx->registry);
                    }
                    else {
                        sys->onUpdate(timing, ctx->registry);
                    }
                }
                catch (const std::exception& e) {
                    PN_CORE_ERROR("System '{}' threw exception: {}", sys->getSysName(), e.what());
                    sys->enabled = false;
                }
                catch (...) {
                    PN_CORE_ERROR("System '{}' threw unknown exception!", sys->getSysName());
                    sys->enabled = false;
                }
            }
        }
        void Controller::updateRegistry(RegistryID id, AppTiming timing) {
            updateSystemsForRegistry(id, timing, false);
        }
        void Controller::fixedUpdateRegistry(RegistryID id, AppTiming timing) {
            updateSystemsForRegistry(id, timing, true);
        }
        void Controller::onFixedUpdate(AppTiming timing) {

            // Update main registry (existing behavior)
            updateSystemsForRegistry(MAIN_REGISTRY_ID, timing, true);

            // Update auto-simulated registries
            for (auto& [id, ctx] : registries) {
                if (id != MAIN_REGISTRY_ID && ctx.auto_simulate) {
                    updateSystemsForRegistry(id, timing, true);
                }
            }
        }
        void Controller::onUpdate(AppTiming timing) {
            // Update main registry (existing behavior)
            updateSystemsForRegistry(MAIN_REGISTRY_ID, timing, false);

            // Update auto-simulated registries
            for (auto& [id, ctx] : registries) {
                if (id != MAIN_REGISTRY_ID && ctx.auto_simulate) {
                    updateSystemsForRegistry(id, timing, false);
                }
            }
        }
        /*****************************************************************//**
        * Component and System Registration
        *********************************************************************/
        void Controller::registerAllComponents()
        {
            // Entity components
            registerComponent<Entity::GUID>("GUID");
            registerComponent<Entity::Name>("Name");
            registerComponent<Entity::Hierarchy>("Hierarchy");
            registerComponent<Entity::Layer>("Layer");

            //Register prefab instance
            registerComponent<Prefab::PrefabInstance>("PrefabInstance");
            // Core components
            registerComponent<Cam>("Camera");
            registerComponent<LocalTransform>("LocalTransform");
            registerComponent<WorldTransform>("WorldTransform");
            registerComponent<ModelRenderer>("ModelRenderer");
            registerComponent<Lighting>("Lighting");
            registerComponent<Physics::RigidBody3D>("RigidBody3D");
            registerComponent<Collision::Collider>("Collider");
            registerComponent<Joint>("Joint");
            registerComponent<BoundingVolume>("BoundingVolume");
            registerComponent<Audio::AudioSource>("AudioSource");
            registerComponent<Scripts>("Scripts");
            // AI Components
            registerComponent<AI::Blackboard>("AIBlackboard");
            registerComponent<AI::Controller>("AIController");
            registerComponent<AI::Sensors>("AISensors");
            registerComponent<AI::NavAgent>("AINavAgent");
            registerComponent<AI::Steering>("AISteering");
            registerComponent<AI::CommandQueue>("AICommandQueue");
            // Metadata components
            registerComponent<MetaData::Tag>("Tag");
            registerComponent<MetaData::EditorVisible>("Editor Visiblity");
            // UI components
            registerComponent<UIRectTransform>("UIRectTransform");
            registerComponent<UIButton>("UIButton");
            registerComponent<UIElement>("UIElement");
            registerComponent<UICanvas>("UICanvas");
            registerComponent<UIAnimation>("UIAnimation");
            registerComponent<UIText>("UIText");
        }
        void Controller::registerAllSystems()
        {
            registerSystem<Transform::System>();
            registerSystem<Physics::System>();
            registerSystem<PAIN::Scripting::GameScriptingSystem>();
#ifdef PN_PLATFORM_WINDOWS	
            registerSystem<Animation::System>();
            registerSystem<Audio::System>();
#endif
            registerSystem<sBVHSystem>();
            // UI Systems reg
            registerSystem<UI::LayoutSystem>();
            registerSystem<UI::InputSystem>();
            registerSystem<UI::AnimationSystem>();
        }
        void Controller::onEvent(Event::Event& e) {
            dispatchToLayers(e);
        }
        /*****************************************************************//**
        * Entity Methods
        *********************************************************************/
        entt::entity Controller::createEntity(RegistryID registryId) {
            auto* ctx = getRegistryContext(registryId);
            if (!ctx) return entt::null;

            entt::entity new_entity = ctx->registry.create();
            ctx->entity_count++;
            Assets::GUID guid = Assets::GUID::Generate();
            ctx->registry.emplace<Entity::GUID>(new_entity, guid);
            ctx->registry.emplace<Entity::Layer>(new_entity);
            ctx->guid_registry.registerEntity(new_entity, guid);
            PN_CORE_INFO("Created entity {} with GUID {} in registry {}",
                static_cast<uint32_t>(new_entity),
                guid.ToString(),
                registryId);
            return new_entity;
        }
        entt::entity Controller::createEntity(Assets::GUID const& e_id, RegistryID registryId) {
            auto* ctx = getRegistryContext(registryId);
            if (!ctx) return entt::null;

            entt::entity new_entity = ctx->registry.create();
            ctx->entity_count++;
            ctx->registry.emplace<Entity::GUID>(new_entity, e_id);
            ctx->registry.emplace<Entity::Layer>(new_entity);
            ctx->guid_registry.registerEntity(new_entity, e_id);
            PN_CORE_INFO("Created entity {} with GUID {} in registry {}",
                static_cast<uint32_t>(new_entity),
                e_id.ToString(),
                registryId);
            return new_entity;
        }
        entt::entity Controller::cloneEntity(entt::entity copy, RegistryID srcRegistry, RegistryID dstRegistry) {
            if (!checkEntity(copy, srcRegistry)) {
                PN_CORE_ERROR("Cannot clone invalid entity: {}", static_cast<uint32_t>(copy));
                return entt::null;
            }
            auto* srcCtx = getRegistryContext(srcRegistry);
            auto* dstCtx = getRegistryContext(dstRegistry);
            if (!srcCtx || !dstCtx) return entt::null;
            // Create new entity in destination registry (auto-assigns new GUID)
            entt::entity clone = createEntity(dstRegistry);
            // Copy all components EXCEPT EntityGUID (already has new one)
            for (auto [id, storage] : srcCtx->registry.storage()) {
                if (storage.contains(copy)) {
                    // Skip EntityGUID component (already assigned)
                    if (id == entt::type_hash<Entity::GUID>::value()) {
                        continue;
                    }

                    // Copy component
                    storage.push(clone, storage.value(copy));
                }
            }
            PN_CORE_INFO("Cloned entity {} (registry {}) to {} (registry {}) with new GUID",
                static_cast<uint32_t>(copy), srcRegistry,
                static_cast<uint32_t>(clone), dstRegistry);
            return clone;
        }
        void Controller::destroyEntity(entt::entity entity, RegistryID registryId) {
            if (!checkEntity(entity, registryId)) {
                PN_CORE_ERROR("Attempted to destroy invalid entity: {}",
                    static_cast<uint32_t>(entity));
                return;
            }
            auto* ctx = getRegistryContext(registryId);
            if (!ctx) return;
            ctx->guid_registry.unregisterEntity(entity);
            ctx->registry.destroy(entity);
            ctx->entity_count--;
        }
        bool Controller::checkEntity(entt::entity entity, RegistryID registryId) const {
            return getRegistry(registryId).valid(entity);
        }
        void Controller::destroyAllEntities(RegistryID registryId) {
            auto* ctx = getRegistryContext(registryId);
            if (!ctx) return;

            ctx->registry.clear();
            ctx->entity_count = 0;
            ctx->guid_registry.clear();

            PN_CORE_INFO("Cleared all entities in registry {}", registryId);
        }
        /*****************************************************************//**
        * Component Methods
        *********************************************************************/
        const std::unordered_map<std::string, std::function<void(entt::entity)>>& Controller::getComponentFactories() const {
            return component_factories;
        }
        std::vector<std::string> Controller::getEntityComponentNames(entt::entity entity) const {
            std::vector<std::string> component_names;
            if (!checkEntity(entity, MAIN_REGISTRY_ID)) {
                return component_names;
            }
            // Iterate all registered component checkers
            for (const auto& [name, checker] : component_checkers) {
                if (checker(entity)) {
                    component_names.push_back(name);
                }
            }
            return component_names;
        }
        void Controller::loadAllComponentsFromJson(entt::entity entity, const nlohmann::json& comps) {
            // Note: This uses MAIN_REGISTRY_ID for backward compatibility
            if (!comps.is_object()) {
                PN_CORE_WARN("loadAllComponentsFromJson: Expected object, got {}", comps.type_name());
                return;
            }
            deserializeAllComponentsImpl(entity, getRegistry(MAIN_REGISTRY_ID), comps, AllGameplayComponents{});
        }
        nlohmann::json Controller::getAllComponentsAsJson(entt::entity entity) const {
            // Note: This uses MAIN_REGISTRY_ID for backward compatibility
            if (!checkEntity(entity, MAIN_REGISTRY_ID)) {
                return nlohmann::json::object();
            }
            return serializeAllComponentsImpl(entity, getRegistry(MAIN_REGISTRY_ID), AllGameplayComponents{});
        }
        bool Controller::hasComponentByName(entt::entity entity, const std::string& name) const {
            auto it = component_checkers.find(name);
            if (it == component_checkers.end()) {
                return false;
            }
            return it->second(entity);
        }
        void Controller::removeComponentByName(entt::entity entity, const std::string& name) {
            auto it = component_removers.find(name);
            if (it != component_removers.end()) {
                it->second(entity);
            }
        }
        void* Controller::getComponentPtrByName(entt::entity entity, const std::string& name) {
            auto it = component_getters.find(name);
            if (it == component_getters.end()) {
                return nullptr;
            }
            return it->second(entity);
        }
    }
}
