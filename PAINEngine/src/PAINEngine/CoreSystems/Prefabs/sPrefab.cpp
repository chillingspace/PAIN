#include "pch.h"

#include "sPrefab.h"

#include "CoreSystems/Serialization/sSerialization.h"
#include "ECS/Components/AllComponents.h"
#include "ECS/Components/cEntity.h"
#include "ECS/Controller.h"


namespace PAIN {
	namespace Prefab {

		Service* Service::create(std::shared_ptr<Services> service) {
			return new Service(service);
		}

		void Service::collectHierarchy(entt::entity root,
			std::vector<entt::entity>& outEntities,
			ECS::RegistryID const& registry_id) {
			// Get registry
			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(registry_id);

			// Check if entity is valid
			if (!registry.valid(root)) {
				PN_CORE_WARN("Invalid root entity in collectHierarchy");
				return;
			}

			outEntities.push_back(root);

			if (auto* hierarchy = registry.try_get<Entity::Hierarchy>(root)) {
				auto ecs_controller = services.lock()->get<ECS::Controller>();

				// Check if childrenGUIDs vector is valid
				if (hierarchy->childrenGUIDs.empty()) {
					return;
				}

				// Iterate with bounds checking
				for (size_t i = 0; i < hierarchy->childrenGUIDs.size(); ++i) {
					const auto& childGUID = hierarchy->childrenGUIDs[i];

					if (!childGUID.IsValid()) {
						PN_CORE_WARN("Invalid child GUID at index {}", i);
						continue;
					}

					entt::entity child = ecs_controller->getGUIDRegistry(registry_id).resolveGUID(childGUID);

					if (child != entt::null && registry.valid(child)) {
						collectHierarchy(child, outEntities, registry_id);
					}
				}
			}
		}

		nlohmann::json Service::serializeEntity(entt::entity entity,
			ECS::RegistryID const& registry_id) {

			// Get registry
			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(registry_id);

			nlohmann::json entityJson;
			// Serialize GUID at top-level
			const auto& guid = registry.get<Entity::GUID>(entity).guid;
			entityJson["entityGUID"] = guid.ToString();

			// Skip serializing these
			std::unordered_set<std::string> skip_these = {
					getComponentName<Prefab::PrefabInstance>(),
					getComponentName<Entity::GUID>() };

			// Serialize all remaining components (except GUID, WorldTransform,
			// PrefabInstance)
			entityJson["components"] = PAIN::ECS::serializeAllComponentsImpl(
				entity, registry, PAIN::AllGameplayComponents(), skip_these);

			return entityJson;
		}
#ifdef PN_PLATFORM_WINDOWS
		// Creation of prefabs should oly be done in windows, think of release build
		// when running in android
		void Service::createPrefab(entt::entity rootEntity,
			const std::string& prefabName,
			ECS::RegistryID const& registry_id) {

			// Get registry
			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(registry_id);

			// Gather hierarchy
			std::vector<entt::entity> hierarchyEntities;
			collectHierarchy(rootEntity, hierarchyEntities, registry_id);

			// Create prefab
			Prefab::PrefabAsset prefab_asset;
			prefab_asset.rootEntityGUID = registry.get<Entity::GUID>(rootEntity).guid;
			prefab_asset.name = prefabName;
			prefab_asset.prefabName = prefabName;

			// Add entities into prefab
			for (auto e : hierarchyEntities) {
				prefab_asset.entities.push_back(serializeEntity(e, registry_id));
			}

			// Craft path to prefab assets
			auto path_service = services.lock()->get<Path::Path>();
			auto prefab_ext = *Assets::getAllExtensions()[Assets::Type::Prefabs].begin();
			auto prefab_folder = Assets::getAllGameFolders()[Assets::Type::Prefabs];
			std::string virt_path_to_prefab = path_service->aliasCombineRelative(
				Path::main_assets_alias,
				prefab_folder.string() + "/" + prefabName + prefab_ext);

			// Save prefab to file
			savePrefabToFile(prefab_asset, virt_path_to_prefab, registry_id);

			// Register the new asset immediately so it appears in the editor without restart
			auto asset_service = services.lock()->get<Assets::Manager>();
			asset_service->registerAsset(prefab_folder.string() + "/" + prefabName + prefab_ext);
		}
#endif

