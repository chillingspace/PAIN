
#pragma once
#ifndef PREFAB_SERVICE_HPP
#define PREFAB_SERVICE_HPP

#include "AssetData.h"
#include "ECS/Components/cPrefab.h"
#include "CoreSystems/Assets/Types/Prefab.h"
#include "ECS/Controller.h"

namespace PAIN {
    namespace Prefab {

        class Service {
        private:
            //Access to services
            std::weak_ptr<Services> services;

            //GUID Remap
            std::unordered_map<Assets::GUID, Assets::GUID> createGUIDRemapTable(std::vector<nlohmann::json> entities);
            entt::entity instantiateEntity(const nlohmann::json& entityData, ECS::RegistryID const& registry_id, const std::unordered_map<Assets::GUID, Assets::GUID>& guidRemap, Assets::GUID prefabGUID, Assets::GUID instanceRootGUID);

            // Validate that all referenced GUIDs are in the remap
            bool validateGUIDRemap(
                const std::vector<nlohmann::json>& entities,
                const std::unordered_map<Assets::GUID, Assets::GUID>& guidRemap
            );
        public:

            //Create service
            explicit Service(std::shared_ptr<Services> svc) :services{svc}{}

            //Collect all entities in the hierarchy
            void collectHierarchy(entt::entity root, std::vector<entt::entity>& outEntities, ECS::RegistryID const& registry_id);

            //Serialize a single entity
            nlohmann::json serializeEntity(entt::entity entity, const ECS::RegistryID const& registry_id);

            //Create and save prefab
            void createPrefab(entt::entity rootEntity, const std::string& prefabName, ECS::RegistryID const& registry_id);
            bool savePrefabToFile(Prefab::PrefabAsset const& prefab_asset, const std::string& filePath, ECS::RegistryID const& registry_id);
            nlohmann::json serializePrefab(Prefab::PrefabAsset const& prefab_asset, ECS::RegistryID const& registry_id);

            //Prefab loading and instantiate
            Prefab::PrefabAsset deserializePrefab(const nlohmann::json& prefabJson);
            std::shared_ptr<Prefab::PrefabAsset> loadPrefabFromFile(const std::string& virtual_path);
            entt::entity instantiatePrefab(const Assets::GUID& prefab_asset_id, ECS::RegistryID const& registry_id = ECS::MAIN_REGISTRY_ID, const glm::vec3& position = glm::vec3(0.0f));

            //Prefab overrides
            void applyOverride(entt::entity instanceEntity, const std::string& componentName, const nlohmann::json& overrideData, ECS::RegistryID const& registry_id = ECS::MAIN_REGISTRY_ID);

            //Checks
            bool isInstance(entt::entity entity, ECS::RegistryID const& registry_id) const;
            std::vector<entt::entity> getInstancesOfPrefab(const Assets::GUID& prefab_asset_id, ECS::RegistryID const& registry_id) const;

            //Prefab instances function
            void updateAllInstances(const Assets::GUID& prefabGUID, ECS::RegistryID const& registry_id = ECS::MAIN_REGISTRY_ID, bool preserveOverrides = true);
            void updateSingleInstance(entt::entity instanceRoot, ECS::RegistryID const& registry_id = ECS::MAIN_REGISTRY_ID, bool preserveOverrides = true);

            //Create path service
            static Service* create(std::shared_ptr<Services> service);
        };
    }
}

#endif
