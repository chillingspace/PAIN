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
#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/GraphicsSettings.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Renderer/skybox.h"

#ifdef _DEBUG
#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#endif

namespace PAIN {
	namespace Scene {

		entt::entity SceneManager::AddObject(const std::shared_ptr<Assets::Model>& mdl,  const std::string& name, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale, Assets::GUID const& diff_id, Assets::GUID const& ao_id)
		{
			auto ecs = services->get<ECS::Controller>();
			auto meta = services->get<MetaData::Service>();

			//// if animated object, may be in T pose. must find root xform
			//glm::vec3 root_scale = glm::vec3(1.f);
			//glm::quat root_rot= glm::quat(1.f, 0.f, 0.f, 0.f);
			//glm::vec3 root_trans = glm::vec3(0.f);

			//PN_CORE_INFO("Model {} has {} animations", mdl->vpath, mdl->animations.size());
			//if (mdl->animations.size()) {

			//	auto anim = mdl->animations[0];
			//	for (const auto& [bone_name, track] : anim.track_map) {
			//		const auto it = std::find_if(mdl->skeleton.begin(), mdl->skeleton.end(), [&bone_name](const Assets::Bone& b) { return bone_name == b.name; });
			//		if (it == mdl->skeleton.end()) {
			//			root_scale = track[0].scale;
			//			root_rot = track[0].rotation;
			//			root_trans = track[0].translation;
			//			break;
			//		}
			//	}
			//}

			//// i suppose i shouldnt bake the root xform into LocalTransform here. so
			////glm::mat4 root_xform = 
			////	glm::translate(glm::mat4(1.f), root_trans) * 
			////	glm::mat4_cast(root_rot) *
			////	glm::scale(glm::mat4(1.f), root_scale);



			entt::entity entity = ecs->createEntity();
			ecs->addEntityComponent(entity, Entity::Name{ name });
			ecs->addEntityComponent(entity, LocalTransform{ pos, rot, scale });
			ecs->addEntityComponent(entity, WorldTransform{});
			ecs->addEntityComponent(entity, Entity::Hierarchy{});
			// ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
		
			ModelRenderer mr = ModelRenderer{ mdl->guid };
			if (mdl->animations.size()) {
				//mr.isPlaying = true;
				//mr.currentAnimationIndex = 0;

				mr.PlayAnimation(0);
			}

			ecs->addEntityComponent(entity, static_cast<ModelRenderer>(mr));

			if (meta) meta->setEntityName(entity, name);

			return entity;
		}

		bool SceneManager::buildEntitiesFromAsset(std::shared_ptr<SceneAsset> scene_asset) {

			if (!scene_asset) {
				PN_CORE_ERROR("[SceneManager] No scene asset loaded");
				return false;
			}

			if (scene_asset->entityData.empty()) {
				PN_CORE_WARN("[SceneManager] Scene has no entity data");
				return true; // Not an error, just an empty scene
			}

			auto controller = services->get<ECS::Controller>();
			if (!controller) {
				PN_CORE_ERROR("[SceneManager] ECS Controller not available");
				return false;
			}

			const auto& ecs = scene_asset->entityData;

			// Find the Entities array
			auto entsIt = ecs.find("Entities");
			if (entsIt == ecs.end() || !entsIt->is_array()) {
				PN_CORE_WARN("[SceneManager] No 'Entities' array in scene data");
				return true;
			}

			// PASS 1: Create all entities with their GUIDs
			std::vector<std::pair<entt::entity, const nlohmann::json*>> entityPairs;

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
					PN_CORE_TRACE("[SceneManager] Created entity {} with GUID {}",
						static_cast<uint32_t>(e), entityGuid.ToString());
				}
				else {
					e = controller->createEntity();
					PN_CORE_WARN("[SceneManager] Created entity {} without GUID",
						static_cast<uint32_t>(e));
				}

				// Store for second pass
				entityPairs.emplace_back(e, &E);
			}

			PN_CORE_INFO("[SceneManager] PASS 1 Complete: Created {} entities", entityPairs.size());

			// PASS 2: Deserialize all components
			PN_CORE_INFO("[SceneManager] PASS 2: Deserializing components");