		nlohmann::json Service::serializePrefab(Prefab::PrefabAsset const& prefab_asset,
			ECS::RegistryID const& registry_id) {
			auto ecs_controller = services.lock()->get<ECS::Controller>();
			entt::entity root = ecs_controller->getGUIDRegistry(registry_id)
				.resolveGUID(prefab_asset.rootEntityGUID);
			if (root == entt::null)
				throw std::runtime_error("Root GUID not found!");

			// Output JSON
			nlohmann::json prefabJson;
			prefabJson["prefabName"] = prefab_asset.prefabName;
			prefabJson["rootEntityGUID"] = prefab_asset.rootEntityGUID.ToString();
			prefabJson["entities"] = prefab_asset.entities;

			std::string parsedJson = prefabJson["entities"].dump(4);

			return prefabJson;
		}

		bool Service::savePrefabToFile(Prefab::PrefabAsset const& prefab_asset,
			const std::string& virtual_path,
			ECS::RegistryID const& registry_id) {
			try {
				nlohmann::json prefabJson = serializePrefab(prefab_asset, registry_id);
				auto path_service = services.lock()->get<Path::Path>();

				auto stream = path_service->createFileStream(virtual_path,
					Path::FileMode::Write, false);
				if (!stream) {
					PN_CORE_ERROR("Failed to create file stream for: {}", virtual_path);
					return false;
				}

				// Write prefab json
				stream->write(prefabJson);

				PN_CORE_INFO("Prefab saved at: {}", virtual_path);
				return true;
			}
			catch (const std::exception& e) {
				PN_CORE_ERROR("Failed to save prefab: {}", e.what());
				return false;
			}
		}

		Prefab::PrefabAsset
			Service::deserializePrefab(const nlohmann::json& prefabJson) {
			Prefab::PrefabAsset prefab_asset;
			try {

				prefab_asset.prefabName = prefabJson.at("prefabName").get<std::string>();
				prefab_asset.rootEntityGUID =
					Assets::GUID(prefabJson.at("rootEntityGUID").get<std::string>());

				prefab_asset.entities.clear();
				if (prefabJson.contains("entities")) {
					for (const auto& entityJson : prefabJson["entities"]) {
						prefab_asset.entities.push_back(entityJson);
					}
				}
			}
			catch (...) {
				PN_CORE_WARN("Invalid Prefab file. Unable to load it");
			}

			return prefab_asset;
		}

		std::shared_ptr<Prefab::PrefabAsset>
			Service::loadPrefabFromFile(const std::string& virtual_path) {

			// Create path service
			auto path_service = services.lock()->get<Path::Path>();

			auto stream =
				path_service->createFileStream(virtual_path, Path::FileMode::Read);
			if (!stream || !stream->good()) {
				PN_CORE_ERROR("Failed to open model file: {}", virtual_path);
				throw std::runtime_error("Failed to open model file: " + virtual_path);
			}

			// Read stream
			nlohmann::json prefabJson = stream->read();

			// Return loaded prefab asset
			return std::make_shared<Prefab::PrefabAsset>(deserializePrefab(prefabJson));
		}

		std::unordered_map<Assets::GUID, Assets::GUID>
			Service::createGUIDRemapTable(std::vector<nlohmann::json> entities) {
			std::unordered_map<Assets::GUID, Assets::GUID> table;
			std::unordered_set<Assets::GUID> usedNewGUIDs; // Detect collisions

			for (const auto& entityData : entities) {
				if (!entityData.contains("entityGUID")) {
					PN_CORE_WARN("Entity missing GUID!");
					continue;
				}

				Assets::GUID oldGUID(entityData["entityGUID"].get<std::string>());
				Assets::GUID newGUID = Assets::GUID::Generate();

				// Check for collision
				int retries = 0;
				while (usedNewGUIDs.find(newGUID) != usedNewGUIDs.end()) {
					PN_CORE_WARN("GUID collision detected! Regenerating...");
					newGUID = Assets::GUID::Generate();
					retries++;

					if (retries > 10) {
						PN_CORE_ERROR("Failed to generate unique GUID after 10 retries!");
						return table; // Return incomplete table
					}
				}

				table[oldGUID] = newGUID;
				usedNewGUIDs.insert(newGUID);
			}

			PN_CORE_INFO("Created GUID remap with {} entries", table.size());

			return table;
		}

		static const nlohmann::json* findRootEntityJson(const nlohmann::json& prefabData,
			const Assets::GUID& rootEntityGUID) {
			for (const auto& entityData : prefabData["entities"]) {
				Assets::GUID guid(entityData["entityGUID"].get<std::string>());
				if (guid == rootEntityGUID)
					return&entityData;
			}
			return nullptr;
		}

