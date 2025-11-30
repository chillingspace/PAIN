#include "pch.h"
#include "Scene.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Assets/sAssets.h"
#include "ECS/Controller.h"
#include "ECS/sMetaData.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cEntity.h"
#include "ECS/Components/cMeshRenderer.h"
#include "ECS/Components/cAudioSource.h"
#include "CoreSystems/Renderer/GraphicsSettings.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Renderer/skybox.h"
#include "CoreSystems/Prefabs/sPrefab.h"
#include "ECS/Components/cAnimation.h"
#include "CoreSystems/Renderer/sRenderer.h"

namespace PAIN {
	namespace Scene {

		entt::entity SceneManager::AddObject(const std::shared_ptr<Assets::Model>& mdl, const std::string& name, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale, Assets::GUID const& diff_id, Assets::GUID const& ao_id)
		{
			auto ecs = services->get<ECS::Controller>();
			auto meta = services->get<MetaData::Service>();

			entt::entity entity = ecs->createEntity();
			ecs->addEntityComponent(entity, Entity::Name{ name });
			ecs->addEntityComponent(entity, LocalTransform{ pos, rot, scale });
			ecs->addEntityComponent(entity, WorldTransform{});
			ecs->addEntityComponent(entity, Entity::Hierarchy{});

			// Create ModelRenderer and CACHE the model asset pointer
			ModelRenderer mr = ModelRenderer{ mdl->guid };
			mr.cachedModelAsset = mdl;  // ← ADD THIS LINE!
			ecs->addEntityComponent(entity, static_cast<ModelRenderer>(mr));

			// Add Animation component if model has animations
			if (mdl->animations.size()) {
				Animation anim;
				anim.PlayAnimation(0);  // Start first animation
				ecs->addEntityComponent(entity, static_cast<Animation>(anim));

				PN_CORE_INFO("Added {} to scene with animation playing", mdl->vpath);
			}
			else {
				PN_CORE_INFO("Added {} to scene", mdl->vpath);
			}

			if (meta) meta->setEntityName(entity, name);

			return entity;
		}


