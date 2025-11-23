#include "pch.h"

#include "sPrefab.h"

#include "ECS/Components/cEntity.h"
#include "ECS/Controller.h"
#include "ECS/Components/AllComponents.h"
#include "CoreSystems/Serialization/sSerialization.h"

namespace PAIN {
	namespace Prefab {

        Service* Service::create(std::shared_ptr<Services> service) {
            return new Service(service);
        }

        void Service::collectHierarchy(entt::entity root, entt::registry& registry, std::vector<entt::entity>& outEntities) {
            outEntities.push_back(root);
            if (auto* hierarchy = registry.try_get<Entity::Hierarchy>(root)) {
                auto ecs_controller = services.lock()->get<ECS::Controller>();
                for (const auto& childGUID : hierarchy->childrenGUIDs) {
                    entt::entity child = ecs_controller->getGUIDRegistry().resolveGUID(childGUID);
                    if (child != entt::null && registry.valid(child)) {
                        collectHierarchy(child, registry, outEntities);
                    }
                }
            }
        }

        nlohmann::json Service::serializeEntity(entt::entity entity, const entt::registry& registry) {
            nlohmann::json entityJson;
            // Serialize GUID at top-level
            const auto& guid = registry.get<Entity::GUID>(entity).guid;
            entityJson["entityGUID"] = guid.ToString();

            //Skip serializing these
            std::unordered_set<std::string> skip_these = { getComponentName<Prefab::PrefabInstance>(), getComponentName<Entity::GUID>() };

            // Serialize all remaining components (except GUID, WorldTransform, PrefabInstance)
            entityJson["components"] = PAIN::ECS::serializeAllComponentsImpl(
                entity,
                registry,
                PAIN::AllGameplayComponents(),
                skip_these
            );

            return entityJson;
        }

        void Service::createPrefab(entt::entity rootEntity, const std::string& prefabName, entt::registry& registry) {
            // Gather hierarchy
            std::vector<entt::entity> hierarchyEntities;
            collectHierarchy(rootEntity, registry, hierarchyEntities);

            // Create PrefabAsset on root
            std::vector<nlohmann::json> entities;
            for (auto e : hierarchyEntities) {
                entities.push_back(serializeEntity(e, registry));
            }
            Prefab::PrefabAsset prefab_asset{
                prefabName,
                registry.get<Entity::GUID>(rootEntity).guid,
                entities
            };

            //Craft path to prefab assets
            auto path_service = services.lock()->get<Path::Path>();
            auto prefab_ext = *Assets::getAllExtensions()[Assets::Type::Prefabs].begin();
            auto prefab_folder = Assets::getAllGameFolders()[Assets::Type::Prefabs];
            std::string virt_path_to_prefab = path_service->aliasCombineRelative(Path::main_assets_alias, prefab_folder.string() + "/" + prefabName + prefab_ext);

            //Save prefab to file
            savePrefabToFile(prefab_asset, virt_path_to_prefab, registry);
        }

        nlohmann::json Service::serializePrefab(Prefab::PrefabAsset const& prefab_asset, entt::registry& registry) {
            auto ecs_controller = services.lock()->get<ECS::Controller>();
            entt::entity root = ecs_controller->getGUIDRegistry().resolveGUID(prefab_asset.rootEntityGUID);
            if (root == entt::null) throw std::runtime_error("Root GUID not found!");

            // Output JSON
            nlohmann::json prefabJson;
            prefabJson["prefabName"] = prefab_asset.prefabName;
            prefabJson["rootEntityGUID"] = prefab_asset.rootEntityGUID.ToString();

            // Entities
            for (auto e : prefab_asset.entities) {
                prefabJson["entities"].push_back(e);
            }
            return prefabJson;
        }

        bool Service::savePrefabToFile(Prefab::PrefabAsset const& prefab_asset, const std::string& virtual_path, entt::registry& registry) {
            try {
                nlohmann::json prefabJson = serializePrefab(prefab_asset, registry);
                auto path_service = services.lock()->get<Path::Path>();

                auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Write, false);
                if (!stream) return false;

                //Write prefab json
                stream->write(prefabJson);

                PN_CORE_INFO("Prefab saved at: {}", virtual_path);
            }
            catch (...) {
                return false;
            }
            return false;
        }

        Prefab::PrefabAsset Service::deserializePrefab(const nlohmann::json& prefabJson) {
            Prefab::PrefabAsset prefab_asset;

            prefab_asset.prefabName = prefabJson.at("prefabName").get<std::string>();
            prefab_asset.rootEntityGUID = Assets::GUID(prefabJson.at("rootEntityGUID").get<std::string>());

            prefab_asset.entities.clear();
            if (prefabJson.contains("entities")) {
                for (const auto& entityJson : prefabJson["entities"]) {
                    prefab_asset.entities.push_back(entityJson);
                }
            }

            return prefab_asset;
        }

        std::shared_ptr<Prefab::PrefabAsset> Service::loadPrefabFromFile(const std::string& virtual_path) {

            //Create path service
            auto path_service = services.lock()->get<Path::Path>();

            auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);
            if (!stream || !stream->good()) {
                PN_CORE_ERROR("Failed to open model file: {}", virtual_path);
                throw std::runtime_error("Failed to open model file: " + virtual_path);
            }

            //Read stream
            nlohmann::json prefabJson = stream->read();