		entt::entity Service::instantiateEntity(
			const nlohmann::json& entityData, ECS::RegistryID const& registry_id,
			const std::unordered_map<Assets::GUID, Assets::GUID>& guidRemap,
			Assets::GUID prefabGUID, Assets::GUID instanceRootGUID) {

			// Get controllers and registry
			auto ecs_controller = services.lock()->get<ECS::Controller>();
			entt::registry& registry = ecs_controller->getRegistry(registry_id);

			// ========================================
			// 1. VALIDATE INPUT DATA
			// ========================================
			if (!entityData.contains("entityGUID")) {
				PN_CORE_ERROR("Entity data missing 'entityGUID' field!");
				return entt::null;
			}

			// ========================================
			// 2. GET OLD GUID AND REMAP (SAFE)
			// ========================================
			Assets::GUID oldGUID;
			try {
				oldGUID = Assets::GUID(entityData["entityGUID"].get<std::string>());
			}
			catch (const std::exception& e) {
				PN_CORE_ERROR("Failed to parse entity GUID: {}", e.what());
				return entt::null;
			}

			// SAFE LOOKUP
			auto remapIt = guidRemap.find(oldGUID);
			if (remapIt == guidRemap.end()) {
				PN_CORE_ERROR("GUID not found in remap: {}", oldGUID.ToString());
				return entt::null;
			}

			Assets::GUID newGUID = remapIt->second;

			// ========================================
			// 3. CREATE ENTITY
			// ========================================
			auto entity = ecs_controller->createEntity(newGUID, registry_id);

			if (entity == entt::null) {
				PN_CORE_ERROR("Failed to create entity!");
				return entt::null;
			}

			// ========================================
			// 4. LOAD COMPONENTS
			// ========================================
			try {
				if (entityData.contains("components")) {
					const auto& componentsJson = entityData["components"];
					ecs_controller->loadAllComponentsFromJson(entity, componentsJson,
						registry_id);
				}
			}
			catch (const std::exception& e) {
				PN_CORE_ERROR("Failed to load components: {}", e.what());
				// Continue anyway - entity is created
			}

			// ========================================
			// 5. FIX UP HIERARCHY (SAFE VECTOR OPERATIONS)
			// ========================================
			if (registry.any_of<Entity::Hierarchy>(entity)) {
				auto& hierarchy = registry.get<Entity::Hierarchy>(entity);

				// Remap parent GUID
				if (hierarchy.parentGUID.IsValid()) {
					auto parentRemapIt = guidRemap.find(hierarchy.parentGUID);
					if (parentRemapIt != guidRemap.end()) {
						hierarchy.parentGUID = parentRemapIt->second;
					}
					else {
						PN_CORE_WARN("Parent GUID not in remap: {} - Clearing",
							hierarchy.parentGUID.ToString());
						hierarchy.parentGUID = Assets::GUID();
					}
				}

				// Remap children GUIDs (SAFE: Build new vector)
				std::vector<Assets::GUID> remappedChildren;
				remappedChildren.reserve(hierarchy.childrenGUIDs.size());

				for (size_t i = 0; i < hierarchy.childrenGUIDs.size(); ++i) {
					const auto& oldChildGUID = hierarchy.childrenGUIDs[i];

					auto childRemapIt = guidRemap.find(oldChildGUID);
					if (childRemapIt != guidRemap.end()) {
						remappedChildren.push_back(childRemapIt->second);
					}
					else {
						PN_CORE_WARN("Child GUID not in remap: {} - Skipping",
							oldChildGUID.ToString());
					}
				}

				// Replace with remapped children
				hierarchy.childrenGUIDs = std::move(remappedChildren);
			}

			// ========================================
			// 6. ADD PREFAB INSTANCE COMPONENT
			// ========================================
			try {
				registry.emplace<PrefabInstance>(
					entity, PrefabInstance{ prefabGUID, instanceRootGUID, oldGUID });
			}
			catch (const std::exception& e) {
				PN_CORE_ERROR("Failed to add PrefabInstance component: {}", e.what());
			}

			return entity;
		}

