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
	void Scene::onDetach() {}

	void Scene::onAttach()
	{

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
		editor_camera = std::make_unique<Camera>(pos, forward, up, GraphicsSettings::get().fov, near_plane, far_plane, width_ratio, height_ratio);
		SetActiveCamera(editor_camera.get());

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
		//	auto nameOpt = services->get<ECS::Controller>()->getEntityComponent<Entity::Name>(e);
		//	std::string name = nameOpt ? nameOpt->get().name : "<no name>";
		//}





		

		if (!game_camera) {
			game_camera = std::make_unique<Camera>(pos, forward, up, GraphicsSettings::get().fov, near_plane, far_plane, width_ratio, height_ratio);
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

		auto ecs = services->get<ECS::Controller>();
		auto& registry = ecs->getRegistry();
		auto view = registry.view<Entity::Name>();

		for (auto e : view) {


			// animation
			auto mdl = ecs->getEntityComponent<ModelRenderer>(e);
			if (mdl.has_value()) mdl->get().UpdateAnimation(timing.dt);

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
			glm::vec3 forward{ glm::normalize(entity_pos - cam_pos) };
			glm::vec3 up{ 0.f, 1.f, 0.f };
			float near_plane = cam->get().near_plane;
			float far_plane = cam->get().far_plane;
			float width_ratio = cam->get().width_ratio;
			float height_ratio = cam->get().height_ratio;

			game_camera = std::make_unique<Camera>(cam_pos, forward, up, GraphicsSettings::get().fov, near_plane, far_plane, width_ratio, height_ratio);

			break;
		}



	}

	void Scene::onEvent(Event::Event& e) {}

	entt::entity Scene::AddObject(const std::shared_ptr<Assets::Model>& mdl,  const std::string& name, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale, Assets::GUID const& diff_id, Assets::GUID const& ao_id)
	{
		auto ecs = services->get<ECS::Controller>();
		auto meta = services->get<MetaData::Service>();

		// if animated object, may be in T pose. must find root xform
		glm::vec3 root_scale = glm::vec3(1.f);
		glm::quat root_rot= glm::quat(1.f, 0.f, 0.f, 0.f);
		glm::vec3 root_trans = glm::vec3(0.f);

		PN_CORE_INFO("Model {} has {} animations", mdl->vpath, mdl->animations.size());
		if (mdl->animations.size()) {

			auto anim = mdl->animations[0];
			for (const auto& [bone_name, track] : anim.track_map) {
				const auto it = std::find_if(mdl->skeleton.begin(), mdl->skeleton.end(), [&bone_name](const Assets::Bone& b) { return bone_name == b.name; });
				if (it == mdl->skeleton.end()) {
					root_scale = track[0].scale;
					root_rot = track[0].rotation;
					root_trans = track[0].translation;
					break;
				}
			}
		}

		// i suppose i shouldnt bake the root xform into LocalTransform here. so
		//glm::mat4 root_xform = 
		//	glm::translate(glm::mat4(1.f), root_trans) * 
		//	glm::mat4_cast(root_rot) *
		//	glm::scale(glm::mat4(1.f), root_scale);



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

	Camera* Scene::GetActiveCamera()
	{
		return active_camera;
	}

	void Scene::SetActiveCamera(Camera* cam) {
		active_camera = cam;
	}

	void Scene::SetEditorCamera() {
		SetActiveCamera(editor_camera.get());

		
	}
	void Scene::SetGameCamera()
	{
		SetActiveCamera(game_camera.get());

	}
}