            //Return loaded prefab asset
            return std::make_shared<Prefab::PrefabAsset>(deserializePrefab(prefabJson));
        }

        std::unordered_map<Assets::GUID, Assets::GUID> Service::createGUIDRemapTable(std::vector<nlohmann::json> entities) {
            std::unordered_map<Assets::GUID, Assets::GUID> table;
            for (const auto& entityData : entities) {
                Assets::GUID oldGUID(entityData["entityGUID"].get<std::string>());
                table[oldGUID] = Assets::GUID::Generate(); // Every entity instance gets new GUID
            }
            return table;
        }

        // Helper to find root entity JSON object from prefab json
        static const nlohmann::json* findRootEntityJson(
            const nlohmann::json& prefabData,
            const Assets::GUID& rootEntityGUID
        ) {
            for (const auto& entityData : prefabData["entities"]) {
                Assets::GUID guid(entityData["entityGUID"].get<std::string>());
                if (guid == rootEntityGUID)
                    return &entityData;
            }
            return nullptr;
        }

        entt::entity Service::instantiateEntity(
            const nlohmann::json& entityData,
            entt::registry& registry,
            const std::unordered_map<Assets::GUID, Assets::GUID>& guidRemap,
            Assets::GUID prefabGUID,
            Assets::GUID instanceRootGUID
        ) {
            auto ecs_controller = services.lock()->get<ECS::Controller>();
            //Get new GUID for this entity
            Assets::GUID oldGUID(entityData["entityGUID"].get<std::string>());
            Assets::GUID newGUID = guidRemap.at(oldGUID);

            //Create the entity and assign its GUID
            auto entity = registry.create();
            registry.emplace<Entity::GUID>(entity, newGUID);

            //Add all components except WorldTransform and any instance components
            const auto& componentsJson = entityData["components"];
            for (auto it = componentsJson.begin(); it != componentsJson.end(); ++it) {
                const std::string compName = it.key();
                if (compName == getComponentName<Entity::GUID>() || compName == getComponentName<Prefab::PrefabInstance>())
                    continue;

               ecs_controller->loadAllComponentsFromJson(entity, it.value());
            }

            //Fix up hierarchy GUIDs using remap
            if (registry.any_of<Entity::Hierarchy>(entity)) {
                auto& hierarchy = registry.get<Entity::Hierarchy>(entity);
                if (hierarchy.parentGUID.IsValid())
                    hierarchy.parentGUID = guidRemap.at(hierarchy.parentGUID);
                for (auto& childGUID : hierarchy.childrenGUIDs)
                    childGUID = guidRemap.at(childGUID);
            }

            //Add PrefabInstance (all instances, including root and children)
            registry.emplace<PrefabInstance>(entity, PrefabInstance{
                prefabGUID,
                instanceRootGUID,
                oldGUID
                });

            return entity;
        }

        entt::entity Service::instantiatePrefab(const Assets::GUID& prefab_asset_id, entt::registry& registry, const glm::vec3& position) {

            //Get asset service
            auto asset_service = services.lock()->get<Assets::Manager>();

            //Get prefab asset
            auto prefab_asset_opt = asset_service->getAsset<Prefab::PrefabAsset>(prefab_asset_id);

            //Check for valid prefab asset
            if (prefab_asset_opt.has_value()) {
                std::shared_ptr<Prefab::PrefabAsset> prefab_asset = prefab_asset_opt.value();

                //Prepare GUID remap
                auto guidRemap = createGUIDRemapTable(prefab_asset->entities);

                //Create all entities
                std::unordered_map<Assets::GUID, entt::entity> guidToEntity;
                for (const auto& entityJson : prefab_asset->entities) {
                    auto e = instantiateEntity(entityJson, registry, guidRemap, prefab_asset_id, guidRemap[prefab_asset->rootEntityGUID]);
                    guidToEntity[registry.get<Entity::GUID>(e).guid] = e;
                }

                //If position/rotation override is provided, set root
                Assets::GUID newRootGUID = guidRemap[prefab_asset->rootEntityGUID];
                entt::entity rootEntity = guidToEntity[newRootGUID];
                if (registry.any_of<LocalTransform>(rootEntity)) {
                    auto& transform = registry.get<LocalTransform>(rootEntity);
                    transform.position = position;
                }

                return rootEntity;
            }

            PN_CORE_WARN("Prefab not available. Entity not instantiated");
            return entt::entity(0);
        }

        void Service::applyOverride(entt::entity instanceEntity, const std::string& componentName, const nlohmann::json& overrideData, entt::registry& registry) {
            if (!registry.any_of<PrefabInstance>(instanceEntity)) return;
            auto& instance = registry.get<PrefabInstance>(instanceEntity);
            instance.componentOverrides[componentName] = overrideData;
            // Immediately apply the override if you want:
            auto ecs_controller = services.lock()->get<ECS::Controller>();
            ecs_controller->loadAllComponentsFromJson(instanceEntity, overrideData);
        }

        bool Service::isInstance(entt::entity entity, entt::registry& registry) const {
            return registry.any_of<PrefabInstance>(entity);
        }

        std::vector<entt::entity> Service::getInstancesOfPrefab(const Assets::GUID& prefabGUID, entt::registry& registry) const {
            std::vector<entt::entity> result;
            auto view = registry.view<PrefabInstance>();
            for (auto e : view) {
                if (view.get<PrefabInstance>(e).sourcePrefabGUID == prefabGUID) {
                    result.push_back(e);
                }
            }
            return result;
        }
	}
}