		// In sPrefab.cpp
		bool Service::validateGUIDRemap(
			const std::vector<nlohmann::json>& entities,
			const std::unordered_map<Assets::GUID, Assets::GUID>& guidRemap) {
			bool isValid = true;

			for (const auto& entityData : entities) {
				if (!entityData.contains("components"))
					continue;

				const auto& components = entityData["components"];

				// Check hierarchy references
				if (components.contains("Entity::Hierarchy")) {
					const auto& hierarchyJson = components["Entity::Hierarchy"];

					// Check parent GUID
					if (hierarchyJson.contains("parentGUID")) {
						std::string parentStr = hierarchyJson["parentGUID"].get<std::string>();
						if (!parentStr.empty() &&
							parentStr != "00000000-0000-0000-0000-000000000000") {
							Assets::GUID parentGUID(parentStr);

							if (guidRemap.find(parentGUID) == guidRemap.end()) {
								PN_CORE_WARN("Parent GUID not in remap: {}", parentGUID.ToString());
								// This might be okay if parent is external to prefab
							}
						}
					}

					// Check children GUIDs
					if (hierarchyJson.contains("childrenGUIDs") &&
						hierarchyJson["childrenGUIDs"].is_array()) {
						for (const auto& childGUIDJson : hierarchyJson["childrenGUIDs"]) {
							std::string childStr = childGUIDJson.get<std::string>();
							if (!childStr.empty() &&
								childStr != "00000000-0000-0000-0000-000000000000") {
								Assets::GUID childGUID(childStr);

								if (guidRemap.find(childGUID) == guidRemap.end()) {
									PN_CORE_ERROR("Child GUID not in remap: {}",
										childGUID.ToString());
									isValid = false;
								}
							}
						}
					}
				}
			}

			return isValid;
		}

		entt::entity Service::instantiatePrefab(const Assets::GUID& prefab_asset_id,
			ECS::RegistryID const& registry_id,
			const glm::vec3& position) {
			PN_CORE_INFO("=== INSTANTIATING PREFAB ===");

			// Get registry
			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(registry_id);

			// ========================================
			// 1. LOAD PREFAB ASSET
			// ========================================
			auto asset_service = services.lock()->get<Assets::Manager>();
			auto prefab_asset_opt =
				asset_service->getAsset<Prefab::PrefabAsset>(prefab_asset_id);

			if (!prefab_asset_opt.has_value()) {
				PN_CORE_ERROR("Prefab asset not found: {}", prefab_asset_id.ToString());
				return entt::null;
			}

			std::shared_ptr<Prefab::PrefabAsset> prefab_asset = prefab_asset_opt.value();

			// ========================================
			// 2. VALIDATE PREFAB DATA
			// ========================================
			if (prefab_asset->entities.empty()) {
				PN_CORE_ERROR("Prefab '{}' has no entities!", prefab_asset->prefabName);
				return entt::null;
			}

			if (!prefab_asset->rootEntityGUID.IsValid()) {
				PN_CORE_ERROR("Prefab '{}' has invalid root GUID!",
					prefab_asset->prefabName);
				return entt::null;
			}

			PN_CORE_INFO("Prefab: '{}' with {} entities", prefab_asset->prefabName,
				prefab_asset->entities.size());

			// ========================================
			// 3. CREATE GUID REMAP
			// ========================================
			auto guidRemap = createGUIDRemapTable(prefab_asset->entities);

			if (guidRemap.empty()) {
				PN_CORE_ERROR("Failed to create GUID remap table!");
				return entt::null;
			}

			// Verify root GUID is in remap
			if (guidRemap.find(prefab_asset->rootEntityGUID) == guidRemap.end()) {
				PN_CORE_ERROR("Root GUID not in remap table: {}",
					prefab_asset->rootEntityGUID.ToString());
				return entt::null;
			}

			Assets::GUID instanceRootGUID = guidRemap[prefab_asset->rootEntityGUID];

			// ========================================
			// 4. INSTANTIATE ALL ENTITIES (SAFE)
			// ========================================
			std::unordered_map<Assets::GUID, entt::entity> guidToEntity;
			std::vector<entt::entity> createdEntities;

			// Reserve space to prevent reallocation
			createdEntities.reserve(prefab_asset->entities.size());
			guidToEntity.reserve(prefab_asset->entities.size());

			for (size_t i = 0; i < prefab_asset->entities.size(); ++i) {
				try {
					// SAFE: Bounds-checked access
					if (i >= prefab_asset->entities.size()) {
						PN_CORE_ERROR("Index {} out of range (size: {})", i,
							prefab_asset->entities.size());
						break;
					}

					const auto& entityJson = prefab_asset->entities[i];

					auto entity = instantiateEntity(entityJson, registry_id, guidRemap,
						prefab_asset_id, instanceRootGUID);

					if (entity != entt::null && registry.valid(entity)) {
						auto* guidComp = registry.try_get<Entity::GUID>(entity);
						if (guidComp) {
							guidToEntity[guidComp->guid] = entity;
							createdEntities.push_back(entity);
						}
					}
					else {
						PN_CORE_WARN("Failed to create entity {}/{}", i + 1,
							prefab_asset->entities.size());
					}

				}
				catch (const std::out_of_range& e) {
					PN_CORE_ERROR("Out of range error for entity {}: {}", i, e.what());
					throw; // Re-throw to see call stack
				}
				catch (const std::exception& e) {
					PN_CORE_ERROR("Exception instantiating entity {}: {}", i, e.what());
					// Continue with other entities
				}
			}

			if (createdEntities.empty()) {
				PN_CORE_ERROR("No entities were created from prefab!");
				return entt::null;
			}

			PN_CORE_INFO("Created {} entities", createdEntities.size());

			// ========================================
			// 5. FIND ROOT ENTITY (SAFE)
			// ========================================
			Assets::GUID newRootGUID = guidRemap[prefab_asset->rootEntityGUID];

			auto rootIt = guidToEntity.find(newRootGUID);
			if (rootIt == guidToEntity.end()) {
				PN_CORE_ERROR("Root entity not found in created entities!");
				// Return first entity as fallback
				return createdEntities.empty() ? entt::null : createdEntities[0];
			}

			entt::entity rootEntity = rootIt->second;

			if (!registry.valid(rootEntity)) {
				PN_CORE_ERROR("Root entity is invalid!");
				return entt::null;
			}

			// ========================================
			// 6. APPLY POSITION OVERRIDE
			// ========================================
			if (registry.any_of<LocalTransform>(rootEntity)) {
				auto& transform = registry.get<LocalTransform>(rootEntity);
				transform.position = position;
				PN_CORE_INFO("Set root position to ({}, {}, {})", position.x, position.y,
					position.z);
			}

			// ========================================
			// 7. APPLY PREFAB DEFAULT COMPOUND COLLIDER
			// ========================================
			if (!prefab_asset->defaultColliderShapes.empty()) {
				// Ensure root entity has CompoundCollider component
				if (!registry.any_of<CompoundCollider>(rootEntity)) {
					registry.emplace<CompoundCollider>(rootEntity);
				}
				auto& compCollider = registry.get<CompoundCollider>(rootEntity);
				compCollider.shapes = prefab_asset->defaultColliderShapes;
				compCollider.useCompoundCollider = true;
				PN_CORE_INFO("Applied {} default collider shapes from prefab",
					prefab_asset->defaultColliderShapes.size());
			}

			PN_CORE_INFO("=== PREFAB INSTANTIATION COMPLETE ===");
			return rootEntity;
		}

