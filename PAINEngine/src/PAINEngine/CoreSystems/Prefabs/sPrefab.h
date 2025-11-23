
#pragma once
#ifndef PREFAB_SERVICE_HPP
#define PREFAB_SERVICE_HPP

#include "AssetData.h"
#include "ECS/Components/cPrefab.h"
#include "CoreSystems/Assets/Types/Prefab.h"

namespace PAIN {
    namespace Prefab {

        class Service {
        private:
            //Access to services
            std::weak_ptr<Services> services;

            //Collect all entities in the hierarchy
            void collectHierarchy(entt::entity root, entt::registry& registry, std::vector<entt::entity>& outEntities);

            //Serialize a single entity
            nlohmann::json serializeEntity(entt::entity entity, const entt::registry& registry);

            //GUID Remap
            std::unordered_map<Assets::GUID, Assets::GUID> createGUIDRemapTable(std::vector<nlohmann::json> entities);
            entt::entity instantiateEntity(const nlohmann::json& entityData, entt::registry& registry, const std::unordered_map<Assets::GUID, Assets::GUID>& guidRemap, Assets::GUID prefabGUID, Assets::GUID instanceRootGUID);

        public:

            //Create service
            explicit Service(std::shared_ptr<Services> svc) :services{svc}{}

            //Create and save prefab
            void createPrefab(entt::entity rootEntity, const std::string& prefabName, entt::registry& registry);
            bool savePrefabToFile(Prefab::PrefabAsset const& prefab_asset, const std::string& filePath, entt::registry& registry);
            nlohmann::json serializePrefab(Prefab::PrefabAsset const& prefab_asset, entt::registry& registry);

            //Prefab loading and instantiate
            Prefab::PrefabAsset deserializePrefab(const nlohmann::json& prefabJson);
            std::shared_ptr<Prefab::PrefabAsset> loadPrefabFromFile(const std::string& virtual_path);
            entt::entity instantiatePrefab(const Assets::GUID& prefab_asset_id, entt::registry& registry, const glm::vec3& position = glm::vec3(0.0f));

            //Prefab overrides
            void applyOverride(entt::entity instanceEntity, const std::string& componentName, const nlohmann::json& overrideData, entt::registry& registry);

            //Checks
            bool isInstance(entt::entity entity, entt::registry& registry) const;
            std::vector<entt::entity> getInstancesOfPrefab(const Assets::GUID& prefab_asset_id, entt::registry& registry) const;

            //Create path service
            static Service* create(std::shared_ptr<Services> service);
        };
    }
}

#endif
