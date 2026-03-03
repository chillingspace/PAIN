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
#include "Systems/Scripting/GameScriptingSystem.h"

#include "LayeredSystems/LevelEditor/Panels/ResourcePanel.h"
#include "LoadingScreen.h"

#include "Systems/Physics/sysPhysics.h"

#include <thread>
#include <atomic>

#ifdef PN_PLATFORM_ANDROID
#include <CoreSystems/Events/Android/FocusEvents.h>
#endif
namespace PAIN {
	namespace Scene {

		/* =========================================================================== */
		/*                                CAMERAS                                      */
		/* =========================================================================== */

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

		Camera* SceneManager::GetEditorCamera()
		{
			return editor_camera.get();
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

		/* =========================================================================== */
		/*                                SCENES                                       */
		/* =========================================================================== */

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
					//PN_CORE_TRACE("[SceneManager] Loading prefab instance entity {}", static_cast<uint32_t>(info.entity));

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
						//PN_CORE_TRACE("[SceneManager] Loaded {} components from prefab", (*prefabEntityData)["components"].size());
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
						//PN_CORE_TRACE("[SceneManager] Applied {} overrides", info.componentOverrides.size());
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
				camSettings.aspectRatioH,
				camSettings.speed,
				camSettings.sensitivity
			);
			
			// Setup camera collision settings
			editor_camera->collisionEnabled = false;
			editor_camera->collisionRadius = camSettings.collisionRadius;
			editor_camera->collisionOffset = camSettings.collisionOffset;
			editor_camera->capsuleHeight = camSettings.capsuleHeight;
			editor_camera->useCapsuleCollision = camSettings.useCapsuleCollision;
			editor_camera->showCollisionGizmo = camSettings.showCollisionGizmo;
			
			SetEditorCamera();

			PN_CORE_ERROR("[SceneManager] Camera setup: pos({}, {}, {}), fov={} , speed={}, sens={}",
				camSettings.position.x, camSettings.position.y, camSettings.position.z,
				camSettings.fov, camSettings.speed, camSettings.sensitivity);
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
				getWorldLight()->direction = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
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