		void Service::applyOverride(entt::entity instanceEntity,
			const std::string& componentName,
			const nlohmann::json& overrideData,
			ECS::RegistryID const& registry_id) {
			// Get registry
			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(registry_id);
			if (!registry.any_of<PrefabInstance>(instanceEntity))
				return;
			auto& instance = registry.get<PrefabInstance>(instanceEntity);
			instance.componentOverrides[componentName] = overrideData;
			// Immediately apply the override if you want:
			auto ecs_controller = services.lock()->get<ECS::Controller>();
			ecs_controller->loadAllComponentsFromJson(instanceEntity, overrideData,
				registry_id);
		}

		bool Service::isInstance(entt::entity entity,
			ECS::RegistryID const& registry_id) const {
			return services.lock()
				->get<ECS::Controller>()
				->getRegistry(registry_id)
				.any_of<PrefabInstance>(entity);
		}

		std::vector<entt::entity>
			Service::getInstancesOfPrefab(const Assets::GUID& prefabGUID,
				ECS::RegistryID const& registry_id) const {
			// Get registry
			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(registry_id);

			std::vector<entt::entity> result;
			auto view = registry.view<PrefabInstance>();
			for (auto e : view) {
				if (view.get<PrefabInstance>(e).sourcePrefabGUID == prefabGUID) {
					result.push_back(e);
				}
			}
			return result;
		}

		void Service::updateAllInstances(const Assets::GUID& prefabGUID,
			ECS::RegistryID const& registry_id,
			bool preserveOverrides) {

			PN_CORE_INFO("[PrefabService] Updating all instances of prefab: {}",
				prefabGUID.ToString());
			auto instances = getInstancesOfPrefab(prefabGUID, registry_id);

			for (auto instanceEntity : instances) {
				updateSingleInstance(instanceEntity, registry_id, preserveOverrides);
			}
			PN_CORE_INFO("[PrefabService] Updated {} instances", instances.size());
		}