		bool SceneManager::buildEntitiesFromAsset(SceneAsset const& scene_asset) {
			if (scene_asset.entityData.empty()) {
				PN_CORE_WARN("[SceneManager] Scene has no entity data");
				return true; // Not an error, just an empty scene
			}
			auto controller = services->get<ECS::Controller>();
			if (!controller) {
				PN_CORE_ERROR("[SceneManager] ECS Controller not available");
				return false;
			}
			auto prefabService = services->get<Prefab::Service>();
			if (!prefabService) {
				PN_CORE_ERROR("[SceneManager] Prefab Service not available");
				return false;
			}
			const auto& ecs = scene_asset.entityData;
			// Find the Entities array
			auto entsIt = ecs.find("Entities");
			if (entsIt == ecs.end() || !entsIt->is_array()) {
				PN_CORE_WARN("[SceneManager] No 'Entities' array in scene data");
				return true;
			}
			// Storage for entity data
			struct EntityLoadInfo {
				entt::entity entity;
				const nlohmann::json* jsonData;
				bool isPrefabInstance;
				Assets::GUID sourcePrefabGUID;
				Assets::GUID correspondingPrefabEntityGUID;
				nlohmann::json componentOverrides;
			};
			std::vector<EntityLoadInfo> entityInfos;
			// ========================================
			// PASS 1: Create entities with GUIDs and identify prefab instances
			// ========================================
			PN_CORE_INFO("[SceneManager] PASS 1: Creating entities with GUIDs");
			for (const auto& ewrap : *entsIt) {
				if (!ewrap.is_object()) continue;
				auto eit = ewrap.find("Entity");
				if (eit == ewrap.end() || !eit->is_object()) continue;
				const auto& E = *eit;
				// Extract GUID from Components
				Assets::GUID entityGuid;
				bool hasGuid = false;
				if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
					if (auto guidIt = compsIt->find("GUID"); guidIt != compsIt->end() && guidIt->is_object()) {
						try {
							Entity::GUID guidComp;
							Serialization::from_json_reflected(guidComp, *guidIt);
							entityGuid = guidComp.guid;
							hasGuid = true;
						}
						catch (const std::exception& ex) {
							PN_CORE_ERROR("[SceneManager] Failed to deserialize GUID: {}", ex.what());
						}
					}
				}
				// Create entity with original GUID if available
				entt::entity e;
				if (hasGuid && entityGuid.IsValid()) {
					e = controller->createEntity(entityGuid);
				}
				else {
					e = controller->createEntity();
					PN_CORE_WARN("[SceneManager] Created entity {} without GUID", static_cast<uint32_t>(e));
				}
				// Check if this is a prefab instance
				EntityLoadInfo info;
				info.entity = e;
				info.jsonData = &E;
				info.isPrefabInstance = false;
				if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
					if (auto prefabInstIt = compsIt->find("PrefabInstance");
						prefabInstIt != compsIt->end() && prefabInstIt->is_object()) {

						try {
							// Extract source prefab GUID
							if (auto srcIt = prefabInstIt->find("sourcePrefabGUID");
								srcIt != prefabInstIt->end() && srcIt->is_string()) {

								info.isPrefabInstance = true;
								info.sourcePrefabGUID = Assets::GUID(srcIt->get<std::string>());

								// Extract corresponding prefab entity GUID
								if (auto corrIt = prefabInstIt->find("correspondingPrefabEntityGUID");
									corrIt != prefabInstIt->end() && corrIt->is_string()) {
									info.correspondingPrefabEntityGUID = Assets::GUID(corrIt->get<std::string>());
								}

								// Extract component overrides
								if (auto overridesIt = prefabInstIt->find("componentOverrides");
									overridesIt != prefabInstIt->end() && overridesIt->is_object()) {
									info.componentOverrides = *overridesIt;
								}
								PN_CORE_INFO("[SceneManager] Entity {} is instance of prefab {}",
									static_cast<uint32_t>(e), info.sourcePrefabGUID.ToString());
							}
						}
						catch (const std::exception& ex) {
							PN_CORE_ERROR("[SceneManager] Failed to parse PrefabInstance: {}", ex.what());
						}
					}
				}
				entityInfos.push_back(info);
			}
			PN_CORE_INFO("[SceneManager] PASS 1 Complete: Created {} entities", entityInfos.size());
			// ========================================
			// PASS 2: Load components
			// ========================================
			PN_CORE_INFO("[SceneManager] PASS 2: Loading components");
			for (auto& info : entityInfos) {
				const auto& E = *info.jsonData;
				if (info.isPrefabInstance) {
					// ========================================
					// PREFAB INSTANCE: Load from prefab + apply overrides
					// ========================================
					PN_CORE_TRACE("[SceneManager] Loading prefab instance entity {}",
						static_cast<uint32_t>(info.entity));
					// Get prefab asset
					auto assetManager = services->get<Assets::Manager>();
					auto prefabAssetOpt = assetManager->getAsset<Prefab::PrefabAsset>(info.sourcePrefabGUID);
					if (!prefabAssetOpt.has_value()) {
						PN_CORE_ERROR("[SceneManager] Prefab {} not found. Loading as regular entity.",
							info.sourcePrefabGUID.ToString());

						// Fallback: Load as regular entity
						if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
							controller->loadAllComponentsFromJson(info.entity, *compsIt);
						}
						continue;
					}
					auto prefabAsset = prefabAssetOpt.value();
					// Find corresponding entity in prefab
					nlohmann::json const* prefabEntityData = nullptr;
					for (const auto& prefabEntity : prefabAsset->entities) {
						if (prefabEntity.contains("entityGUID")) {
							Assets::GUID prefabEntityGUID(prefabEntity["entityGUID"].get<std::string>());
							if (prefabEntityGUID == info.correspondingPrefabEntityGUID) {
								prefabEntityData = &prefabEntity;
								break;
							}
						}
					}
					if (!prefabEntityData) {
						PN_CORE_ERROR("[SceneManager] Entity not found in prefab. Loading as regular entity.");
						if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
							controller->loadAllComponentsFromJson(info.entity, *compsIt);
						}
						continue;
					}
					// Load components from prefab
					if (prefabEntityData->contains("components") && (*prefabEntityData)["components"].is_object()) {
						controller->loadAllComponentsFromJson(info.entity, (*prefabEntityData)["components"]);
						PN_CORE_TRACE("[SceneManager] Loaded {} components from prefab",
							(*prefabEntityData)["components"].size());
					}
					// Apply overrides
					if (!info.componentOverrides.empty()) {
						for (auto it = info.componentOverrides.begin(); it != info.componentOverrides.end(); ++it) {
							const std::string& componentName = it.key();
							const nlohmann::json& overrideData = it.value();
							// Load this specific component (will replace the prefab version)
							nlohmann::json componentJson;
							componentJson[componentName] = overrideData;
							controller->loadAllComponentsFromJson(info.entity, componentJson);
						}
						PN_CORE_TRACE("[SceneManager] Applied {} overrides", info.componentOverrides.size());
					}
					// Re-add PrefabInstance component (reload from scene)
					if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
						if (auto prefabInstIt = compsIt->find("PrefabInstance");
							prefabInstIt != compsIt->end() && prefabInstIt->is_object()) {

							nlohmann::json prefabInstJson;
							prefabInstJson["PrefabInstance"] = *prefabInstIt;
							controller->loadAllComponentsFromJson(info.entity, prefabInstJson);
						}
					}
				}
				else {
					// ========================================
					// REGULAR ENTITY: Load all components from scene
					// ========================================
					if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
						controller->loadAllComponentsFromJson(info.entity, *compsIt);
					}
					// Fallback: Add Name component if not present
					if (!controller->hasEntityComponent<Entity::Name>(info.entity)) {
						if (auto n = E.find("Name"); n != E.end() && n->is_string()) {
							controller->addEntityComponent(info.entity, Entity::Name{ n->get<std::string>() });
						}
						else {
							controller->addEntityComponent(info.entity,
								Entity::Name{ "Entity " + std::to_string(static_cast<uint32_t>(info.entity)) });
						}
					}
				}
			}
			PN_CORE_INFO("[SceneManager] PASS 2 Complete: Loaded components for {} entities", entityInfos.size());
			PN_CORE_INFO("[SceneManager] Successfully built {} entities from scene asset", entityInfos.size());
			return true;
		}

		void SceneManager::setupCamera(SceneAsset const& scene_asset) {

			const auto& camSettings = scene_asset.camera;

			// Set active game cam
			active_game_cam = camSettings.active_game_cam;

			// Create or update camera
			editor_camera = std::make_unique<Camera>(
				camSettings.position,
				camSettings.forward,
				camSettings.up,
				camSettings.fov,
				camSettings.nearPlane,
				camSettings.farPlane,
				camSettings.aspectRatioW,
				camSettings.aspectRatioH
			);
			SetEditorCamera();

			PN_CORE_INFO("[SceneManager] Camera setup: pos({}, {}, {}), fov={}",
				camSettings.position.x, camSettings.position.y, camSettings.position.z,
				camSettings.fov);
		}

		void SceneManager::setupEnvironment(SceneAsset const& scene_asset) {

			//Get environment variables
			const auto& env = scene_asset.environment;

			//Setup gs
			gs.world_light = env.useWorldLight;
			gs.ibl = env.useIBL;
			gs.DEBUG_USE_DIFFUSE_MAP = env.useDiffuseMap;
			gs.DEBUG_USE_AO_MAP = env.useAOMap;
			gs.DEBUG_USE_NORMAL_MAP = env.useNormalMap;
			gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP = env.useRoughnessMetallicMap;
			gs.DEBUG_USE_EMISSION_MAP = env.useEmissionMap;
			gs.DEBUG_PBR_MAP_TYPE = env.pbr_map;

			//Create default light sources
			LightSources::get().create(camera_light_name);
			PN_CORE_INFO("[SceneManager] Created cam and world light source.");

			//Set up camera light intensity
			if (!getCameraLight()) {
				LightSources::get().create(camera_light_name);
			}
			getCameraLight()->L_intensity = env.cameraLightIntensity;

			//Set up world light
			if (gs.world_light) {
				if (!getWorldLight()) {
					LightSources::get().create(world_light_name);
				}
				getWorldLight()->L_intensity = env.worldLightIntensity;
				getWorldLight()->forward = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
				getWorldLight()->setShadowType(Light::SHADOW_TYPES::MAPPED);
				getWorldLight()->type = Light::TYPES::DIRECTIONAL;
			}

			//Load Skybox GUID
			if (env.skyboxGUID.IsValid()) {
				curr_skybox_id = env.skyboxGUID;
				Skybox::get().setTexture(curr_skybox_id);
				PN_CORE_INFO("[SceneManager] Set skybox with texture of GUID: {}", curr_skybox_id.ToString());
			}
			else {

#ifdef PN_PLATFORM_WINDOWS
				std::filesystem::path sb_path = "engine/textures/skybox2.hdr";
#else
				std::filesystem::path sb_path = "engine\\\\textures\\\\skybox2.hdr";
#endif
				//Set skybox id
				curr_skybox_id = services->get<Assets::Manager>()->findGUID(sb_path);

				//Default skybox texture
				Skybox::get().setTexture(sb_path);
				PN_CORE_INFO("[SceneManager] Set default skybox");
			}

			PN_CORE_INFO("[SceneManager] Environment setup complete");
		}

		void SceneManager::setupLayers(SceneAsset const& scene_asset) {
			layers = scene_asset.layers;
			if (layers.empty()) layers.push_back(Layer{ 0, 1, true });
			mask_matrix = scene_asset.mask_matrix;

			//Get all entities with layers
			auto ecs_controller = services->get<ECS::Controller>();

			//Get view of components
			auto view = ecs_controller->getRegistry().view<Entity::Layer, Entity::GUID>();

			// When creating entities, check if layer component exists and validate
			for (auto [e, layer, guid] : view.each()) {

				// Validate layer ID is within scene's layer count
				if (layer.layer_id >= layers.size()) {
					PN_CORE_WARN("Entity {} has invalid layerID {}, resetting to 0", guid.guid.ToString(), layer.layer_id);
					layer.layer_id = 0;
					layer.layer_mask = 1;
				}

				// Update mask based on ID
				layer.layer_mask = 1 << layer.layer_id;
				layer.layerName = layers[layer.layer_id].name;
			}
		}

		nlohmann::json SceneManager::captureCurrentEntities() {
			nlohmann::json ecs = nlohmann::json::object();
			nlohmann::json ents = nlohmann::json::array();

			auto controller = services->get<ECS::Controller>();
			auto prefabService = services->get<Prefab::Service>();

			if (!controller) {
				PN_CORE_ERROR("[SceneManager] ECS Controller not available");
				return ecs;
			}

			auto& registry = controller->getRegistry();
			auto view = registry.view<Entity::Name>();

			for (auto e : view) {
				nlohmann::json E = nlohmann::json::object();

				// Get entity name
				if (auto nameOpt = controller->getEntityComponent<Entity::Name>(e)) {
					E["Name"] = nameOpt->get().name;
				}

				// Detect and save overrides for prefab instances
				if (prefabService && registry.any_of<Prefab::PrefabInstance>(e)) {
					prefabService->updateAllOverrides(e, ECS::MAIN_REGISTRY_ID);
				}

				// Serialize all components
				E["Components"] = controller->getAllComponentsAsJson(e);

				// Wrap in Entity object
				ents.push_back(nlohmann::json{ {"Entity", std::move(E)} });
			}

			ecs["Entities"] = std::move(ents);

			PN_CORE_INFO("[SceneManager] Captured {} entities", ents.size());

			return ecs;
		}

		void SceneManager::captureSceneVariables(SceneAsset& scene_asset) {

			//Capture all camera variables
			if (active_camera) {
				// Game camera separate from editor cam
				scene_asset.camera.active_game_cam = active_game_cam;

				// Editor cam variables
				scene_asset.camera.position = active_camera->pos;
				scene_asset.camera.forward = active_camera->forward;
				scene_asset.camera.up = active_camera->up;
				scene_asset.camera.nearPlane = active_camera->near_plane;
				scene_asset.camera.farPlane = active_camera->far_plane;
				scene_asset.camera.fov = active_camera->fov;
				scene_asset.camera.aspectRatioW = active_camera->width_ratio;
				scene_asset.camera.aspectRatioH = active_camera->height_ratio;
			}

			//Capture all graphics and env variables
			scene_asset.environment.cameraLightIntensity = getCameraLight() ? getCameraLight()->L_intensity : scene_asset.environment.cameraLightIntensity;
			scene_asset.environment.worldLightIntensity = getWorldLight() ? getWorldLight()->L_intensity : scene_asset.environment.worldLightIntensity;
			scene_asset.environment.skyboxGUID = curr_skybox_id;

			//Other graphic settings
			scene_asset.environment.useWorldLight = gs.world_light;
			scene_asset.environment.useDiffuseMap = gs.DEBUG_USE_DIFFUSE_MAP;
			scene_asset.environment.useAOMap = gs.DEBUG_USE_AO_MAP;
			scene_asset.environment.useNormalMap = gs.DEBUG_USE_NORMAL_MAP;
			scene_asset.environment.useRoughnessMetallicMap = gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP;
			scene_asset.environment.useEmissionMap = gs.DEBUG_USE_EMISSION_MAP;
			scene_asset.environment.pbr_map = gs.DEBUG_PBR_MAP_TYPE;

			//Capture all layer variables
			scene_asset.layers = layers;
			scene_asset.mask_matrix = mask_matrix;
		}

		nlohmann::json SceneManager::convertSceneToJSON(SceneAsset& scn_asset) {
			//Convert scene asset to JSON
			nlohmann::json sceneJson = nlohmann::json::object();

			//Camera settings
			sceneJson["camera"] = {
				{"active_game_cam", scn_asset.camera.active_game_cam},
				{"position", {scn_asset.camera.position.x, scn_asset.camera.position.y, scn_asset.camera.position.z}},
				{"forward", {scn_asset.camera.forward.x, scn_asset.camera.forward.y, scn_asset.camera.forward.z}},
				{"up", {scn_asset.camera.up.x, scn_asset.camera.up.y, scn_asset.camera.up.z}},
				{"fov", scn_asset.camera.fov},
				{"nearPlane", scn_asset.camera.nearPlane},
				{"farPlane", scn_asset.camera.farPlane},
				{"aspectRatioW", scn_asset.camera.aspectRatioW},
				{"aspectRatioH", scn_asset.camera.aspectRatioH}
			};

			//Environment settings
			sceneJson["environment"] = {
				{"skyboxGUID", scn_asset.environment.skyboxGUID.ToString()},
				{"cameraLightIntensity", {scn_asset.environment.cameraLightIntensity.x, scn_asset.environment.cameraLightIntensity.y, scn_asset.environment.cameraLightIntensity.z}},
				{"worldLightIntensity", {scn_asset.environment.worldLightIntensity.x, scn_asset.environment.worldLightIntensity.y, scn_asset.environment.worldLightIntensity.z}},
				{"useWorldLight", scn_asset.environment.useWorldLight},
				{"useIBL", scn_asset.environment.useIBL},
				{"useDiffuseMap", scn_asset.environment.useDiffuseMap},
				{"useAOMap", scn_asset.environment.useAOMap},
				{"useNormalMap", scn_asset.environment.useNormalMap},
				{"useRoughnessMetallicMap", scn_asset.environment.useRoughnessMetallicMap},
				{"useEmissionMap", scn_asset.environment.useEmissionMap},
				{"pbr_map", static_cast<int>(scn_asset.environment.pbr_map)}
			};

			// Layers
			nlohmann::json layersJson = nlohmann::json::array();
			for (const auto& layer : scn_asset.layers) {
				layersJson.push_back({
					{"id", layer.id},
					{"mask", layer.mask},
					{"enabled", layer.enabled},
					{"name", layer.name},
					{"color",  {layer.color.r, layer.color.g, layer.color.b}}
					});
			}
			sceneJson["layers"] = layersJson;
			sceneJson["mask_matrix"] = scn_asset.mask_matrix;

			// Entity data
			sceneJson["ecs"] = scn_asset.entityData;

			return sceneJson;
		}

