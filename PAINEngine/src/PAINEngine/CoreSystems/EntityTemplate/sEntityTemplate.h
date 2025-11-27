#pragma once
#ifndef ENTITY_TEMPLATE_SERVICE_HPP
#define ENTITY_TEMPLATE_SERVICE_HPP

#include "AssetData.h"
#include "CoreSystems/Assets/Types/EntityTemplate.h"
#include "ECS/Controller.h"

namespace PAIN {
    namespace EntityTemplate {

        //Entity template service
        class Service {
        private:
            std::weak_ptr<Services> services;

        public:

            //Explicit creation of the entity template
            explicit Service(std::shared_ptr<Services> svc) : services{svc} {}

            // Create template from an existing entity
            Assets::GUID createFromEntity(
                entt::entity entity,
                const std::string& templateName,
                ECS::RegistryID const& registry_id = ECS::MAIN_REGISTRY_ID,
                const std::vector<std::string>& tags = {}
            );

            // Spawn entity from template
            entt::entity spawn(
                const Assets::GUID& templateGUID,
                ECS::RegistryID const& registry_id = ECS::MAIN_REGISTRY_ID,
                const glm::vec3& position = glm::vec3(0.0f),
                const glm::quat& rotation = glm::quat(1, 0, 0, 0),
                const glm::vec3& scale = glm::vec3(1.0f)
            );

            // Save/Load templates
            bool saveTemplateToFile(
                const EntityTemplate::TemplateAsset& templateAsset,
                const std::string& filePath
            );

            std::shared_ptr<EntityTemplate::TemplateAsset> loadTemplateFromFile(
                const std::string& virtual_path
            );

            // Serialize/Deserialize
            nlohmann::json serializeTemplate(
                const EntityTemplate::TemplateAsset& templateAsset
            );

            EntityTemplate::TemplateAsset deserializeTemplate(
                const nlohmann::json& templateJson
            );

            std::shared_ptr<EntityTemplate::TemplateAsset> importTemplate(
                const std::string& sourceFilePath,
                const std::string& newTemplateName
            );

            // Helper: Get all component data from entity
            nlohmann::json getAllComponentsFromEntity(
                entt::entity entity,
                ECS::RegistryID const& registry_id = ECS::MAIN_REGISTRY_ID
            );
        };
    }
}

#endif