		void Service::updateSingleInstance(entt::entity instanceRoot,
			ECS::RegistryID const& registry_id,
			bool preserveOverrides) {

			// Get registry
			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(registry_id);

			if (!registry.all_of<PrefabInstance>(instanceRoot)) {
				PN_CORE_WARN("[PrefabService] Entity is not a prefab instance");
				return;
			}
			auto& prefabInst = registry.get<PrefabInstance>(instanceRoot);
			auto savedOverrides = prefabInst.componentOverrides; // Backup overrides
			// Get fresh prefab data
			auto asset_service = services.lock()->get<Assets::Manager>();
			auto prefabAssetOpt =
				asset_service->getAsset<PrefabAsset>(prefabInst.sourcePrefabGUID);
			if (!prefabAssetOpt.has_value()) {
				PN_CORE_ERROR("[PrefabService] Source prefab not found: {}",
					prefabInst.sourcePrefabGUID.ToString());
				return;
			}
			auto prefabAsset = prefabAssetOpt.value();
			// Find corresponding entity data in prefab
			nlohmann::json* entityData = nullptr;
			for (auto& entJson : prefabAsset->entities) {
				Assets::GUID entGUID(entJson["entityGUID"].get<std::string>());
				if (entGUID == prefabInst.correspondingPrefabEntityGUID) {
					entityData = &entJson;
					break;
				}
			}
			if (!entityData || !entityData->contains("components")) {
				PN_CORE_ERROR("[PrefabService] Could not find entity data in prefab");
				return;
			}
			auto ecs_controller = services.lock()->get<ECS::Controller>();
			// Apply fresh prefab component data, but skip Entity::Hierarchy and Entity::GUID.
			// Hierarchy GUIDs are remapped per-instance during instantiation and must not be
			// overwritten with stale prefab GUIDs (which would reset child entity references).
			auto componentsJson = (*entityData)["components"];
			componentsJson.erase(getComponentName<Entity::Hierarchy>());
			componentsJson.erase(getComponentName<Entity::GUID>());
			ecs_controller->loadAllComponentsFromJson(
				instanceRoot, componentsJson, registry_id);
			// Re-apply overrides if requested
			if (preserveOverrides && !savedOverrides.empty()) {
				for (const auto& [componentName, overrideData] : savedOverrides) {
					// Check if component still exists
					if (ecs_controller->hasComponentByName(instanceRoot, componentName,
						registry_id)) {
						ecs_controller->loadAllComponentsFromJson(instanceRoot, overrideData,
							registry_id);
					}
				}

				// Restore overrides map
				prefabInst.componentOverrides = savedOverrides;
			}
			PN_CORE_INFO("[PrefabService] Updated instance: {}",
				static_cast<uint32_t>(instanceRoot));
		}