#ifdef PN_PLATFORM_WINDOWS
		bool SceneManager::saveSceneToPath(SceneAsset& scn_asset, std::filesystem::path const& relative) {

			//Convert scene asset to JSON
			nlohmann::json sceneJson = convertSceneToJSON(scn_asset);

			//Save scene
			auto path_service = services->get<Path::Path>();
			if (!path_service) {
				PN_CORE_ERROR("[SceneManager] Path Service not available");
				return false;
			}

			//Craft virtual path
			std::string virtual_path = path_service->aliasCombineRelative(Path::main_assets_alias, relative.string());

			//Save json
			auto stream = path_service->createFileStream(virtual_path, Path::FileMode::Write);

			//Write to stream
			stream->write(sceneJson);

			//Log scene saved
			PN_CORE_INFO("[SceneManager] Scene saved successfully to: {}", virtual_path);
			return true;
		}
#endif

		void SceneManager::configScene(SceneAsset const& scn_asset) {
			//Build new scene
			setupCamera(scn_asset);
			setupEnvironment(scn_asset);
			setupLayers(scn_asset);

			//Failed to build entities
			if (!buildEntitiesFromAsset(scn_asset)) {
				PN_CORE_ERROR("[SceneManager] Failed to build entities from scene asset");
				return;
			}

			// set scene in renderer
			auto renderer = services->get<sRenderer>();
			renderer->setScene(services->get<Scene::SceneManager>());


			// init vbo for scene
			{
				std::vector<ModelRenderer> models{};

				auto ecs = services->get<ECS::Controller>();
				auto& registry = ecs->getRegistry();
				auto view = registry.view<ModelRenderer>();
				for (auto e : view) {
					auto mdl = ecs->getEntityComponent<ModelRenderer>(e);
					if (mdl.has_value()) models.push_back(mdl.value());
				}

				renderer->initSceneVbo(models);
			}
		}

		void SceneManager::onAttach() {

			//Get ECS Controller
			auto ecs = services->get<ECS::Controller>();

			//Init skybox here, set texture for skybox in config scene
			Skybox::get().init(services);
			PN_CORE_INFO("[SceneManager] Initialized skybox");

			// Demo Object and Audio Setup

			auto pathService = services->get<Path::Path>();
			auto asset_manager = services->get<Assets::Manager>();

			{
				// for .mesh(converted from .obj only)
				std::optional<std::shared_ptr<Assets::Model>> mdl_opt;
				std::shared_ptr<Assets::Model> mdl;

#ifdef PN_PLATFORM_WINDOWS
				std::filesystem::path dm_path = "game/models/damagedhelmet/DamagedHelmet.mesh";
#else	
				std::filesystem::path dm_path = "game\\models\\damagedhelmet\\DamagedHelmet.mesh";
#endif
				//Get model
				PN_CORE_INFO("Attempting to add {} to scene", dm_path.string());
				mdl_opt = asset_manager->getAsset<Assets::Model>(dm_path);
				if (mdl_opt.has_value()) {
					mdl = mdl_opt.value();

					auto e = AddObject(mdl, "dm", { 0.f, 1.5f, 1.f }, glm::angleAxis(glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f)), { 1.f, 1.f, 1.f });
				}


#ifdef PN_PLATFORM_WINDOWS
				std::filesystem::path tc_path = "game/models/toycar/ToyCar.mesh";
#else	
				std::filesystem::path tc_path = "game\\models\\toycar\\ToyCar.mesh";
#endif
				//Get model
				PN_CORE_INFO("Attempting to add {} to scene", tc_path.string());
				mdl_opt = asset_manager->getAsset<Assets::Model>(tc_path);
				if (mdl_opt.has_value()) {
					mdl = mdl_opt.value();

					auto e = AddObject(mdl, "toycar", { 2.f, 1.5f, 1.f }, glm::angleAxis(glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3{ 0.005f });
				}


#ifdef PN_PLATFORM_WINDOWS
#ifdef PN_PLATFORM_WINDOWS
				std::filesystem::path bs_path = "game/models/brainstem/BrainStem.mesh";
#else	
				std::filesystem::path bs_path = "game\\models\\brainstem\\BrainStem.mesh";
#endif
				//Get model

				PN_CORE_INFO("Attempting to add {} to scene", bs_path.string());
				mdl_opt = asset_manager->getAsset<Assets::Model>(bs_path);
				if (mdl_opt.has_value()) {
					mdl = mdl_opt.value();
					//mdl->materials[0].metallic = 0.f;
					//mdl->materials[0].roughness = 1.f;
					//mdl->materials[0].baseColor = { 0.3f, 0.3f, 0.3f };
					auto e = AddObject(mdl, "bs", { 0.f, 0.f, -10.f }, glm::angleAxis(glm::radians(0.f), glm::vec3(0.0f, 0.0f, 0.0f)), { 5.f, 5.f, 5.f });
				}
				else {
					throw std::runtime_error("animation obj err");
				}
#endif

#ifdef PN_PLATFORM_WINDOWS
				std::filesystem::path fh_path = "game/models/Frog_Hopping.mesh";
#else	
				std::filesystem::path fh_path = "game\\models\\Frog_Hopping.mesh";
#endif
				//Get model
				PN_CORE_INFO("Attempting to add {} to scene", fh_path.string());
				mdl_opt = asset_manager->getAsset<Assets::Model>(fh_path);
				if (mdl_opt.has_value()) {
					mdl = mdl_opt.value();
					//mdl->materials[0].metallic = 0.f;
					//mdl->materials[0].roughness = 1.f;
					//mdl->materials[0].baseColor = { 0.3f, 0.3f, 0.3f };
					auto e = AddObject(mdl, "fh", { -3.f, 2.f, 0.f }, glm::angleAxis(glm::radians(0.f), glm::vec3(0.0f, 0.0f, 0.0f)), { 1.f, 1.f, 1.f });
				}
				else {
					throw std::runtime_error("animation obj err");
				}
			}

			//Create default scene asset
			SceneAsset default_scene_config;

			// Prep for subs
			//std::filesystem::path init_scn_path = "game/scenes/prototype.scn";

			//auto scn_opt = asset_manager->getAssetData(init_scn_path);

			//if (scn_opt) {

			//	loadScene(scn_opt.get()->guid);
			//}


			//Configure scene with default settings
			configScene(default_scene_config);

			//Craft skybox path and get GUID
			std::filesystem::path skybox_path = "engine/textures/skybox2.hdr";

			//Set skybox
			setCurrSkyBoxTexture(asset_manager->findGUID(skybox_path));

			//Log scene manager init
			PN_CORE_INFO("[SceneManager] Initialized");
		}

		void SceneManager::onDetach() {
			PN_CORE_INFO("[SceneManager] Shutting down");

			// Clean up current scene
			unloadScene();
		}

		void SceneManager::onUpdate(AppTiming timing) {

			//		// Get time scale from ViewportPanel (0.0 when paused, 1.0 when playing)
			//		float timeScale = 1.0f;
			//		bool isPaused = false;

			//#ifdef _DEBUG
			//		if (auto viewport = services->get<Editor::Panel::ViewportPanel>()) {
			//			timeScale = viewport->getTimeScale();
			//			isPaused = (timeScale == 0.0f);
			//		}
			//#endif

					// Daytime / Nighttime setting
			{
				if (gs.world_light) {

					auto olc = getWorldLight();

					if (!olc) {
						LightSources::get().create("world");
						getWorldLight()->L_intensity = glm::vec3(GraphicsSettings::get().global_light_intensity);
						getWorldLight()->forward = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
						getWorldLight()->setShadowType(Light::SHADOW_TYPES::MAPPED);
						getWorldLight()->type = Light::TYPES::DIRECTIONAL;
					}
					else {
						olc->position = GetActiveCamera()->pos - glm::normalize(olc->forward) * olc->shadow_source_follow_distance;
					}
				}
				else {
					auto olc = LightSources::get().get("world");

					if (olc) {
						LightSources::get().destroy("world");
					}
				}
			}

			auto ecs = services->get<ECS::Controller>();
			auto& registry = ecs->getRegistry();
			auto view = registry.view<Entity::Name>();

			std::unordered_set<std::string> cameras_seen_this_frame;


			for (auto e : view) {


				auto cam = ecs->getEntityComponent<Cam>(e);

				if (!cam.has_value()) {
					continue;
				}
				auto trans = ecs->getEntityComponent<LocalTransform>(e);

				glm::vec3 entity_pos = { 0.f,0.f,0.f };
				glm::quat entity_rot = glm::quat({ 0.f,0.f,0.f });
				glm::vec3 entity_scale = { 0.f,0.f,0.f };

				if (trans.has_value()) {
					entity_pos = trans->get().position;
					entity_rot = trans->get().rotation;
					entity_scale = trans->get().scale;
				}

				glm::vec3 offset_world = entity_rot * (cam->get().trans_offset * entity_scale);
				glm::vec3 cam_pos = entity_pos + offset_world;

				glm::vec3 look_offset = cam->get().rot_offset * entity_scale; //offset from entity
				glm::vec3 target_pos = entity_pos + look_offset;

				glm::vec3 forward{ glm::normalize(target_pos - cam_pos) };
				glm::vec3 up{ 0.f, 1.f, 0.f };
				float near_plane = cam->get().near_plane;
				float far_plane = cam->get().far_plane;
				float width_ratio = cam->get().width_ratio;
				float height_ratio = cam->get().height_ratio;


				auto metadata = services->get<MetaData::Service>();

				std::string entity_name = "UNNAMED CAMERA";
				if (metadata) {
					entity_name = metadata->getEntityName(e);
				}

				cameras_seen_this_frame.insert(entity_name);


				auto it = game_cameras.find(entity_name); // Single lookup

				// If camera exists update every frame else create new
				if (it != game_cameras.end()) {
					auto curr_cam = it->second.get();
					curr_cam->pos = cam_pos;
					curr_cam->forward = forward;
					curr_cam->up = up;
					curr_cam->fov = GraphicsSettings::get().fov;
					curr_cam->near_plane = near_plane;
					curr_cam->far_plane = far_plane;
					curr_cam->width_ratio = width_ratio;
					curr_cam->height_ratio = height_ratio;

				}
				else {
					game_cameras.insert(std::pair<std::string, std::unique_ptr<Camera>>(entity_name, std::make_unique<Camera>(cam_pos, forward, up, GraphicsSettings::get().fov, near_plane, far_plane, width_ratio, height_ratio)));

				}
			}

			// Clear unwanted cameras
			for (auto it = game_cameras.begin(); it != game_cameras.end(); ) {
				const std::string& map_name = it->first;
				if (cameras_seen_this_frame.find(map_name) == cameras_seen_this_frame.end()) {

					// SAFETY: Check if we are deleting the currently active camera
					if (active_camera == it->second.get()) {
						// Fallback to editor camera or nullptr to prevent crashing
						SetEditorCamera();
						active_game_cam = "";
						PN_CORE_WARN("Active Camera '{}' was deleted. Switched to Editor Camera.", map_name);
					}

					// Erase returns the iterator to the NEXT element
					it = game_cameras.erase(it);
				}
				else {
					// Move to next
					++it;
				}

			}
		}

		void SceneManager::loadScene(const Assets::GUID& sceneGUID) {
			auto assetManager = services->get<Assets::Manager>();
			if (!assetManager) {
				PN_CORE_ERROR("[SceneManager] Asset Manager not available");
				return;
			}

			// Get scene asset by GUID
			auto sceneOpt = assetManager->getAsset<SceneAsset>(sceneGUID);
			if (!sceneOpt.has_value()) {
				PN_CORE_ERROR("[SceneManager] Failed to load scene with GUID: {}", sceneGUID.ToString());
				return;
			}

			//Get scene asset
			auto currentSceneAsset = sceneOpt.value();

			//Clear old scene
			unloadScene();

			//Configure the new scene
			if (currentSceneAsset) {
				configScene(*currentSceneAsset);
			}
			else {
				PN_CORE_INFO("[SceneManager] Failed to configure scene with GUID: {}", sceneGUID.ToString());
				return;
			}

			// init vbo for scene
			//{
			//	std::vector<ModelRenderer> models{};

			//	auto ecs = services->get<ECS::Controller>();
			//	auto& registry = ecs->getRegistry();
			//	auto view = registry.view<ModelRenderer>();
			//	for (auto e : view) {
			//		auto mdl = ecs->getEntityComponent<ModelRenderer>(e);
			//		if (mdl.has_value()) models.push_back(mdl.value());
			//	}

			//	auto renderer = services->get<sRenderer>();
			//	renderer->initSceneVbo(models);
			//}

			//Scene loaded successfully
			PN_CORE_INFO("[SceneManager] Loaded scene from GUID: {}", sceneGUID.ToString());
			curr_scene_id = sceneGUID;
		}

		void SceneManager::loadScene(std::filesystem::path const& relative_path) {

			//path service and asset service
			auto assets_service = services->get<Assets::Manager>();
			if (!assets_service) {
				PN_CORE_ERROR("[SceneManager] Asset | Path Manager not available");
				return;
			}

			//Get GUID to load scene
			auto id = assets_service->findGUID(relative_path);

			//Load scene
			loadScene(id);
		}