		void SceneManager::setupFloor(SceneAsset const& scene_asset) {
			//Load floor settings from scene asset
			floor_enabled = scene_asset.floor.enabled;
			floor_position = scene_asset.floor.position;
			floor_extents = scene_asset.floor.halfExtents;

			//Update physics floor if physics system exists
			auto physics = services->get<ECS::Controller>()->getSystem<Physics::System>();
			if (physics) {
				physics->set_floor_enabled(floor_enabled);
			}

			PN_CORE_INFO("[SceneManager] Floor setup: enabled={}, pos=({:.2f}, {:.2f}, {:.2f}), extents=({:.2f}, {:.2f}, {:.2f})",
				floor_enabled, floor_position.x, floor_position.y, floor_position.z,
				floor_extents.x, floor_extents.y, floor_extents.z);
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

		void SceneManager::setupLoadingScreen(SceneAsset const& scene_asset) {
			if (!loadingScreen) {
				PN_CORE_WARN("[SceneManager] Loading screen not initialized");
				return;
			}

			// Get loading screen from scene asset
			const auto& ls = scene_asset.loadingScreen;

			// Background settings
			loadingScreen->setBackgroundTexture(ls.backgroundTextureGUID);
			loadingScreen->setBackgroundColor(ls.backgroundColor);
			loadingScreen->setBGScale(ls.bgScale);
			loadingScreen->setShowBG(ls.showBackground);
			loadingScreen->setShowOverlay(ls.showOverlay);

			// Progress bar settings
			loadingScreen->setProgressBarPosition(ls.progressBarPosition.x, ls.progressBarPosition.y);
			loadingScreen->setProgressBarSize(ls.progressBarSize.x, ls.progressBarSize.y);
			loadingScreen->setProgressBarFillColor(ls.fillColor);
			loadingScreen->setProgressBarGlowColor(ls.glowColor);
			loadingScreen->setProgressBarGlowIntensity(ls.glowIntensity);
			loadingScreen->setShowProgressBar(ls.showProgressBar);

			// Status text settings
			loadingScreen->setStatusTextPosition(ls.statusTextPosition.x, ls.statusTextPosition.y);
			loadingScreen->setStatusTextScale(ls.statusTextScale);
			loadingScreen->setShowStatusText(ls.showStatusText);

			// Spritesheet animation settings
			loadingScreen->setSpritesheetAnimation(ls.frameCount, ls.framesPerRow, ls.frameTime);
			loadingScreen->setAnimationEnabled(ls.animationEnabled);

			PN_CORE_INFO("[SceneManager] Loading screen setup complete");
		}

		void SceneManager::cacheSceneAssets(SceneAsset const& scene_asset) {

			//Get asset manager
			auto assetMananger = services->get<Assets::Manager>();

			//Unload the cache
			assetMananger->clearAssetCache();

			//Batch cache all assets from scene
			assetMananger->batchCacheAssets(scene_asset.assets_to_cache);

			//Batch upload all textures to GPU
			assetMananger->batchUploadAllCachedTextures();

			//Refresh editor resources only in debug mode
#ifdef _DEBUG
#ifdef PN_PLATFORM_WINDOWS
			{
				auto editor = services->get<Editor::Editor>();
				if (editor) {
					auto resource_panel = editor->getPanel<Editor::Panel::ResourcePanel>();
					if (resource_panel) resource_panel->refreshResources();
				}
			}
#endif
#endif
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

			// Identify Root Entities (No Parent)
			std::vector<entt::entity> roots;
			auto view = registry.view<Entity::Hierarchy>();

			for (auto [entity, hierarchy] : view.each()) {
				if (!hierarchy.parentGUID.IsValid()) {
					roots.push_back(entity);
				}
			}

			// Sort roots by sibling index
			std::stable_sort(roots.begin(), roots.end(), [&](entt::entity a, entt::entity b) {
				auto hA = controller->getEntityComponent<Entity::Hierarchy>(a);
				auto hB = controller->getEntityComponent<Entity::Hierarchy>(b);
				int idxA = hA ? hA->get().siblingIndex : 0;
				int idxB = hB ? hB->get().siblingIndex : 0;
				return idxA < idxB;
			});

			for (auto root : roots) {
				recursiveCapture( root, ents);
			}

			ecs["Entities"] = std::move(ents);

			PN_CORE_INFO("[SceneManager] Captured {} entities ", ents.size());

			return ecs;

			//auto view = registry.view<Entity::Name>();

			//for (auto e : view) {
			//	nlohmann::json E = nlohmann::json::object();

			//	// Get entity name
			//	if (auto nameOpt = controller->getEntityComponent<Entity::Name>(e)) {
			//		E["Name"] = nameOpt->get().name;
			//	}

			//	// Detect and save overrides for prefab instances
			//	if (prefabService && registry.any_of<Prefab::PrefabInstance>(e)) {
			//		prefabService->updateAllOverrides(e, ECS::MAIN_REGISTRY_ID);
			//	}

			//	// Serialize all components
			//	E["Components"] = controller->getAllComponentsAsJson(e);


			//	// Wrap in Entity object
			//	ents.push_back(nlohmann::json{ {"Entity", std::move(E)} });
			//}

			//ecs["Entities"] = std::move(ents);

			//PN_CORE_INFO("[SceneManager] Captured {} entities", ents.size());

			//return ecs;
		}

		void SceneManager::recursiveCapture(entt::entity entity, nlohmann::json& jsonArray)
		{

			auto controller = services->get<ECS::Controller>();
			auto prefabService = services->get<Prefab::Service>();

			// Serialize THIS entity
			nlohmann::json E = nlohmann::json::object();

			// Name
			if (auto nameOpt = controller->getEntityComponent<Entity::Name>(entity)) {
				E["Name"] = nameOpt->get().name;
			}

			// Prefab Overrides
			if (prefabService && controller->getRegistry().any_of<Prefab::PrefabInstance>(entity)) {
				prefabService->updateAllOverrides(entity, ECS::MAIN_REGISTRY_ID);
			}

			// Components (This includes siblingIndex automatically via reflection)
			E["Components"] = controller->getAllComponentsAsJson(entity);

			// Add to main JSON Array
			jsonArray.push_back(nlohmann::json{ {"Entity", std::move(E)} });

			// Find and Sort Children
			if (auto h = controller->getEntityComponent<Entity::Hierarchy>(entity)) {
				std::vector<entt::entity> children;
				for (const auto& childGUID : h.value().get().childrenGUIDs) {
					entt::entity child = controller->resolveGUID(childGUID);
					if (controller->checkEntity(child)) {
						children.push_back(child);
					}
				}

				// Sort Children by Sibling Index before saving
				std::stable_sort(children.begin(), children.end(), [&](entt::entity a, entt::entity b) {
					auto hA = controller->getEntityComponent<Entity::Hierarchy>(a);
					auto hB = controller->getEntityComponent<Entity::Hierarchy>(b);
					int idxA = hA ? hA->get().siblingIndex : 0;
					int idxB = hB ? hB->get().siblingIndex : 0;
					return idxA < idxB;
					});

				// Recurse
				for (auto child : children) {
					recursiveCapture(child, jsonArray);
				}
			}
		}

		void SceneManager::captureCachedAssets(SceneAsset& scene_asset) {

			//Clear scene asset GUIDS
			scene_asset.assets_to_cache.clear();

			//Get all models in the current ecs registry
			auto ecs = services->get<ECS::Controller>();
			auto& registry = ecs->getRegistry();
			auto view = registry.view<ModelRenderer>();
			auto assetManager = services->get<Assets::Manager>();

			//Lambda to cache function
			auto cacheAsset = [&](Assets::GUID const& id) {
				//Insert material GUID into asset to cache
				if (id.IsValid() && !scene_asset.assets_to_cache.count(id)) scene_asset.assets_to_cache.insert(id);
				};

			// Collect all unique models and build combined vertex/index buffers
			for (auto e : view) {

				//Get mdl asset
				auto mdl = ecs->getEntityComponent<ModelRenderer>(e);
				if (mdl.has_value()) {

					//Retrieve model GUID and insert
					Assets::GUID mdl_id = mdl.value().get().modelGUID;
					cacheAsset(mdl_id);

					//Retrieve material
					auto const& mat_vec = mdl.value().get().materials;
					for (auto const& mat_inst : mat_vec) {

						//optional material asset
						auto materialAssetOpt = assetManager->getAsset<Assets::Material>(mat_inst.materialGUID);

						// Load material asset
						auto materialAsset = materialAssetOpt.has_value() ? materialAssetOpt.value() : nullptr;

						//Check material asset
						if (materialAsset) {

							//Insert material GUID into asset to cache
							cacheAsset(mat_inst.materialGUID);

							//Insert textures within materials to cache
							{
								//Albedo Texture
								std::optional<std::shared_ptr<Assets::Texture>> tex_opt = mat_inst.useOverrides ?
									assetManager->getAsset<Assets::Texture>(mat_inst.albedoTextureOverride)
									: assetManager->getAsset<Assets::Texture>(materialAsset->albedoTexturePath);

								if (tex_opt.has_value()) {
									cacheAsset(tex_opt.value().get()->guid);
								}

								//Normal texture
								tex_opt = mat_inst.useOverrides ?
									assetManager->getAsset<Assets::Texture>(mat_inst.normalTextureOverride)
									: assetManager->getAsset<Assets::Texture>(materialAsset->normalTexturePath);

								if (tex_opt.has_value()) {
									cacheAsset(tex_opt.value().get()->guid);
								}

								//Metallic texture
								tex_opt = mat_inst.useOverrides ?
									assetManager->getAsset<Assets::Texture>(mat_inst.metallicTextureOverride)
									: assetManager->getAsset<Assets::Texture>(materialAsset->metallicTexturePath);

								if (tex_opt.has_value()) {
									cacheAsset(tex_opt.value().get()->guid);
								}

								//Roughness texture
								tex_opt = mat_inst.useOverrides ?
									assetManager->getAsset<Assets::Texture>(mat_inst.roughnessTextureOverride)
									: assetManager->getAsset<Assets::Texture>(materialAsset->roughnessTexturePath);

								if (tex_opt.has_value()) {
									cacheAsset(tex_opt.value().get()->guid);
								}

								//AO texture
								tex_opt = mat_inst.useOverrides ?
									assetManager->getAsset<Assets::Texture>(mat_inst.aoTextureOverride)
									: assetManager->getAsset<Assets::Texture>(materialAsset->aoTexturePath);

								if (tex_opt.has_value()) {
									cacheAsset(tex_opt.value().get()->guid);
								}

								//Emissive texture
								tex_opt = mat_inst.useOverrides ?
									assetManager->getAsset<Assets::Texture>(mat_inst.emissiveTextureOverride)
									: assetManager->getAsset<Assets::Texture>(materialAsset->emissiveTexturePath);

								if (tex_opt.has_value()) {
									cacheAsset(tex_opt.value().get()->guid);
								}

								//Height texture
								tex_opt = mat_inst.useOverrides ?
									assetManager->getAsset<Assets::Texture>(mat_inst.heightTextureOverride)
									: assetManager->getAsset<Assets::Texture>(materialAsset->heightTexturePath);

								if (tex_opt.has_value()) {
									cacheAsset(tex_opt.value().get()->guid);
								}

								//Opacity texture
								tex_opt = mat_inst.useOverrides ?
									assetManager->getAsset<Assets::Texture>(mat_inst.opacityTextureOverride)
									: assetManager->getAsset<Assets::Texture>(materialAsset->opacityTexturePath);

								if (tex_opt.has_value()) {
									cacheAsset(tex_opt.value().get()->guid);
								}
							}
						}
					}
				}

				//Get audio asset
				auto audio = ecs->getEntityComponent<Audio::AudioSource>(e);
				if (audio.has_value()) {

					//Cache the audio source
					cacheAsset(audio.value().get().selected_audio);
				}
			}
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
				scene_asset.camera.speed = active_camera->speed;
				scene_asset.camera.sensitivity = active_camera->sensitivity;
				
				// Camera collision settings
				scene_asset.camera.collisionEnabled = active_camera->collisionEnabled;
				scene_asset.camera.collisionRadius = active_camera->collisionRadius;
				scene_asset.camera.collisionOffset = active_camera->collisionOffset;
				scene_asset.camera.capsuleHeight = active_camera->capsuleHeight;
				scene_asset.camera.useCapsuleCollision = active_camera->useCapsuleCollision;
				scene_asset.camera.showCollisionGizmo = active_camera->showCollisionGizmo;

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

			//Capture floor settings
			scene_asset.floor.enabled = floor_enabled;
			scene_asset.floor.position = floor_position;
			scene_asset.floor.halfExtents = floor_extents;

			//Capture loading screen settings
			if (loadingScreen) {
				scene_asset.loadingScreen.backgroundTextureGUID = loadingScreen->getBackgroundTexture();
				scene_asset.loadingScreen.backgroundColor = loadingScreen->getBackgroundColor();
				scene_asset.loadingScreen.bgScale = loadingScreen->getBGScale();
				scene_asset.loadingScreen.showBackground = loadingScreen->getShowBG();
				scene_asset.loadingScreen.showOverlay = loadingScreen->getShowOverlay();
			
				scene_asset.loadingScreen.progressBarPosition = loadingScreen->getProgressBarPosition();
				scene_asset.loadingScreen.progressBarSize = loadingScreen->getProgressBarSize();
			
				auto [fillColor, glowColor, glowIntensity] = loadingScreen->getProgressBarStyle();
				scene_asset.loadingScreen.fillColor = fillColor;
				scene_asset.loadingScreen.glowColor = glowColor;
				scene_asset.loadingScreen.glowIntensity = glowIntensity;
				scene_asset.loadingScreen.showProgressBar = loadingScreen->getShowProgressBar();
			
				scene_asset.loadingScreen.statusTextPosition = loadingScreen->getStatusTextPosition();
				scene_asset.loadingScreen.statusTextScale = loadingScreen->getStatusTextScale();
				scene_asset.loadingScreen.showStatusText = loadingScreen->getShowStatusText();
			
				auto [frameCount, framesPerRow, frameTime, animEnabled] = loadingScreen->getSpritesheetSettings();
				scene_asset.loadingScreen.frameCount = frameCount;
				scene_asset.loadingScreen.framesPerRow = framesPerRow;
				scene_asset.loadingScreen.frameTime = frameTime;
				scene_asset.loadingScreen.animationEnabled = animEnabled;
			}
		}

		nlohmann::json SceneManager::convertSceneToJSON(SceneAsset& scn_asset) {
			//*** NOTE When adding variables to capture remember to add into AssetLoader.cpp as well to parse the json var
			
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
				{"aspectRatioH", scn_asset.camera.aspectRatioH},
				{"speed", scn_asset.camera.speed},
				{"sensitivity", scn_asset.camera.sensitivity},
				// Camera collision settings
				{"collisionEnabled", scn_asset.camera.collisionEnabled},
				{"collisionRadius", scn_asset.camera.collisionRadius},
				{"collisionOffset", scn_asset.camera.collisionOffset},
				{"capsuleHeight", scn_asset.camera.capsuleHeight},
				{"useCapsuleCollision", scn_asset.camera.useCapsuleCollision},
				{"showCollisionGizmo", scn_asset.camera.showCollisionGizmo}
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

			//Loading screen settings  
			sceneJson["loadingScreen"] = {
				{"backgroundTextureGUID", scn_asset.loadingScreen.backgroundTextureGUID.ToString()},
				{"backgroundColor", {scn_asset.loadingScreen.backgroundColor.r, scn_asset.loadingScreen.backgroundColor.g, scn_asset.loadingScreen.backgroundColor.b}},
				{"bgScale", scn_asset.loadingScreen.bgScale},
				{"showBackground", scn_asset.loadingScreen.showBackground},
				{"showOverlay", scn_asset.loadingScreen.showOverlay},

				{"progressBarPosition", {scn_asset.loadingScreen.progressBarPosition.x, scn_asset.loadingScreen.progressBarPosition.y}},
				{"progressBarSize", {scn_asset.loadingScreen.progressBarSize.x, scn_asset.loadingScreen.progressBarSize.y}},
				{"fillColor", {scn_asset.loadingScreen.fillColor.r, scn_asset.loadingScreen.fillColor.g, scn_asset.loadingScreen.fillColor.b}},
				{"glowColor", {scn_asset.loadingScreen.glowColor.r, scn_asset.loadingScreen.glowColor.g, scn_asset.loadingScreen.glowColor.b}},
				{"glowIntensity", scn_asset.loadingScreen.glowIntensity},
				{"showProgressBar", scn_asset.loadingScreen.showProgressBar},

				{"statusTextPosition", {scn_asset.loadingScreen.statusTextPosition.x, scn_asset.loadingScreen.statusTextPosition.y}},
				{"statusTextScale", scn_asset.loadingScreen.statusTextScale},
				{"showStatusText", scn_asset.loadingScreen.showStatusText},

				{"frameCount", scn_asset.loadingScreen.frameCount},
				{"framesPerRow", scn_asset.loadingScreen.framesPerRow},
				{"frameTime", scn_asset.loadingScreen.frameTime},
				{"animationEnabled", scn_asset.loadingScreen.animationEnabled}
			};

			// Layers
			nlohmann::json layersJson = nlohmann::json::array();
			for (const auto& layer : scn_asset.layers) {
				layersJson.push_back({
					{"id", layer.id},
					{"mask", layer.mask},
					{"enabled", layer.enabled},
					{"pickable", layer.pickable},
					{"name", layer.name},
					{"color",  {layer.color.r, layer.color.g, layer.color.b}}
					});
			}
			sceneJson["layers"] = layersJson;
			sceneJson["mask_matrix"] = scn_asset.mask_matrix;

			//Assets cache
			sceneJson["assets"] = scn_asset.assets_to_cache;

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
			PN_CORE_INFO("[SceneManager] Starting async scene configuration");
			// ========================================
			// PHASE 1: Main Thread Setup (Fast, No I/O)
			// ========================================
			setupLoadingScreen(scn_asset);
			setupCamera(scn_asset);
			setupEnvironment(scn_asset);
			setupFloor(scn_asset);
			setupLayers(scn_asset);

			// Clear existing asset cache
			auto assetManager = services->get<Assets::Manager>();
			assetManager->clearAssetCache();

			// ========================================
			// PHASE 2: Initialize Loading Screen
			// ========================================
			loadingScreen->setStatus("Initializing scene...");
			loadingScreen->setProgress(0.0f);
			loadingScreen->render();

			// ========================================
			// PHASE 3: Launch Worker Thread for Asset Loading
			// ========================================
			std::atomic<bool> loadingComplete{ false };
			std::atomic<bool> loadingFailed{ false };
			std::string errorMessage;
			std::thread workerThread([&]() {
				try {
					PN_CORE_INFO("[AsyncLoader] Worker thread started");

					// Step 1: Load all assets (CPU-only, thread-safe)
					loadingScreen->setStatus("Loading scene assets...");
					loadingScreen->setProgress(0.1f);
					const auto& assetGuids = scn_asset.assets_to_cache;
					size_t totalAssets = assetGuids.size();
					size_t loadedAssets = 0;
					PN_CORE_INFO("[AsyncLoader] Loading {} assets", totalAssets);
					for (const auto& guid : assetGuids) {
						if(assetManager->cacheAsset(guid)) loadedAssets++;
						float progress = 0.1f + (loadedAssets / (float)totalAssets) * 0.6f;
						loadingScreen->setProgress(progress);
						if (loadedAssets % 10 == 0) {
							PN_CORE_INFO("[AsyncLoader] Loaded {}/{} assets", loadedAssets, totalAssets);
						}
					}
					PN_CORE_INFO("[AsyncLoader] All assets loaded");

					// Step 2: Build entities (CPU-only)
					loadingScreen->setStatus("Building scene entities...");
					loadingScreen->setProgress(0.7f);
					if (!buildEntitiesFromAsset(scn_asset)) {
						throw std::runtime_error("Failed to build entities from scene asset");
					}
					loadingScreen->setProgress(0.9f);
					PN_CORE_INFO("[AsyncLoader] Worker thread complete");
				}
				catch (const std::exception& e) {
					PN_CORE_ERROR("[AsyncLoader] Worker thread failed: {}", e.what());
					errorMessage = e.what();
					loadingFailed.store(true);
				}
				loadingComplete.store(true);
				});

			// ========================================
			// PHASE 4: Render Loading Screen Loop
			// ========================================
			PN_CORE_INFO("[SceneManager] Entering loading screen render loop");
			while (!loadingComplete.load()) {
				loadingScreen->render();
				std::this_thread::sleep_for(std::chrono::milliseconds(16));
			}
			workerThread.join();
			PN_CORE_INFO("[SceneManager] Worker thread joined");
			if (loadingFailed.load()) {
				PN_CORE_ERROR("[SceneManager] Scene loading failed: {}", errorMessage);
				return;
			}

			// ========================================
			// PHASE 5: Finalize on Main Thread (GPU)
			// ========================================
			loadingScreen->setStatus("Uploading textures to GPU...");
			loadingScreen->setProgress(0.95f);
			loadingScreen->render();
			PN_CORE_INFO("[SceneManager] Uploading textures to GPU");
			assetManager->batchUploadAllCachedTextures();
			loadingScreen->setStatus("Building Model's VBO...");
			loadingScreen->setProgress(0.98f);
			loadingScreen->render();
			services->get<sRenderer>()->initSceneVbo();

#ifdef _DEBUG
#ifdef PN_PLATFORM_WINDOWS
			{
				auto editor = services->get<Editor::Editor>();
				if (editor) {
#ifdef PN_PLATFORM_WINDOWS
					auto resource_panel = editor->getPanel<Editor::Panel::ResourcePanel>();
					if (resource_panel) resource_panel->refreshResources();
#endif
				}
			}
#endif
#endif

			//Scene config completed
			loadingScreen->setProgress(1.0f);
			loadingScreen->setStatus("Scene loading Complete!");
			loadingScreen->render();
			loadingScreen->finish();
			PN_CORE_INFO("[SceneManager] Scene configuration complete");
		}

		void SceneManager::onAttach() {

			//Services
			auto ecs = services->get<ECS::Controller>();
			auto pathService = services->get<Path::Path>();
			auto asset_manager = services->get<Assets::Manager>();

			//Set scecne manager for renderer
			services->get<sRenderer>()->setScene(services->get<Scene::SceneManager>());

			//Init loading screen
			loadingScreen = std::make_unique<LoadingScreen>();
			loadingScreen->init(services);

			//Init camera collision system
			m_cameraCollisionSystem = std::make_unique<CameraCollisionSystem>();
			auto physicsSystem = ecs->getSystem<Physics::System>();
			if (physicsSystem) {
				void* physPtr = physicsSystem->GetPhysicsSystem();
				if (physPtr) {
					m_cameraCollisionSystem->init(physPtr);
					PN_CORE_INFO("[SceneManager] Camera collision system initialized");
				}
			}

			//Init skybox here, set texture for skybox in config scene
			Skybox::get().init(services);
			std::filesystem::path skybox_path = "engine/textures/skybox2.hdr";
			setCurrSkyBoxTexture(asset_manager->findGUID(skybox_path));
			PN_CORE_INFO("[SceneManager] Initialized skybox");

#ifdef _DEBUG
			// Demo Object and Audio Setup
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

					auto e = AddObject(mdl, "dm", { 0.f, 1.5f, 1.f }, glm::angleAxis(glm::radians(0.f), glm::vec3(1.0f, 0.0f, 0.0f)), { 1.f, 1.f, 1.f });
				}


#ifdef PN_PLATFORM_WINDOWS
				std::filesystem::path fox_path = "game/models/Fox.mesh";
#else	
				std::filesystem::path fox_path = "game\\models\\Fox.mesh";
#endif
				//Get model
				PN_CORE_INFO("Attempting to add {} to scene", fox_path.string());
				mdl_opt = asset_manager->getAsset<Assets::Model>(fox_path);
				if (mdl_opt.has_value()) {
					mdl = mdl_opt.value();

					auto e = AddObject(mdl, "fox", { 5.f, 1.5f, 1.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(1.0f, 0.0f, 0.0f)), glm::vec3{ 0.05f });
				}


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

#ifdef PN_PLATFORM_WINDOWS
				std::filesystem::path fh_path = "game/models/FrogAnim.mesh";
#else	
				std::filesystem::path fh_path = "game\\models\\FrogAnim.mesh";
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

				//Create default scene asset
				SceneAsset default_scene_config;

				//Configure scene with default settings
				configScene(default_scene_config);
			}
#else
			// Prep for subs
			std::filesystem::path init_scn_path = "game/scenes/mainmenu.scn";

