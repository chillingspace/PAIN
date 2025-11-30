#include "Scene.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Assets/sAssets.h"
#include "ECS/Controller.h"
#include "ECS/sMetaData.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cTransform.h"
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
	void Scene::onDetach() {}

	void Scene::onAttach()
	{

		auto ecs = services->get<ECS::Controller>();

		// Camera and Scene Setup
		glm::vec3 pos{ 0.f, 2.f, 4.f };
		glm::vec3 forward{ -glm::normalize(pos) };
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

				//mdl->materials[0].gl_diffuse_tex = ogre_diffuse_tex->gl_texture;
				//mdl->materials[0].gl_ao_tex = ogre_smile_ao_map->gl_texture;
				//mdl->materials[0].metallic = 0.f;
				//mdl->materials[0].roughness = 1.f;
				//mdl->materials[0].baseColor = { 1, 0, 1 };

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
			//mdl->materials[0].gl_diffuse_tex = ogre_diffuse_tex->gl_texture;
			//mdl->materials[0].gl_ao_tex = ogre_smile_ao_map->gl_texture;
			//mdl->materials[0].metallic = 0.f;
			//mdl->materials[0].roughness = 1.f;
			//mdl->materials[0].baseColor = { 1, 0, 1 };
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
			//mdl->materials[0].gl_diffuse_tex = sdcc_tex->gl_texture;
			//mdl->materials[0].metallic = 1.f;
			//mdl->materials[0].roughness = 0.f;
			//mdl->materials[0].baseColor = { 1, 0, 1 };
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
			//mdl->materials[0].gl_diffuse_tex = city_tex->gl_texture;
			//mdl->materials[0].metallic = 0.f;
			//mdl->materials[0].roughness = 1.f;
			//mdl->materials[0].baseColor = { 0, 1, 0 };
			AddObject(mdl, "city", { -8.f, 0.f, -5.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f)), { 3.f, 3.f, 3.f });
		}

#ifdef PN_PLATFORM_WINDOWS
		std::filesystem::path crumpled_path = "game/models/damagedhelmet/DamagedHelmet.mesh";
#else	
		std::filesystem::path crumpled_path = "game\\models\\damagedhelmet\\DamagedHelmet.mesh";
#endif
		//Get model
		mdl_opt = asset_manager->getAsset<Assets::Model>(crumpled_path);
		if (mdl_opt.has_value()) {
			mdl = mdl_opt.value();
			//mdl->materials[0].metallic = 0.f;
			//mdl->materials[0].roughness = 1.f;
			//mdl->materials[0].baseColor = { 0.3f, 0.3f, 0.3f };
			auto e = AddObject(mdl, "dm", { 0.f, 1.5f, 1.f }, glm::angleAxis(glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f)), { 1.f, 1.f, 1.f });
		}



		// gltf testing
//#define GLTF_TEST
#ifdef GLTF_TEST
		{
			mdl = cacheModel("game_assets://models/930/930.mesh");
			AddObject(mdl, "930", { 0.f, 1.f, -5.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		}
#endif


		// font
		TextRenderer::get();

#ifdef PN_PLATFORM_WINDOWS
		std::filesystem::path sb_path = "engine/textures/skybox2.hdr";
#else
		std::filesystem::path sb_path = "engine\\textures\\skybox2.hdr";
#endif

		// skybox
		Skybox::get().init(services, sb_path);

		// Test load prefab
		//std::vector<entt::entity> loaded_entities = services->get<Serialization::Service>()->loadPrefabFromFile("ogre_right.prefab");
		//for (auto e : loaded_entities) {
		//	// Info: Print entity names, transforms, etc.
		//	auto nameOpt = services->get<ECS::Controller>()->getEntityComponent<MetaData::EntityName>(e);
		//	std::string name = nameOpt ? nameOpt->get().name : "<no name>";
		//}

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

	void Scene::onUpdate(AppTiming timing)
	{
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

	void Scene::onEvent(Event::Event& e) {}

	entt::entity Scene::AddObject(const std::shared_ptr<Assets::Model>& mdl,  const std::string& name, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale, Assets::GUID const& diff_id, Assets::GUID const& ao_id)
	{
		auto ecs = services->get<ECS::Controller>();
		auto meta = services->get<MetaData::Service>();

		entt::entity entity = ecs->createEntity();
		ecs->addEntityComponent(entity, MetaData::EntityName{ name });
		ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
		ecs->addEntityComponent(entity, ModelRenderer{ mdl->guid });

		if (meta) meta->setEntityName(entity, name);

		return entity;
	}

	Camera* Scene::GetActiveCamera()
	{
		return camera.get();
	}
}