#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG
		void SceneManager::createScene(std::string const& name) {

			//Check if valid name was provided
			std::filesystem::path relative;
			if (!name.empty()) {
				//Craft relative
				auto ext = *(Assets::getAllExtensions()[Assets::Type::Scenes].begin());
				auto game_scn_folder = Assets::getAllGameFolders()[Assets::Type::Scenes];
				relative = game_scn_folder / (name + ext);
			}
			else {
				PN_CORE_WARN("Unable to create scene, no name provided.");
				return;
			}

			//Create a default scene
			SceneAsset default_scn;
			saveSceneToPath(default_scn, relative);

			//Unload scene
			unloadScene();
		}


		void SceneManager::deleteScene(Assets::GUID const& id) {

			//Get asset manager
			auto assetManager = services->get<Assets::Manager>();
			if (!assetManager) {
				PN_CORE_ERROR("[SceneManager] Asset Manager not available");
				return;
			}

			//Delete file
			assetManager->removeFile(id);
		}
#endif

		void SceneManager::saveActiveScene(Assets::GUID const& scn_id, std::string const& name) {

			//Check
			auto assetManager = services->get<Assets::Manager>();
			if (!assetManager) {
				PN_CORE_ERROR("[SceneManager] Asset Manager not available");
				return;
			}

			//Create a scene class to be saved
			Scene::SceneAsset* currentSceneAsset;

			//Get scene asset by GUID
			auto sceneOpt = assetManager->getAsset<SceneAsset>(scn_id);
			if (!sceneOpt.has_value()) {

				//Check if name is provided for def asset to be saved
				if (name.empty()) {
					PN_CORE_WARN("Invalid use of the save active scene. No name and invalid GUID provided.");
					return;
				}

				//Scn id is invalid, creating new asset class to be saved
				currentSceneAsset = new SceneAsset();
			}
			else {
				//Scn successfully loaded
				currentSceneAsset = sceneOpt.value().get();
			}

			//Capture current ECS state
			currentSceneAsset->entityData = captureCurrentEntities();

			//Capture the scene variables
			captureSceneVariables(*currentSceneAsset);

			//Check if valid name was provided
			std::filesystem::path relative;
			if (scn_id.IsValid() && name.empty()) {
				relative = currentSceneAsset->main_relative_path;
			}
			else {
				//Craft relative
				auto ext = *(Assets::getAllExtensions()[Assets::Type::Scenes].begin());
				auto game_scn_folder = Assets::getAllGameFolders()[Assets::Type::Scenes];
				relative = game_scn_folder / (name + ext);
			}

			//Save scene to path
			saveSceneToPath(*currentSceneAsset, relative);

			//Clear up memory for allocated mem
			if (!sceneOpt.has_value()) delete currentSceneAsset;
		}