			for (const auto& [e, Eptr] : entityPairs) {
				const auto& E = *Eptr;

				// Deserialize all components from the Components object
				if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
					controller->loadAllComponentsFromJson(e, *compsIt);
				}

				// Fallback: Add Name component if not present
				if (!controller->hasEntityComponent<Entity::Name>(e)) {
					if (auto n = E.find("Name"); n != E.end() && n->is_string()) {
						controller->addEntityComponent(e, Entity::Name{ n->get<std::string>() });
					}
					else {
						controller->addEntityComponent(e, Entity::Name{ "Entity " + std::to_string(static_cast<uint32_t>(e)) });
					}
				}
			}

			PN_CORE_INFO("[SceneManager] PASS 2 Complete: Deserialized components for {} entities", entityPairs.size());
			PN_CORE_INFO("[SceneManager] Successfully built {} entities from scene asset", entityPairs.size());

			return true;
		}

		void SceneManager::setupCamera(std::shared_ptr<SceneAsset> scene_asset) {
			if (!scene_asset) {
				PN_CORE_ERROR("[SceneManager] No scene asset to setup camera from");
				return;
			}

			const auto& camSettings = scene_asset->camera;

			// Create or update camera
			camera = std::make_unique<Camera>(
				camSettings.position,
				camSettings.forward,
				camSettings.up,
				camSettings.fov,
				camSettings.nearPlane,
				camSettings.farPlane,
				camSettings.aspectRatioW,
				camSettings.aspectRatioH
			);

			PN_CORE_INFO("[SceneManager] Camera setup: pos({}, {}, {}), fov={}",
				camSettings.position.x, camSettings.position.y, camSettings.position.z,
				camSettings.fov);
		}

		void SceneManager::setupEnvironment(std::shared_ptr<SceneAsset> scene_asset) {
			if (!scene_asset) {
				PN_CORE_ERROR("[SceneManager] No scene asset to setup environment from");
				return;
			}

			const auto& env = scene_asset->environment;

			// Setup ambient light (camera light)
			LightSources::get().create("cam");
			if (auto camLightOpt = LightSources::get().get("cam")) {
				Light& camLight = camLightOpt.value();
				camLight.L_intensity = env.ambientColor * env.ambientIntensity * 0.01f;
			}

			// Setup directional light if daytime is enabled
			if (env.useDaytime) {
				LightSources::get().create("world");
				if (auto worldLightOpt = LightSources::get().get("world")) {
					Light& worldLight = worldLightOpt.value();
					worldLight.forward = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
					worldLight.L_intensity = glm::vec3(env.globalLightIntensity);
					worldLight.setShadowType(Light::SHADOW_TYPES::MAPPED);
					worldLight.type = Light::TYPES::DIRECTIONAL;

					PN_CORE_INFO("[SceneManager] Created directional world light");
				}

				GraphicsSettings::get().ibl = true;
			}
			else {
				GraphicsSettings::get().ibl = false;
			}

			// Load skybox if GUID is valid
			if (env.skyboxGUID.IsValid()) {
				auto pathService = services->get<Path::Path>();
				if (pathService) {
					// TODO: Resolve skybox path from GUID and load
					// For now, use default skybox
#ifdef PN_PLATFORM_WINDOWS
					std::filesystem::path sb_path = "engine/textures/skybox2.hdr";
#else
					std::filesystem::path sb_path = "engine\\\\textures\\\\skybox2.hdr";
#endif

					Skybox::get().init(services, sb_path);
					PN_CORE_INFO("[SceneManager] Initialized skybox");
				}
			}

			PN_CORE_INFO("[SceneManager] Environment setup complete");
		}

		nlohmann::json SceneManager::captureCurrentEntities() {
			nlohmann::json ecs = nlohmann::json::object();
			nlohmann::json ents = nlohmann::json::array();

			auto controller = services->get<ECS::Controller>();
			if (!controller) {
				PN_CORE_ERROR("[SceneManager] ECS Controller not available");
				return ecs;
			}

			// Iterate all entities with Name component
			auto& registry = controller->getRegistry();
			auto view = registry.view<Entity::Name>();

			for (auto e : view) {
				nlohmann::json E = nlohmann::json::object();

				// Get entity name
				if (auto nameOpt = controller->getEntityComponent<Entity::Name>(e)) {
					E["Name"] = nameOpt->get().name;
				}
				else {
					E["Name"] = "Entity " + std::to_string(static_cast<uint32_t>(e));
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

		nlohmann::json SceneManager::convertSceneToJSON(SceneAsset& scn_asset) {
			//Convert scene asset to JSON
			nlohmann::json sceneJson = nlohmann::json::object();

			//Camera settings
			sceneJson["camera"] = {
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
				{"ambientColor", {scn_asset.environment.ambientColor.x, scn_asset.environment.ambientColor.y, scn_asset.environment.ambientColor.z}},
				{"ambientIntensity", scn_asset.environment.ambientIntensity},
				{"skyboxGUID", scn_asset.environment.skyboxGUID.ToString()},
				{"useDaytime", scn_asset.environment.useDaytime},
				{"globalLightIntensity", scn_asset.environment.globalLightIntensity}
			};

			// Layers
			nlohmann::json layersJson = nlohmann::json::array();
			for (const auto& layer : scn_asset.layers) {
				layersJson.push_back({
					{"id", layer.id},
					{"mask", layer.mask},
					{"enabled", layer.enabled}
					});
			}
			sceneJson["layers"] = layersJson;
			sceneJson["mask_matrix"] = scn_asset.mask_matrix;

			// Entity data
			sceneJson["ecs"] = scn_asset.entityData;

			return sceneJson;
		}

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

		void SceneManager::onAttach() {

			auto ecs = services->get<ECS::Controller>();
			
			// Camera and Scene Setup
			glm::vec3 pos{ 0.f, 2.f, 4.f };
			//glm::vec3 forward{-glm::normalize(pos)};
			glm::vec3 forward{ 0.f, 0.f, -1.f };
			glm::vec3 up{ 0.f, 1.f, 0.f };
			float near_plane{ 0.1f };
			float far_plane{ 100.f };
			float width_ratio{ 16.f };
			float height_ratio{ 9.f };
			camera = std::make_unique<Camera>(pos, forward, up, GraphicsSettings::get().fov, near_plane, far_plane, width_ratio, height_ratio);
			
			// Init light sources (REQUIRED TO FUNCTION)
			LightSources::get().create("cam");
			auto olcam = LightSources::get().get("cam");
			Light& lcam = olcam.value();
			lcam.L_intensity = glm::vec3(0.01f);
			
			if (GraphicsSettings::get().daytime) {
				LightSources::get().create("world");
				auto olc = LightSources::get().get("world");
				Light& lc = olc.value();
				lc.forward = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
				//lc.position = -lc.forward * 10.f;					// follows camera
				lc.L_intensity = glm::vec3(GraphicsSettings::get().global_light_intensity);
				lc.setShadowType(Light::SHADOW_TYPES::MAPPED);
				lc.type = Light::TYPES::DIRECTIONAL;
			}
			//lc.far_plane = 200.f;
			//lc.forward = -lc.position;
			
			// Demo Object and Audio Setup
			auto audioManager = services->get<Audio::Audio>();
			auto pathService = services->get<Path::Path>();
			auto asset_manager = services->get<Assets::Manager>();
			
			auto ogre_diff = Assets::GUID("5923aab8-5293-f945-958e-496acd0218c3");
			auto ogre_smile_ao = Assets::GUID("cee03212-928a-6347-9d55-07fe46ac3ea1");
			
			// for .mesh(converted from .obj only)
			std::optional<std::shared_ptr<Assets::Model>> mdl_opt;
			std::shared_ptr<Assets::Model> mdl;
			{
	#ifdef PN_PLATFORM_WINDOWS
				std::filesystem::path ogre_smile_path = "game/models/ogre_smile.mesh";
	#else	
				std::filesystem::path ogre_smile_path = "game\\models\\ogre_smile.mesh";
	#endif
			
				//Get model
				mdl_opt = asset_manager->getAsset<Assets::Model>(ogre_smile_path);
				if (mdl_opt.has_value()) {
					mdl = mdl_opt.value();
			
					// logging to check data
					{
						PN_CORE_TRACE("File: {}\nVertices: {}\nIndices: {}\nMaterials: {}", mdl->vpath, mdl->vertices.size(), mdl->indices.size(), mdl->materials.size());
						//PN_CORE_TRACE("Base roughness: {}\nBase metallic: {}\nBase color: {},{},{}", mdl->materials[0].roughness, mdl->materials[0].metallic, mdl->materials[0].baseColor.r, mdl->materials[0].baseColor.g, mdl->materials[0].baseColor.b);
					}
			
					AddObject(mdl, "ogre_smile", { 0.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f }, ogre_diff, ogre_smile_ao);
				}
			}
			
	#ifdef PN_PLATFORM_WINDOWS
			std::filesystem::path ogre_path = "game/models/ogre.mesh";
	#else	
			std::filesystem::path ogre_path = "game\\models\\ogre.mesh";
	#endif
			//Get model
			mdl_opt = asset_manager->getAsset<Assets::Model>(ogre_path);
			if (mdl_opt.has_value()) {
				mdl = mdl_opt.value();
			
				AddObject(mdl, "ogre_left", { -2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
			
				AddObject(mdl, "ogre_left", { -2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f }, ogre_diff, ogre_smile_ao);
				AddObject(mdl, "ogre_right", { 2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f }, ogre_diff, ogre_smile_ao);
			}
			
	#ifdef PN_PLATFORM_WINDOWS
			std::filesystem::path sdcc_path = "game/models/sdcc.mesh";
	#else	
			std::filesystem::path sdcc_path = "game\\models\\sdcc.mesh";
	#endif
			
			auto sdcc_diff = Assets::GUID("71051859-f5ee-144a-b1e5-59ad02d13695");
			//Get model
			mdl_opt = asset_manager->getAsset<Assets::Model>(sdcc_path);
			if (mdl_opt.has_value()) {
				mdl = mdl_opt.value();
			
				AddObject(mdl, "sdcc", { 5.f, 0.f, -3.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f)), { 3.f, 3.f, 3.f });
			}
			
	#ifdef PN_PLATFORM_WINDOWS
			std::filesystem::path city_path = "game/models/city.mesh";
	#else	
			std::filesystem::path city_path = "game\\models\\city.mesh";
	#endif
			
			auto city_diff = Assets::GUID{ "29fe999b-d257-bf41-879d-6d7578d43734" };
			//Get model
			mdl_opt = asset_manager->getAsset<Assets::Model>(city_path);
			if (mdl_opt.has_value()) {
				mdl = mdl_opt.value();
			
				AddObject(mdl, "city", { -8.f, 0.f, -5.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f)), { 3.f, 3.f, 3.f });
			}
			
	#ifdef PN_PLATFORM_WINDOWS
			std::filesystem::path dm_path = "game/models/damagedhelmet/DamagedHelmet.mesh";
	#else	
			std::filesystem::path dm_path = "game\\models\\damagedhelmet\\DamagedHelmet.mesh";
	#endif
			//Get model
			mdl_opt = asset_manager->getAsset<Assets::Model>(dm_path);
			if (mdl_opt.has_value()) {
				mdl = mdl_opt.value();
			
				auto e = AddObject(mdl, "dm", { 0.f, 1.5f, 1.f }, glm::angleAxis(glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f)), { 1.f, 1.f, 1.f });
			}
			
			
	#ifdef PN_PLATFORM_WINDOWS
			std::filesystem::path bs_path = "game/models/brainstem/BrainStem.mesh";
	#else	
			std::filesystem::path bs_path = "game\\models\\brainstem\\BrainStem.mesh";
	#endif
			//Get model
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
			std::filesystem::path fh_path = "game/models/Frog_Hopping.mesh";
	#else	
			std::filesystem::path fh_path = "game\\models\\Frog_Hopping.mesh";
	#endif
			//Get model
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
			
			// font
			TextRenderer::get();
			
	#ifdef PN_PLATFORM_WINDOWS
			std::filesystem::path sb_path = "engine/textures/skybox2.hdr";
	#else
			std::filesystem::path sb_path = "engine\\textures\\skybox2.hdr";
	#endif
			
			// skybox
			Skybox::get().init(services, sb_path);

			//Log scene manager init
			PN_CORE_INFO("[SceneManager] Initialized");
		}

		void SceneManager::onDetach() {
			PN_CORE_INFO("[SceneManager] Shutting down");

			// Clean up current scene
			unloadScene();
		}

		void SceneManager::onUpdate(AppTiming timing) {

			// Update camera if exists
			if (camera) {
				// Camera update logic can go here if needed
				// For now, the camera is controlled externally (editor, player controller, etc.)
			}

			//// Update animations (if you want this here instead of in ECS systems)
			//auto ecs = services->get<ECS::Controller>();
			//if (!ecs) return;

			//auto& registry = ecs->getRegistry();
			//auto view = registry.view<ModelRenderer>();
			//for (auto e : view) {
			//	auto mdl = ecs->getEntityComponent<ModelRenderer>(e);
			//	if (mdl.has_value()) {
			//		mdl->get().UpdateAnimation(timing.dt);
			//	}
			//}

			// Get time scale from ViewportPanel (0.0 when paused, 1.0 when playing)
			float timeScale = 1.0f;
			bool isPaused = false;

	#ifdef _DEBUG
			if (auto viewport = services->get<Editor::Panel::ViewportPanel>()) {
				timeScale = viewport->getTimeScale();
				isPaused = (timeScale == 0.0f);
			}
	#endif

			// Daytime / Nighttime setting
			{
				if (GraphicsSettings::get().daytime) {

					auto olc = LightSources::get().get("world");

					if (!olc) {
						LightSources::get().create("world");
						auto olc = LightSources::get().get("world");
						Light& lc = olc.value();
						lc.forward = glm::normalize(glm::vec3{ -0.5f, -0.5f, -0.2f });
						//lc.position = -lc.forward * 10.f;					// follows camera
						lc.L_intensity = glm::vec3(GraphicsSettings::get().global_light_intensity);
						lc.setShadowType(Light::SHADOW_TYPES::MAPPED);
						lc.type = Light::TYPES::DIRECTIONAL;
						GraphicsSettings::get().ibl = true;
					}
				}
				else {
					auto olc = LightSources::get().get("world");

					if (olc) {
						LightSources::get().destroy("world");
						GraphicsSettings::get().ibl = false;
					}
				}
			}

			if (GraphicsSettings::get().daytime) {
				auto olc = LightSources::get().get("world");
				Light& lc = olc.value();
				lc.position = GetActiveCamera()->pos - glm::normalize(lc.forward) * lc.shadow_source_follow_distance;
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

			//Build new scene
			setupCamera(currentSceneAsset);
			setupEnvironment(currentSceneAsset);

			//Failed to build entities
			if (!buildEntitiesFromAsset(currentSceneAsset)) {
				PN_CORE_ERROR("[SceneManager] Failed to build entities from scene asset");
				return;
			}

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

		void SceneManager::deleteScene() {
			//auto assetManager = services->get<Assets::Manager>();
			//auto path_service = services->get<Path::Path>();
			//if (!assetManager || !path_service) {
			//	PN_CORE_ERROR("[SceneManager] Asset | Path Manager not available");
			//	return false;
			//}

			////Remove scene
			//assetManager->removeFile(path_service->resolvePath(Path::main_assets_alias, curr_scene_path.string()));
		}

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

			//Update camera settings from current camera
			if (camera) {
				currentSceneAsset->camera.position = camera->pos;
				currentSceneAsset->camera.forward = camera->forward;
				currentSceneAsset->camera.up = camera->up;
				currentSceneAsset->camera.nearPlane = camera->near_plane;
				currentSceneAsset->camera.farPlane = camera->far_plane;
				currentSceneAsset->camera.fov = camera->fov;
				currentSceneAsset->camera.aspectRatioW = camera->width_ratio;
				currentSceneAsset->camera.aspectRatioH = camera->height_ratio;
			}

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
			if(!sceneOpt.has_value()) delete currentSceneAsset;
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
			// Camera will be reconfigured when a new scene loads

			// Clear scene asset reference (but keep the object if we're reloading)
			// currentSceneAsset.reset();
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
