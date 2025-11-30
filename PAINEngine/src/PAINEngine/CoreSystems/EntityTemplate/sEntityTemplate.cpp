#include "pch.h"
#include "sEntityTemplate.h"

#include "ECS/Components/cEntity.h"
#include "ECS/Components/AllComponents.h"
#include "ECS/Controller.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Path/Path.h"

namespace PAIN {
    namespace EntityTemplate {

#ifdef PN_PLATFORM_WINDOWS
        Assets::GUID Service::createFromEntity(
            entt::entity entity,
            const std::string& templateName,
            ECS::RegistryID const& registry_id,
            const std::vector<std::string>& tags
        ) {
            auto ecs_controller = services.lock()->get<ECS::Controller>();
            auto& registry = ecs_controller->getRegistry(registry_id);

            if (!registry.valid(entity)) {
                PN_CORE_ERROR("[EntityTemplate] Invalid entity provided for template creation");
                return Assets::GUID();
            }

            // Get all components from entity
            nlohmann::json componentData = getAllComponentsFromEntity(entity, registry_id);

            // Create template asset
            EntityTemplate::TemplateAsset templateAsset(templateName, std::move(componentData), std::vector<std::string>(tags));

            // Generate GUID for template
            templateAsset.guid = Assets::GUID::Generate();
            templateAsset.name = templateName;

            // Save to file
            auto path_service = services.lock()->get<Path::Path>();

            std::string template_ext = *Assets::getAllExtensions()[Assets::Type::Templates].begin();
            auto template_folder = Assets::getAllGameFolders()[Assets::Type::Templates].string();
            std::string virt_path = path_service->aliasCombineRelative(
                Path::main_assets_alias,
                template_folder + std::string("/") + templateName + template_ext
            );

            if (!saveTemplateToFile(templateAsset, virt_path)) {
                PN_CORE_ERROR("[EntityTemplate] Failed to save template to file");
                return Assets::GUID();
            }

            PN_CORE_INFO("[EntityTemplate] Created template '{}' from entity", templateName);

            return templateAsset.guid;
        }
#endif

        entt::entity Service::spawn(
            const Assets::GUID& templateGUID,
            ECS::RegistryID const& registry_id,
            const glm::vec3& position,
            const glm::quat& rotation,
            const glm::vec3& scale
        ) {
            auto assetManager = services.lock()->get<Assets::Manager>();
            auto ecs_controller = services.lock()->get<ECS::Controller>();
            auto& registry = ecs_controller->getRegistry(registry_id);

            // Get template asset
            auto templateOpt = assetManager->getAsset<EntityTemplate::TemplateAsset>(templateGUID);
            if (!templateOpt.has_value()) {
                PN_CORE_ERROR("[EntityTemplate] Template {} not found", templateGUID.ToString());
                return entt::null;
            }

            auto templateAsset = templateOpt.value();

            // Create new entity
            entt::entity newEntity = ecs_controller->createEntity(registry_id);

            if (newEntity == entt::null) {
                PN_CORE_ERROR("[EntityTemplate] Failed to create entity");
                return entt::null;
            }

            // Load all components from template
            if (templateAsset->componentData.is_object() && !templateAsset->componentData.empty()) {
                ecs_controller->loadAllComponentsFromJson(newEntity, templateAsset->componentData, registry_id);
            }

            // Override transform if provided
            if (registry.any_of<LocalTransform>(newEntity)) {
                auto& transform = registry.get<LocalTransform>(newEntity);
                transform.position = position;
                transform.rotation = rotation;
                transform.scale = scale;
            }
            else {
                // Add transform if not present
                registry.emplace<LocalTransform>(newEntity, position, rotation, scale);
            }

            PN_CORE_INFO("[EntityTemplate] Spawned entity from template '{}'", templateAsset->templateName);

            return newEntity;
        }

        bool Service::saveTemplateToFile(
            const EntityTemplate::TemplateAsset& templateAsset,
            const std::string& filePath
        ) {
            try {
                nlohmann::json templateJson = serializeTemplate(templateAsset);
                auto path_service = services.lock()->get<Path::Path>();

                auto stream = path_service->createFileStream(filePath, Path::FileMode::Write, false);
                if (!stream) {
                    PN_CORE_ERROR("[EntityTemplate] Failed to create file stream for: {}", filePath);
                    return false;
                }

                stream->write(templateJson);

                PN_CORE_INFO("[EntityTemplate] Template saved at: {}", filePath);
                return true;
            }
            catch (const std::exception& e) {
                PN_CORE_ERROR("[EntityTemplate] Failed to save template: {}", e.what());
                return false;
            }
        }

        std::shared_ptr<EntityTemplate::TemplateAsset> Service::loadTemplateFromFile(
            const std::string& virtual_path
        ) {
            try {
                auto path_service = services.lock()->get<Path::Path>();
                auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Read);

                if (!stream) {
                    PN_CORE_ERROR("[EntityTemplate] Failed to open file: {}", virtual_path);
                    return nullptr;
                }

                nlohmann::json templateJson = stream->read();

                EntityTemplate::TemplateAsset templateAsset = deserializeTemplate(templateJson);

                PN_CORE_INFO("[EntityTemplate] Loaded template '{}'", templateAsset.templateName);

                //Return template asset
                return std::make_shared<EntityTemplate::TemplateAsset>(templateAsset);
            }
            catch (const std::exception& e) {
                PN_CORE_ERROR("[EntityTemplate] Failed to load template: {}", e.what());
                return nullptr;
            }
        }

        nlohmann::json Service::serializeTemplate(
            const EntityTemplate::TemplateAsset& templateAsset
        ) {
            nlohmann::json templateJson;
            templateJson["templateName"] = templateAsset.templateName;
            templateJson["guid"] = templateAsset.guid.ToString();
            templateJson["componentData"] = templateAsset.componentData;
            templateJson["tags"] = templateAsset.tags;
            return templateJson;
        }

        EntityTemplate::TemplateAsset Service::deserializeTemplate(
            const nlohmann::json& templateJson
        ) {
            EntityTemplate::TemplateAsset templateAsset;
            templateAsset.templateName = templateJson.value("templateName", "Unnamed");
            templateAsset.guid = Assets::GUID(templateJson.value("guid", ""));
            templateAsset.componentData = templateJson.value("componentData", nlohmann::json::object());
            templateAsset.tags = templateJson.value("tags", std::vector<std::string>());
            templateAsset.name = templateAsset.templateName;
            return templateAsset;
        }

        nlohmann::json Service::getAllComponentsFromEntity(
            entt::entity entity,
            ECS::RegistryID const& registry_id
        ) {
            auto ecs_controller = services.lock()->get<ECS::Controller>();

            // Skip these components (system-managed or auto-generated)
            std::unordered_set<std::string> skip_components = {
                getComponentName<Entity::GUID>(),
                getComponentName<WorldTransform>(),  // Computed
                getComponentName<Prefab::PrefabInstance>()  // Don't include in templates
            };

            nlohmann::json componentData = ecs_controller->getAllComponentsAsJson(entity, registry_id);
            return componentData;
        }
    }
}