#endif

		void SceneManager::unloadScene() {
			PN_CORE_INFO("[SceneManager] Unloading current scene");

			// Destroy all ECS entities
			auto controller = services->get<ECS::Controller>();
			if (controller) {
				controller->destroyAllEntities();
				PN_CORE_INFO("[SceneManager] Cleared all entities");
			}

			//Reset with default
			curr_scene_id = Assets::GUID();

			// Clear lights
			//LightSources::get().clear();
			//PN_CORE_INFO("[SceneManager] Cleared all lights");

			// Reset camera (but don't destroy it - we'll reuse it)
			active_game_cam = "";

			// Clear scene asset reference (but keep the object if we're reloading)
			// currentSceneAsset.reset();
		}

		void SceneManager::setCurrSkyBoxTexture(Assets::GUID const& skybox_id) {
			curr_skybox_id = skybox_id;
			Skybox::get().setTexture(curr_skybox_id);
		}

		Camera* SceneManager::GetActiveCamera()
		{
			return active_camera;
		}

		Camera* SceneManager::GetGameCamera()
		{
			auto it = game_cameras.find(active_game_cam);
			if (it != game_cameras.end()) {
				return it->second.get();
			}
			return nullptr;
		}

		void SceneManager::SetActiveCamera(Camera* cam) {
			active_camera = cam;
		}

		void SceneManager::SetEditorCamera() {
			SetActiveCamera(editor_camera.get());
		}

		void SceneManager::SetGameCamera()
		{
			auto it = game_cameras.find(active_game_cam);
			if (it != game_cameras.end()) {
				SetActiveCamera(it->second.get());
			}

		}

		void SceneManager::ChangeGameCamera(std::string cam_name)
		{
			PN_CORE_INFO("SELECTING Camera : {}", cam_name);
			active_game_cam = cam_name;
		}

		const std::string& SceneManager::GetActiveGameCamera()
		{
			return active_game_cam;
		}

		const std::unordered_map<std::string, std::unique_ptr<Camera>>& SceneManager::GetAllGameCamera() const {
			return game_cameras;
		}
	}

	//	void Scene::onDetach() {}
	//
	//	void Scene::onAttach()
	//	{
	//
	//		auto ecs = services->get<ECS::Controller>();
	//
	//		// Camera and Scene Setup
	//		glm::vec3 pos{ 0.f, 2.f, 4.f };
	//		//glm::vec3 forward{-glm::normalize(pos)};
	//		glm::vec3 forward{ 0.f, 0.f, -1.f };
	//		glm::vec3 up{ 0.f, 1.f, 0.f };
	//		float near_plane{ 0.1f };
	//		float far_plane{ 100.f };
	//		float width_ratio{ 16.f };
	//		float height_ratio{ 9.f };
	//		camera = std::make_unique<Camera>(pos, forward, up, GraphicsSettings::get().fov, near_plane, far_plane, width_ratio, height_ratio);
	//
	//		// Init light sources (REQUIRED TO FUNCTION)
	//		LightSources::get().create("cam");
	//		auto olcam = LightSources::get().get("cam");
	//		Light& lcam = olcam.value();
	//		lcam.L_intensity = glm::vec3(0.01f);
	//
	//		if (GraphicsSettings::get().daytime) {
	//			LightSources::get().create("world");
	//			auto olc = LightSources::get().get("world");
	//			Light& lc = olc.value();
	//			lc.forward = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
	//			//lc.position = -lc.forward * 10.f;					// follows camera
	//			lc.L_intensity = glm::vec3(GraphicsSettings::get().global_light_intensity);
	//			lc.setShadowType(Light::SHADOW_TYPES::MAPPED);
	//			lc.type = Light::TYPES::DIRECTIONAL;
	//		}
	//		//lc.far_plane = 200.f;
	//		//lc.forward = -lc.position;
	//
	//		// Demo Object and Audio Setup
	//		auto audioManager = services->get<Audio::Audio>();
	//		auto pathService = services->get<Path::Path>();
	//		auto asset_manager = services->get<Assets::Manager>();
	//
	//		auto ogre_diff = Assets::GUID("5923aab8-5293-f945-958e-496acd0218c3");
	//		auto ogre_smile_ao = Assets::GUID("cee03212-928a-6347-9d55-07fe46ac3ea1");
	//
	//		// for .mesh(converted from .obj only)
	//		std::optional<std::shared_ptr<Assets::Model>> mdl_opt;
	//		std::shared_ptr<Assets::Model> mdl;
	//		{
	//#ifdef PN_PLATFORM_WINDOWS
	//			std::filesystem::path ogre_smile_path = "game/models/ogre_smile.mesh";
	//#else	
	//			std::filesystem::path ogre_smile_path = "game\\models\\ogre_smile.mesh";
	//#endif
	//
	//			//Get model
	//			mdl_opt = asset_manager->getAsset<Assets::Model>(ogre_smile_path);
	//			if (mdl_opt.has_value()) {
	//				mdl = mdl_opt.value();
	//
	//				// logging to check data
	//				{
	//					PN_CORE_TRACE("File: {}\nVertices: {}\nIndices: {}\nMaterials: {}", mdl->vpath, mdl->vertices.size(), mdl->indices.size(), mdl->materials.size());
	//					//PN_CORE_TRACE("Base roughness: {}\nBase metallic: {}\nBase color: {},{},{}", mdl->materials[0].roughness, mdl->materials[0].metallic, mdl->materials[0].baseColor.r, mdl->materials[0].baseColor.g, mdl->materials[0].baseColor.b);
	//				}
	//
	//				AddObject(mdl, "ogre_smile", { 0.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f }, ogre_diff, ogre_smile_ao);
	//			}
	//		}
	//
	//#ifdef PN_PLATFORM_WINDOWS
	//		std::filesystem::path ogre_path = "game/models/ogre.mesh";
	//#else	
	//		std::filesystem::path ogre_path = "game\\models\\ogre.mesh";
	//#endif
	//		//Get model
	//		mdl_opt = asset_manager->getAsset<Assets::Model>(ogre_path);
	//		if (mdl_opt.has_value()) {
	//			mdl = mdl_opt.value();
	//
	//			AddObject(mdl, "ogre_left", { -2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
	//
	//			AddObject(mdl, "ogre_left", { -2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f }, ogre_diff, ogre_smile_ao);
	//			AddObject(mdl, "ogre_right", { 2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f }, ogre_diff, ogre_smile_ao);
	//		}
	//
	//#ifdef PN_PLATFORM_WINDOWS
	//		std::filesystem::path sdcc_path = "game/models/sdcc.mesh";
	//#else	
	//		std::filesystem::path sdcc_path = "game\\models\\sdcc.mesh";
	//#endif
	//
	//		auto sdcc_diff = Assets::GUID("71051859-f5ee-144a-b1e5-59ad02d13695");
	//		//Get model
	//		mdl_opt = asset_manager->getAsset<Assets::Model>(sdcc_path);
	//		if (mdl_opt.has_value()) {
	//			mdl = mdl_opt.value();
	//
	//			AddObject(mdl, "sdcc", { 5.f, 0.f, -3.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f)), { 3.f, 3.f, 3.f });
	//		}
	//
	//#ifdef PN_PLATFORM_WINDOWS
	//		std::filesystem::path city_path = "game/models/city.mesh";
	//#else	
	//		std::filesystem::path city_path = "game\\models\\city.mesh";
	//#endif
	//
	//		auto city_diff = Assets::GUID{ "29fe999b-d257-bf41-879d-6d7578d43734" };
	//		//Get model
	//		mdl_opt = asset_manager->getAsset<Assets::Model>(city_path);
	//		if (mdl_opt.has_value()) {
	//			mdl = mdl_opt.value();
	//
	//			AddObject(mdl, "city", { -8.f, 0.f, -5.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f)), { 3.f, 3.f, 3.f });
	//		}
	//
	//#ifdef PN_PLATFORM_WINDOWS
	//		std::filesystem::path dm_path = "game/models/damagedhelmet/DamagedHelmet.mesh";
	//#else	
	//		std::filesystem::path dm_path = "game\\models\\damagedhelmet\\DamagedHelmet.mesh";
	//#endif
	//		//Get model
	//		mdl_opt = asset_manager->getAsset<Assets::Model>(dm_path);
	//		if (mdl_opt.has_value()) {
	//			mdl = mdl_opt.value();
	//
	//			auto e = AddObject(mdl, "dm", { 0.f, 1.5f, 1.f }, glm::angleAxis(glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f)), { 1.f, 1.f, 1.f });
	//		}
	//
	//
	//#ifdef PN_PLATFORM_WINDOWS
	//		std::filesystem::path bs_path = "game/models/brainstem/BrainStem.mesh";
	//#else	
	//		std::filesystem::path bs_path = "game\\models\\brainstem\\BrainStem.mesh";
	//#endif
	//		//Get model
	//		mdl_opt = asset_manager->getAsset<Assets::Model>(bs_path);
	//		if (mdl_opt.has_value()) {
	//			mdl = mdl_opt.value();
	//			//mdl->materials[0].metallic = 0.f;
	//			//mdl->materials[0].roughness = 1.f;
	//			//mdl->materials[0].baseColor = { 0.3f, 0.3f, 0.3f };
	//			auto e = AddObject(mdl, "bs", { 0.f, 0.f, -10.f }, glm::angleAxis(glm::radians(0.f), glm::vec3(0.0f, 0.0f, 0.0f)), { 5.f, 5.f, 5.f });
	//		}
	//		else {
	//			throw std::runtime_error("animation obj err");
	//		}
	//
	//#ifdef PN_PLATFORM_WINDOWS
	//		std::filesystem::path fh_path = "game/models/Frog_Hopping.mesh";
	//#else	
	//		std::filesystem::path fh_path = "game\\models\\Frog_Hopping.mesh";
	//#endif
	//		//Get model
	//		mdl_opt = asset_manager->getAsset<Assets::Model>(fh_path);
	//		if (mdl_opt.has_value()) {
	//			mdl = mdl_opt.value();
	//			//mdl->materials[0].metallic = 0.f;
	//			//mdl->materials[0].roughness = 1.f;
	//			//mdl->materials[0].baseColor = { 0.3f, 0.3f, 0.3f };
	//			auto e = AddObject(mdl, "fh", { -3.f, 2.f, 0.f }, glm::angleAxis(glm::radians(0.f), glm::vec3(0.0f, 0.0f, 0.0f)), { 1.f, 1.f, 1.f });
	//		}
	//		else {
	//			throw std::runtime_error("animation obj err");
	//		}
	//
	//
	//
	//		// gltf testing
	////#define GLTF_TEST
	//#ifdef GLTF_TEST
	//		{
	//			mdl = cacheModel("game_assets://models/930/930.mesh");
	//			AddObject(mdl, "930", { 0.f, 1.f, -5.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
	//		}
	//#endif
	//
	//
	//		// font
	//		TextRenderer::get();
	//
	//#ifdef PN_PLATFORM_WINDOWS
	//		std::filesystem::path sb_path = "engine/textures/skybox2.hdr";
	//#else
	//		std::filesystem::path sb_path = "engine\\textures\\skybox2.hdr";
	//#endif
	//
	//		// skybox
	//		Skybox::get().init(services, sb_path);
	//
	//		// Test load prefab
	//		//std::vector<entt::entity> loaded_entities = services->get<Serialization::Service>()->loadPrefabFromFile("ogre_right.prefab");
	//		//for (auto e : loaded_entities) {
	//		//	// Info: Print entity names, transforms, etc.
	//		//	auto nameOpt = services->get<ECS::Controller>()->getEntityComponent<Entity::Name>(e);
	//		//	std::string name = nameOpt ? nameOpt->get().name : "<no name>";
	//		//}
	//
	//	}
	//
	//	void Scene::onUpdate(AppTiming timing)
	//	{
	//		// Get time scale from ViewportPanel (0.0 when paused, 1.0 when playing)
	//		float timeScale = 1.0f;
	//		bool isPaused = false;
	//
	//#ifdef _DEBUG
	//		if (auto viewport = services->get<Editor::Panel::ViewportPanel>()) {
	//			timeScale = viewport->getTimeScale();
	//			isPaused = (timeScale == 0.0f);
	//		}
	//#endif
	//
	//		// Daytime / Nighttime setting
	//		{
	//			if (GraphicsSettings::get().daytime) {
	//
	//				auto olc = LightSources::get().get("world");
	//
	//				if (!olc) {
	//					LightSources::get().create("world");
	//					auto olc = LightSources::get().get("world");
	//					Light& lc = olc.value();
	//					lc.forward = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
	//					//lc.position = -lc.forward * 10.f;					// follows camera
	//					lc.L_intensity = glm::vec3(GraphicsSettings::get().global_light_intensity);
	//					lc.setShadowType(Light::SHADOW_TYPES::MAPPED);
	//					lc.type = Light::TYPES::DIRECTIONAL;
	//					GraphicsSettings::get().ibl = true;
	//				}
	//			}
	//			else {
	//				auto olc = LightSources::get().get("world");
	//
	//				if (olc) {
	//					LightSources::get().destroy("world");
	//					GraphicsSettings::get().ibl = false;
	//				}
	//			}
	//		}
	//
	//		if (GraphicsSettings::get().daytime) {
	//			auto olc = LightSources::get().get("world");
	//			Light& lc = olc.value();
	//			lc.position = GetActiveCamera()->pos - glm::normalize(lc.forward) * lc.shadow_source_follow_distance;
	//		}
	//
	//		// animation
	//		auto ecs = services->get<ECS::Controller>();
	//		auto& registry = ecs->getRegistry();
	//		auto view = registry.view<ModelRenderer>();
	//		for (auto e : view) {
	//			auto mdl = ecs->getEntityComponent<ModelRenderer>(e);
	//			if(mdl.has_value()) mdl->get().UpdateAnimation(timing.dt);
	//		}
	//	}
	//
	//	void Scene::onEvent(Event::Event& e) {}
	//
	//	entt::entity Scene::AddObject(const std::shared_ptr<Assets::Model>& mdl,  const std::string& name, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale, Assets::GUID const& diff_id, Assets::GUID const& ao_id)
	//	{
	//		auto ecs = services->get<ECS::Controller>();
	//		auto meta = services->get<MetaData::Service>();
	//
	//		// if animated object, may be in T pose. must find root xform
	//		glm::vec3 root_scale = glm::vec3(1.f);
	//		glm::quat root_rot= glm::quat(1.f, 0.f, 0.f, 0.f);
	//		glm::vec3 root_trans = glm::vec3(0.f);
	//
	//		PN_CORE_INFO("Model {} has {} animations", mdl->vpath, mdl->animations.size());
	//		if (mdl->animations.size()) {
	//
	//			auto anim = mdl->animations[0];
	//			for (const auto& [bone_name, track] : anim.track_map) {
	//				const auto it = std::find_if(mdl->skeleton.begin(), mdl->skeleton.end(), [&bone_name](const Assets::Bone& b) { return bone_name == b.name; });
	//				if (it == mdl->skeleton.end()) {
	//					root_scale = track[0].scale;
	//					root_rot = track[0].rotation;
	//					root_trans = track[0].translation;
	//					break;
	//				}
	//			}
	//		}
	//
	//		// i suppose i shouldnt bake the root xform into LocalTransform here. so
	//		//glm::mat4 root_xform = 
	//		//	glm::translate(glm::mat4(1.f), root_trans) * 
	//		//	glm::mat4_cast(root_rot) *
	//		//	glm::scale(glm::mat4(1.f), root_scale);
	//
	//
	//
	//		entt::entity entity = ecs->createEntity();
	//		ecs->addEntityComponent(entity, Entity::Name{ name });
	//		ecs->addEntityComponent(entity, LocalTransform{ pos, rot, scale });
	//		ecs->addEntityComponent(entity, WorldTransform{});
	//		ecs->addEntityComponent(entity, Entity::Hierarchy{});
	//		// ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
	//		
	//		ModelRenderer mr = ModelRenderer{ mdl->guid };
	//		if (mdl->animations.size()) {
	//			//mr.isPlaying = true;
	//			//mr.currentAnimationIndex = 0;
	//
	//			mr.PlayAnimation(0);
	//		}
	//
	//		ecs->addEntityComponent(entity, static_cast<ModelRenderer>(mr));
	//
	//		if (meta) meta->setEntityName(entity, name);
	//
	//		return entity;
	//	}
	//
	//	Camera* Scene::GetActiveCamera()
	//	{
	//		return camera.get();
	//	}
}