		entt::entity
			Service::loadPrefabForEditing(const Assets::GUID& prefabAssetGUID,
				ECS::RegistryID const& editRegistryID) {
			PN_CORE_INFO("=== LOADING PREFAB FOR EDITING (PRESERVE GUIDs) ===");

			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(editRegistryID);
			auto assetManager = services.lock()->get<Assets::Manager>();

			// Load prefab asset
			auto prefabAssetOpt =
				assetManager->getAsset<Prefab::PrefabAsset>(prefabAssetGUID);
			if (!prefabAssetOpt.has_value()) {
				PN_CORE_ERROR("Prefab asset not found: {}", prefabAssetGUID.ToString());
				return entt::null;
			}

			auto prefabAsset = prefabAssetOpt.value();

			if (prefabAsset->entities.empty()) {
				PN_CORE_ERROR("Prefab has no entities!");
				return entt::null;
			}

			PN_CORE_INFO(
				"Loading prefab '{}' with {} entities (PRESERVING ORIGINAL GUIDs)",
				prefabAsset->prefabName, prefabAsset->entities.size());

			// NO GUID REMAP - Use original GUIDs from prefab file
			std::vector<entt::entity> loadedEntities;

			for (const auto& entityData : prefabAsset->entities) {
				if (!entityData.contains("entityGUID")) {
					PN_CORE_WARN("Entity missing GUID, skipping");
					continue;
				}

				// Use ORIGINAL GUID from prefab (no remapping!)
				Assets::GUID originalGUID(entityData["entityGUID"].get<std::string>());

				// Create entity with original GUID
				auto ecsController = services.lock()->get<ECS::Controller>();
				entt::entity entity =
					ecsController->createEntity(originalGUID, editRegistryID);

				if (entity == entt::null) {
					PN_CORE_ERROR("Failed to create entity with GUID {}",
						originalGUID.ToString());
					continue;
				}

				// Load components
				if (entityData.contains("components") &&
					entityData["components"].is_object()) {
					ecsController->loadAllComponentsFromJson(entity, entityData["components"],
						editRegistryID);
				}

				loadedEntities.push_back(entity);

				PN_CORE_TRACE("Loaded entity {} with ORIGINAL GUID {}",
					static_cast<uint32_t>(entity), originalGUID.ToString());
			}

			// Fix up hierarchy (GUIDs should already match since we used originals)
			for (size_t i = 0; i < loadedEntities.size(); ++i) {
				auto entity = loadedEntities[i];

				if (registry.any_of<Entity::Hierarchy>(entity)) {
					auto& hierarchy = registry.get<Entity::Hierarchy>(entity);

					// Hierarchy GUIDs should already be correct since we used original GUIDs
					// No remapping needed!

					PN_CORE_TRACE("Entity {} hierarchy: parent={}, children={}",
						static_cast<uint32_t>(entity),
						hierarchy.parentGUID.ToString(),
						hierarchy.childrenGUIDs.size());
				}
			}

			// Find and return root entity
			auto ecsController = services.lock()->get<ECS::Controller>();
			entt::entity rootEntity = ecsController->getGUIDRegistry(editRegistryID)
				.resolveGUID(prefabAsset->rootEntityGUID);

			if (rootEntity == entt::null) {
				PN_CORE_ERROR("Failed to find root entity with GUID {}",
					prefabAsset->rootEntityGUID.ToString());

				// Return first entity as fallback
				return loadedEntities.empty() ? entt::null : loadedEntities[0];
			}

			PN_CORE_INFO("Successfully loaded prefab for editing, root entity: {}",
				static_cast<uint32_t>(rootEntity));

			return rootEntity;
		}

		bool Service::isComponentOverridden(entt::entity instance,
			const std::string& componentName,
			ECS::RegistryID registryID) const {
			auto ecsController = services.lock()->get<ECS::Controller>();
			auto& registry = ecsController->getRegistry(registryID);

			// Check if entity has PrefabInstance component
			auto* prefabInst = registry.try_get<PrefabInstance>(instance);
			if (!prefabInst)
				return false;

			// Skip non-serializable components
			std::unordered_set<std::string> skipComponents = {
					getComponentName<Prefab::PrefabInstance>(),
					getComponentName<Entity::GUID>(), getComponentName<WorldTransform>() };

			if (skipComponents.find(componentName) != skipComponents.end()) {
				return false;
			}

			// Get current component value from instance
			nlohmann::json currentComponentJson =
				ecsController->getComponentAsJson(instance, componentName, registryID);

			if (currentComponentJson.empty()) {
				// Component doesn't exist on instance
				return false;
			}

			// Get original component value from prefab
			nlohmann::json prefabComponentJson = getComponentFromPrefab(
				prefabInst->sourcePrefabGUID, prefabInst->correspondingPrefabEntityGUID,
				componentName);

			if (prefabComponentJson.empty()) {
				// Component doesn't exist in prefab
				// This could mean it was added to the instance (which is an override!)
				return true;
			}

			// Compare JSON values
			bool isDifferent = (currentComponentJson != prefabComponentJson);

			return isDifferent;
		}

		nlohmann::json
			Service::getComponentFromPrefab(const Assets::GUID& prefabGUID,
				const Assets::GUID& entityGUID,
				const std::string& componentName) const {
			// Load prefab
			auto assetManager = services.lock()->get<Assets::Manager>();
			auto prefabOpt = assetManager->getAsset<PrefabAsset>(prefabGUID);

			if (!prefabOpt.has_value()) {
				return nlohmann::json();
			}

			auto prefab = prefabOpt.value();

			// Find entity in prefab
			for (const auto& entityData : prefab->entities) {
				if (entityData.contains("entityGUID")) {
					Assets::GUID guid(entityData["entityGUID"].get<std::string>());
					if (guid == entityGUID) {
						// Found the entity, extract component
						if (entityData.contains("components") &&
							entityData["components"].contains(componentName)) {
							return entityData["components"][componentName];
						}
					}
				}
			}

			return nlohmann::json();
		}