			auto scn_opt = asset_manager->getAssetData(init_scn_path);

			if (scn_opt) {

				loadScene(scn_opt.get()->guid);
				//SetGameCamera();
			}
#endif

			//Log scene manager init
			PN_CORE_INFO("[SceneManager] Initialized");
		}

		void SceneManager::onDetach() {
			PN_CORE_INFO("[SceneManager] Shutting down");

			// Clean up current scene
			unloadScene();
		}

		void SceneManager::onEvent(Event::Event& e) {
			Event::Dispatcher dispatcher(e);
#ifdef PN_PLATFORM_WINDOWS
			dispatcher.Dispatch<Event::WindowFocused>([&](Event::WindowFocused& e) -> bool {
				if (e.checkWindowFocus()) {
					services->get<Audio::Audio>()->resumeAll();
				}
				else {
					services->get<Audio::Audio>()->pauseAll();
				}
				PN_CORE_INFO(e.toString());
				return false;
				});

#else
			dispatcher.Dispatch<Event::FocusLost>([&](Event::FocusLost& e) -> bool {
				services->get<Audio::Audio>()->pauseAll();
				PN_CORE_INFO("[SceneManager] {}", e.toString());
				return false;
				});
			dispatcher.Dispatch<Event::FocusGained>([&](Event::FocusGained& e) -> bool {
				services->get<Audio::Audio>()->resumeAll();
				PN_CORE_INFO("[SceneManager] {}", e.toString());
				return false;
				});

#endif
		}