		bool Service::detectAndSaveOverride(entt::entity instance,
			const std::string& componentName,
			ECS::RegistryID registryID) {
			auto ecsController = services.lock()->get<ECS::Controller>();
			auto& registry = ecsController->getRegistry(registryID);

			// Get PrefabInstance component
			auto* prefabInst = registry.try_get<PrefabInstance>(instance);
			if (!prefabInst) {
				PN_CORE_WARN("Entity is not a prefab instance");
				return false;
			}

			// Get current component value from instance
			nlohmann::json instanceComponentJson =
				ecsController->getComponentAsJson(instance, componentName, registryID);

			if (instanceComponentJson.empty()) {
				PN_CORE_WARN("Component '{}' not found on instance", componentName);
				return false;
			}

			// Get original component value from prefab
			nlohmann::json prefabComponentJson = getComponentFromPrefab(
				prefabInst->sourcePrefabGUID, prefabInst->correspondingPrefabEntityGUID,
				componentName);

			// Compare
			if (instanceComponentJson == prefabComponentJson) {
				// No difference - remove override if it exists
				prefabInst->componentOverrides.erase(componentName);
				PN_CORE_TRACE("Component '{}' matches prefab, override removed",
					componentName);
				return false;
			}
			else {
				// Different - save as override
				prefabInst->componentOverrides[componentName] = instanceComponentJson;
				PN_CORE_INFO("Component '{}' overridden", componentName);
				return true;
			}
		}

		void Service::updateAllOverrides(entt::entity instance,
			ECS::RegistryID registryID) {
			auto ecsController = services.lock()->get<ECS::Controller>();
			auto& registry = ecsController->getRegistry(registryID);

			// Verify this is a prefab instance
			auto* prefabInst = registry.try_get<PrefabInstance>(instance);
			if (!prefabInst)
				return;

			// Get all component names on this entity
			auto componentNames = ecsController->getComponentNames(instance, registryID);

			// Skip these components (metadata, not user-editable)
			std::unordered_set<std::string> skipComponents = {
					getComponentName<Prefab::PrefabInstance>(),
					getComponentName<Entity::GUID>(), getComponentName<WorldTransform>() };

			// Check each component
			for (const auto& componentName : componentNames) {
				if (skipComponents.find(componentName) != skipComponents.end()) {
					continue;
				}

				detectAndSaveOverride(instance, componentName, registryID);
			}

			PN_CORE_TRACE("Updated overrides for entity {}, {} overrides found",
				static_cast<uint32_t>(instance),
				prefabInst->componentOverrides.size());
		}

		bool Service::revertComponentOverride(entt::entity instance,
			const std::string& componentName,
			ECS::RegistryID registryID) {
			auto ecsController = services.lock()->get<ECS::Controller>();
			auto& registry = ecsController->getRegistry(registryID);

			// Get PrefabInstance
			auto* prefabInst = registry.try_get<PrefabInstance>(instance);
			if (!prefabInst) {
				PN_CORE_ERROR("Entity is not a prefab instance");
				return false;
			}

			// Get original component from prefab
			nlohmann::json prefabComponentJson = getComponentFromPrefab(
				prefabInst->sourcePrefabGUID, prefabInst->correspondingPrefabEntityGUID,
				componentName);

			if (prefabComponentJson.empty()) {
				PN_CORE_ERROR("Component '{}' not found in prefab", componentName);
				return false;
			}

			// Load prefab component value onto instance
			nlohmann::json componentsJson;
			componentsJson[componentName] = prefabComponentJson;
			ecsController->loadAllComponentsFromJson(instance, componentsJson,
				registryID);

			// Remove from overrides
			prefabInst->componentOverrides.erase(componentName);

			PN_CORE_INFO("Reverted component '{}' to prefab value", componentName);
			return true;
		}

		void Service::revertAllOverrides(entt::entity instance,
			ECS::RegistryID registryID) {
			auto& registry =
				services.lock()->get<ECS::Controller>()->getRegistry(registryID);

			auto* prefabInst = registry.try_get<PrefabInstance>(instance);
			if (!prefabInst)
				return;

			// Get list of overridden components
			std::vector<std::string> overriddenComponents;
			for (const auto& [compName, _] : prefabInst->componentOverrides) {
				overriddenComponents.push_back(compName);
			}

			// Revert each one
			for (const auto& compName : overriddenComponents) {
				revertComponentOverride(instance, compName, registryID);
			}

			PN_CORE_INFO("Reverted all {} overrides for entity {}",
				overriddenComponents.size(), static_cast<uint32_t>(instance));
		}
	} // namespace Prefab
} // namespace PAIN