		void SceneManager::initCameraCollisionSystem(void* physicsSystem) {
			if (!m_cameraCollisionSystem) {
				m_cameraCollisionSystem = std::make_unique<CameraCollisionSystem>();
			}
			m_cameraCollisionSystem->init(physicsSystem);
			PN_CORE_INFO("[SceneManager] Camera collision system initialized");
		}

		void SceneManager::onUpdate(AppTiming timing) {

#ifndef _DEBUG
			// !TODO: fix this impl for release
			SetGameCamera();
#endif

			// Daytime / Nighttime setting
			{
				if (gs.world_light) {

					auto olc = getWorldLight();

					if (!olc) {
						LightSources::get().create("world");
						getWorldLight()->L_intensity = glm::vec3(GraphicsSettings::get().global_light_intensity);
						getWorldLight()->direction = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
						getWorldLight()->setShadowType(Light::SHADOW_TYPES::MAPPED);
						getWorldLight()->type = Light::TYPES::DIRECTIONAL;
					}
					else {
						olc->position = GetActiveCamera()->pos - glm::normalize(olc->direction) * olc->shadow_source_follow_distance;
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
					auto new_cam = std::make_unique<Camera>(cam_pos, forward, up, GraphicsSettings::get().fov, near_plane, far_plane, width_ratio, height_ratio);
					// Copy collision settings from editor camera
					if (editor_camera) {
						new_cam->collisionEnabled = editor_camera->collisionEnabled;
						new_cam->collisionRadius = editor_camera->collisionRadius;
						new_cam->collisionOffset = editor_camera->collisionOffset;
						new_cam->capsuleHeight = editor_camera->capsuleHeight;
						new_cam->useCapsuleCollision = editor_camera->useCapsuleCollision;
					}
					game_cameras.insert(std::pair<std::string, std::unique_ptr<Camera>>(entity_name, std::move(new_cam)));

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

			//Scene loaded successfully
			PN_CORE_INFO("[SceneManager] Loaded scene {} from GUID: {}", currentSceneAsset->name, sceneGUID.ToString());
			curr_scene_id = sceneGUID;
			services->get<Serialization::Service>()->setCurrSceneFileName(currentSceneAsset->name);

			services->get<Serialization::Service>()->markSceneChanged();

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
				loadScene(id);
		}

		void SceneManager::changeScene(const Assets::GUID& sceneGUID) {
			if (sceneGUID.IsValid()) {
				// Only update previous scene if we are actually switching to a different scene
				// This preserves the "Back" history if we are just restarting the current level
				if (sceneGUID != curr_scene_id) {
					prev_scene_id = curr_scene_id;
				}

				PN_CORE_INFO("[SceneManager] Scheduling scene transition to GUID: {}", sceneGUID.ToString());
				pending_scene_change = true;
				next_scene_guid = sceneGUID;
			}
		}

		void SceneManager::processPendingSceneChange()
		{
			// If no change is pending, do nothing
			if (!pending_scene_change) return;

			PN_CORE_INFO("[SceneManager] Executing pending scene transition...");

			pending_scene_change = false;

			auto assetManager = services->get<Assets::Manager>();
			if (!assetManager) {
				PN_CORE_ERROR("[SceneManager] Asset Manager not available");
				return;
			}

			// Get scene asset by GUID
			auto sceneOpt = assetManager->getAsset<SceneAsset>(next_scene_guid);
			if (!sceneOpt.has_value()) {
				PN_CORE_ERROR("[SceneManager] Failed to load scene with GUID: {}", next_scene_guid.ToString());
				return;
			}

			//Get scene asset
			auto currentSceneAsset = sceneOpt.value();

			//Clear old scene
			PN_CORE_INFO("[SceneManager] Unloading current scene");

			// reset lua scripting state before destroying entities
			//auto controller = services->get<ECS::Controller>();
			//PN_CORE_INFO("[SceneManager::onStop] Resetting Lua scripting state...");
			//auto scriptingSystem = controller->getSystem<Scripting::GameScriptingSystem>();
			//if (scriptingSystem) {
			//	scriptingSystem->getLuaManager().resetForSceneReload();
			//	PN_CORE_INFO("[SceneManager::onStop] Lua state reset complete");
			//}
			//else {
			//	PN_CORE_WARN("[SceneManager::onStop] GameScriptingSystem not found!");
			//}

			// Destroy all ECS entities
			auto controller = services->get<ECS::Controller>();
			if (controller) {
				controller->destroyAllEntities();
				PN_CORE_INFO("[SceneManager] Cleared all entities");
			}

			//Reset with default
			curr_scene_id = Assets::GUID();
			services->get<Serialization::Service>()->setCurrSceneFileName("");

			// Reset camera
			active_game_cam = "";

			//Configure the new scene
			if (currentSceneAsset) {
				configScene(*currentSceneAsset);
			}
			else {
				PN_CORE_INFO("[SceneManager] Failed to configure scene with GUID: {}", next_scene_guid.ToString());
				return;
			}

			//Scene changed successfully
			PN_CORE_INFO("[SceneManager] Changed scene to {} from GUID: {}", currentSceneAsset->name, next_scene_guid.ToString());
			curr_scene_id = next_scene_guid;
			services->get<Serialization::Service>()->setCurrSceneFileName(currentSceneAsset->name);

			setPlaying(true);
			setGamePaused(false);
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

			if (is_playing) {
				PN_CORE_WARN("[SceneManager] Cannot save scene while Game is Playing! Please Stop first.");
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

			//Capture assets to cache
			captureCachedAssets(*currentSceneAsset);

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

			if (is_playing) {
				setPlaying(false);
			}

			// reset lua scripting state before destroying entities
			auto controller = services->get<ECS::Controller>();
			PN_CORE_INFO("[SceneManager::onStop] Resetting Lua scripting state...");
			auto scriptingSystem = controller->getSystem<Scripting::GameScriptingSystem>();
			if (scriptingSystem) {
				scriptingSystem->getLuaManager().resetForSceneReload();
				PN_CORE_INFO("[SceneManager::onStop] Lua state reset complete");
			}
			else {
				PN_CORE_WARN("[SceneManager::onStop] GameScriptingSystem not found!");
			}

			// Destroy all ECS entities
			if (controller) {
				controller->destroyAllEntities();
				PN_CORE_INFO("[SceneManager] Cleared all entities");
			}

			//Reset with default
			curr_scene_id = Assets::GUID();
			services->get<Serialization::Service>()->clearModifiedFlag();
			services->get<Serialization::Service>()->setCurrSceneFileName("");


			// Reset camera (but don't destroy it - we'll reuse it)
			active_game_cam = "";
			
			// Clear scene asset reference (but keep the object if we're reloading)
			// currentSceneAsset.reset();
		}

		void SceneManager::onPlay()
		{
			PN_CORE_INFO("PLAY");
			captureSceneVariables(scene_snapshot);
			scene_snapshot.entityData = captureCurrentEntities();
			guid_snapshot = curr_scene_id;
			setPlaying(true);
		}

		void SceneManager::onStop()
		{
			PN_CORE_INFO("STOPPED");
			auto controller = services->get<ECS::Controller>();

			services->get<Audio::Audio>()->stopAll();

			// reset lua scripting state before destroying entities
			PN_CORE_INFO("[SceneManager::onStop] Resetting Lua scripting state...");
			auto scriptingSystem = controller->getSystem<Scripting::GameScriptingSystem>();
			if (scriptingSystem) {
				scriptingSystem->getLuaManager().resetForSceneReload();
				PN_CORE_INFO("[SceneManager::onStop] Lua state reset complete");
			}
			else {
				PN_CORE_WARN("[SceneManager::onStop] GameScriptingSystem not found!");
			}
			
			// Clears all entities
			controller->destroyAllEntities();
			// Clear GUID mappings
			controller->getGUIDRegistry().clear();

			// Restore env snapshot variables
			setupLoadingScreen(scene_snapshot);
			if (curr_scene_id != guid_snapshot) {
				curr_scene_id = guid_snapshot;
				setupCamera(scene_snapshot);
			}
			setupEnvironment(scene_snapshot);
			setupLayers(scene_snapshot);

			// Rebuild entites from snapshot
			buildEntitiesFromAsset(scene_snapshot);	

			services->get<Serialization::Service>()->markSceneChanged();

			setPlaying(false);

		}

		void SceneManager::setPlaying(bool playing)
		{
			if (playing)
			{
				is_playing = true;
			}
			else {
				is_game_paused = false;
				is_playing = false;
			}
		}

		void SceneManager::setGamePaused(bool paused)
		{
			is_game_paused = paused;
		}


		/* =========================================================================== */
		/*                            ENVIRONMENT                                      */
		/* =========================================================================== */


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

		void SceneManager::setCurrSkyBoxTexture(Assets::GUID const& skybox_id) {
			curr_skybox_id = skybox_id;
			Skybox::get().setTexture(curr_skybox_id);
		}

		/* =========================================================================== */
		/*                                LAYERS                                       */
		/* =========================================================================== */

		bool SceneManager::isLayerEnabled(int layer_id) {
			for (const auto& layer : layers) {
				if (layer.id == layer_id) {
					return layer.enabled;
				}
			}
			return true;
		}

		bool SceneManager::canLayersInteract(int layer1, int layer2) const {
			if (layer1 < 0 || layer2 < 0) return true;  // Invalid = allow
			if (layer1 >= mask_matrix.size()) return true;
			if (layer2 >= mask_matrix[layer1].size()) return true;

			return mask_matrix[layer1][layer2];
		}

		int SceneManager::getPickingMask() const
		{
			int mask = 0;
			for (const auto& layer : layers) {
				if (layer.enabled && layer.pickable) {
					mask |= layer.mask; // (1 << layer.id)
				}
			}
			return mask;
		}

	}
}