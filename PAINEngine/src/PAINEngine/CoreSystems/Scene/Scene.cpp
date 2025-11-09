#include "Scene.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Assets/sAssets.h"
#include "ECS/Controller.h"
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

namespace {
	static uint32_t djb2_hash(const std::string& str) {
		uint32_t hash = 5381;
		for (char c : str)
			hash = ((hash << 5) + hash) + (uint8_t)c; /* hash * 33 + c */
		return hash;
	}
}

namespace PAIN {
	void Scene::onDetach() {}

	void Scene::onAttach()
	{

		// For audio comp testing
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

		// Init light sources
		LightSources::get().create("cam");
		auto olcam = LightSources::get().get("cam");
		Light& lcam = olcam.value();
		lcam.L_intensity = glm::vec3(0.01f);
		//lcam.setShadowType(Light::SHADOW_TYPES::MAPPED);

		//GraphicsSettings::get().daytime = false;
		//GraphicsSettings::get().ibl = false;

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

		LightSources::get().create("a");
		auto ola = LightSources::get().get("a");
		Light& la = ola.value();
		la.position = glm::vec3(10.f, 1.f, -8.f);
		la.L_intensity = glm::vec3(2.f);

		//LightSources::get().create("b");
		//auto olb = LightSources::get().get("b");
		//Light& lb = olb.value();
		//lb.position = glm::vec3(-4.f, 4.f, -8.f);
		//lb.L_intensity = glm::vec3(0.2f);

		// Demo Object and Audio Setup
		auto audioManager = services->get<Audio::Audio>();
		auto pathService = services->get<Path::Path>();

		auto ogre_diffuse_tex = services->get<Assets::Manager>()->getAsset<Assets::Texture>(Assets::GUID("5923aab8-5293-f945-958e-496acd0218c3"));
		auto ogre_smile_ao_map = services->get<Assets::Manager>()->getAsset<Assets::Texture>(Assets::GUID("cee03212-928a-6347-9d55-07fe46ac3ea1"));


		// for .mesh(converted from .obj only)
		std::shared_ptr<Assets::Model> mdl;
		{
			mdl = cacheModel("game_assets://models/ogre_smile.mesh");

			mdl->materials[0].gl_diffuse_tex = ogre_diffuse_tex->gl_texture;
			mdl->materials[0].gl_ao_tex = ogre_smile_ao_map->gl_texture;
			mdl->materials[0].metallic = 0.f;
			mdl->materials[0].roughness = 1.f;
			mdl->materials[0].baseColor = { 1, 0, 1 };

			// logging to check data
			{
				PN_CORE_TRACE("File: {}\nVertices: {}\nIndices: {}\nMaterials: {}", mdl->vpath, mdl->vertices.size(), mdl->indices.size(), mdl->materials.size());
				PN_CORE_TRACE("Base roughness: {}\nBase metallic: {}\nBase color: {},{},{}", mdl->materials[0].roughness, mdl->materials[0].metallic, mdl->materials[0].baseColor.r, mdl->materials[0].baseColor.g, mdl->materials[0].baseColor.b);
			}

			AddObject(mdl, "ogre_smile", { 0.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		}

		mdl = cacheModel("game_assets://models/ogre.mesh");
		mdl->materials[0].gl_diffuse_tex = ogre_diffuse_tex->gl_texture;
		mdl->materials[0].gl_ao_tex = ogre_smile_ao_map->gl_texture;
		mdl->materials[0].metallic = 0.f;
		mdl->materials[0].roughness = 1.f;
		mdl->materials[0].baseColor = { 1, 0, 1 };
		AddObject(mdl, "ogre_left", { -2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });

		mdl = getModel(djb2_hash("game_assets://models/ogre.mesh"));
		mdl->materials[0].gl_diffuse_tex = ogre_diffuse_tex->gl_texture;
		mdl->materials[0].gl_ao_tex = ogre_smile_ao_map->gl_texture;
		mdl->materials[0].metallic = 0.f;
		mdl->materials[0].roughness = 1.f;
		mdl->materials[0].baseColor = { 1, 0, 1 };
		AddObject(mdl, "ogre_right", { 2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });

		auto sdcc_tex = services->get<Assets::Manager>()->getAsset<Assets::Texture>(Assets::GUID("71051859-f5ee-144a-b1e5-59ad02d13695"));
		mdl = cacheModel("game_assets://models/sdcc.mesh");
		mdl->materials[0].gl_diffuse_tex = sdcc_tex->gl_texture;
		mdl->materials[0].metallic = 1.f;
		mdl->materials[0].roughness = 0.f;
		mdl->materials[0].baseColor = { 1, 0, 1 };
		AddObject(mdl, "sdcc", {5.f, 0.f, -3.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f)), {3.f, 3.f, 3.f});

		auto city_tex = services->get<Assets::Manager>()->getAsset<Assets::Texture>(Assets::GUID("29fe999b-d257-bf41-879d-6d7578d43734"));
		mdl = cacheModel("game_assets://models/city.mesh");
		mdl->materials[0].gl_diffuse_tex = city_tex->gl_texture;
		mdl->materials[0].metallic = 0.f;
		mdl->materials[0].roughness = 1.f;
		mdl->materials[0].baseColor = { 0, 1, 0 };
		AddObject(mdl, "city", { -8.f, 0.f, -5.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f)), { 3.f, 3.f, 3.f });

		mdl = cacheModel("game_assets://models/CrumpledDevelopable.mesh");
		mdl->materials[0].metallic = 0.f;
		mdl->materials[0].roughness = 1.f;
		mdl->materials[0].baseColor = { 0.3f, 0.3f, 0.3f };
		AddObject(mdl, "cd", { 5.f, 1.f, 10.f }, glm::angleAxis(glm::radians(-90.f), glm::vec3(0.0f, 1.0f, 0.0f)), { 3.f, 3.f, 3.f });



		// gltf testing
//#define GLTF_TEST
#ifdef GLTF_TEST
		{
			mdl = cacheModel("game_assets://models/930/930.mesh");
			AddObject(mdl, "930", { 0.f, 1.f, -5.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		}
#endif

		/*
		{
			auto obj_path = services->get<Path::Path>()->resolvePath("game_assets://models/ogre_smile.obj");
			auto smile_ogre_mesh_id = cacheMesh(obj_path);
			auto smile_ogre_mesh = getMesh(smile_ogre_mesh_id);

			smile_ogre_mesh->texture_id = ogre_diffuse_tex->gl_texture;
			smile_ogre_mesh->material.tex = smile_ogre_mesh->texture_id;
			smile_ogre_mesh->material.useTex = true;
			smile_ogre_mesh->material.aoTex = ogre_smile_ao_map->gl_texture;
			smile_ogre_mesh->material.useAo = true;
			smile_ogre_mesh->material.metal = 0.f;
			smile_ogre_mesh->material.rough = 1.f;

			//AddObject(smile_ogre_mesh_id, "ogre_1", { 0.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		}
		*/

		// New audio demo test
		// Add the looping sound to the "screen" entity as a component
#ifdef PN_PLATFORM_WINDOWS
		auto loopingSoundPath = pathService->resolvePath("game_assets://Audio/Music/Boss_Music.wav");
#else
		auto loopingSoundPath = ("file:///android_asset/game/audio/music/Boss_Music.ogg");
#endif



		if (audioManager)
		{
			// Define the rectangular path
			float pathWidth = 16.0f;
			float pathDepth = 8.0f;
			glm::vec3 pathCenter = { 0.0f, 1.0f, 0.0f };
			m_pathCorners = {
				pathCenter + glm::vec3(-pathWidth / 2, 0.0f, -pathDepth / 2),
				pathCenter + glm::vec3(pathWidth / 2, 0.0f, -pathDepth / 2),
				pathCenter + glm::vec3(pathWidth / 2, 0.0f,  pathDepth / 2),
				pathCenter + glm::vec3(-pathWidth / 2, 0.0f,  pathDepth / 2)
			};

			// Load and play the looping music
			//std::string loopingSoundPath = pathService->resolvePath("game_assets://Audio/Music/Boss_Music.wav");
			//audioManager->loadSound(loopingSoundPath, true, true, false, 1.0f, 20.0f);
			//auto channelOpt = audioManager->play(loopingSoundPath, pathCorners[0], 0.0f);
			//if (channelOpt.has_value()) {
			//	audioSourceChannel = channelOpt.value();
			//}

			// Load the footstep playlist
			Audio::PlaylistDesc footstepPlaylist;
			footstepPlaylist.name = "FootstepsGrass";
			for (int i = 1; i <= 8; ++i)
			{


				//#ifdef PN_PLATFORM_WINDOWS
				//std::string footstepFile = "Footstep_Grass_0" + std::to_string(i) + ".wav";
				//std::string footstepPath = pathService->resolvePath("game_assets://Audio/SFX/MovingSFX/" + footstepFile);
				//#else
				//std::string footstepFile = "Footstep_Grass_0" + std::to_string(i) + ".ogg";
				//std::string footstepPath = ("file:///android_asset/game/audio/sfx/movingsfx/" + footstepFile);
				//#endif
				//audioManager->loadSound(footstepPath, true, false, false, 1.0f, 15.0f);
				//footstepPlaylist.paths.push_back(footstepPath);
			}
			audioManager->loadPlaylist(footstepPlaylist);
		}

		// font
		TextRenderer::get();

		// skybox
		Skybox::get().init(services, services->get<Path::Path>()->resolvePath("engine_assets://textures/skybox2.hdr"));

		// Test load prefab
		//std::vector<entt::entity> loaded_entities = services->get<Serialization::Service>()->loadPrefabFromFile("sdcc.prefab");
		//for (auto e : loaded_entities) {
		//	// Info: Print entity names, transforms, etc.
		//	auto nameOpt = services->get<ECS::Controller>()->getEntityComponent<MetaData::EntityName>(e);
		//	std::string name = nameOpt ? nameOpt->get().name : "<no name>";
		//}

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

		//// Apply time scale to deltaTime for simulation
		//float scaledDt = timing.dt * timeScale;

		if (GraphicsSettings::get().daytime) {
			auto olc = LightSources::get().get("world");
			Light& lc = olc.value();
			lc.position = GetActiveCamera()->pos - glm::normalize(lc.forward) * lc.shadow_source_follow_distance;
		}

		//auto ecs = services->get<ECS::Controller>();
		//auto audioManager = services->get<Audio::Audio>();
		//if (!audioManager || m_audioSourceEntity == entt::null) return;

		//// The AudioSystem now handles pause/resume automatically via the AppSystem interface

		//// Animate the audio source object along a predefined path (respects pause)
		//m_demoTime += scaledDt; // Changed from timing.dt
		//float progress = fmod(m_demoTime, m_segmentDuration) / m_segmentDuration;
		//int segment = static_cast<int>(m_demoTime / m_segmentDuration) % 4;
		//if (segment != m_currentPathSegment) {
		//	m_currentPathSegment = segment;
		//}
		//glm::vec3 startPos = m_pathCorners[m_currentPathSegment];
		//glm::vec3 endPos = m_pathCorners[(m_currentPathSegment + 1) % 4];
		//glm::vec3 currentPosition = glm::mix(startPos, endPos, progress);

		//// Update the Transform component in the ECS for the renderer
		//if (auto transform = ecs->getEntityComponent<Transform>(m_audioSourceEntity)) {
		//	transform->get().position = currentPosition;
		//}

		//// Handle footstep playback at intervals (respects pause)
		//m_footstepTimer -= scaledDt; // Changed from timing.dt
		//if (m_footstepTimer <= 0.0f)
		//{
		//	// This is a "one-shot" sound, not tied to a component.
		//	// It's fine to keep calling the service directly for this.
		//	audioManager->playRandom("FootstepsGrass", currentPosition, 0.0f);
		//	m_footstepTimer = m_footstepInterval;
		//}
	}

	void Scene::onEvent(Event::Event& e) {}

	entt::entity Scene::AddObject(const std::shared_ptr<Assets::Model>& mdl, const std::string& name, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale)
	{
		auto ecs = services->get<ECS::Controller>();
		entt::entity entity = ecs->createEntity();
		ecs->addEntityComponent(entity, MetaData::EntityName{ name });
		ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
		ecs->addEntityComponent(entity, ModelRenderer{ djb2_hash(mdl->vpath) });

		return entity;
	}

	Camera* Scene::GetActiveCamera()
	{
		return camera.get();
	}

	std::shared_ptr<Mesh> Scene::loadMesh(const std::string& path_to_mesh)
	{
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		bool file_ok = false;

#ifdef PN_PLATFORM_ANDROID
		PN_CORE_INFO("Using Android asset manager for mesh");
		std::string mesh_data = ReadFileAndroid(path_to_mesh);
		if (mesh_data.empty()) {
			PN_CORE_ERROR("Failed to read mesh data from Android assets: {0}", path_to_mesh);
		}
		else {
			PN_CORE_INFO("Successfully read mesh data from Android assets: {0}", path_to_mesh);
			PN_CORE_INFO("Mesh data size: {0} bytes", mesh_data.size());
			file_ok = true;
		}
#endif



#ifdef PN_PLATFORM_WINDOWS

		std::filesystem::path mesh_full = path_to_mesh;

		file_ok = std::filesystem::exists(path_to_mesh) && path_to_mesh != "";
#endif

		if (!file_ok)
		{
			PN_CORE_ERROR("Mesh file not found: {}, loading default mesh", path_to_mesh == "" ? "No mesh file given" : path_to_mesh);
			vertices = {
				// Front (+Z)
				{{-0.5f, -0.5f,  0.5f}, {0,0,1}},
				{{ 0.5f, -0.5f,  0.5f}, {0,0,1}},
				{{ 0.5f,  0.5f,  0.5f}, {0,0,1}},
				{{-0.5f,  0.5f,  0.5f}, {0,0,1}},

				// Back (-Z)
				{{ 0.5f, -0.5f, -0.5f}, {0,0,-1}},
				{{-0.5f, -0.5f, -0.5f}, {0,0,-1}},
				{{-0.5f,  0.5f, -0.5f}, {0,0,-1}},
				{{ 0.5f,  0.5f, -0.5f}, {0,0,-1}},

				// Left (-X)
				{{-0.5f, -0.5f, -0.5f}, {-1,0,0}},
				{{-0.5f, -0.5f,  0.5f}, {-1,0,0}},
				{{-0.5f,  0.5f,  0.5f}, {-1,0,0}},
				{{-0.5f,  0.5f, -0.5f}, {-1,0,0}},

				// Right (+X)
				{{ 0.5f, -0.5f,  0.5f}, {1,0,0}},
				{{ 0.5f, -0.5f, -0.5f}, {1,0,0}},
				{{ 0.5f,  0.5f, -0.5f}, {1,0,0}},
				{{ 0.5f,  0.5f,  0.5f}, {1,0,0}},

				// Top (+Y)
				{{-0.5f,  0.5f,  0.5f}, {0,1,0}},
				{{ 0.5f,  0.5f,  0.5f}, {0,1,0}},
				{{ 0.5f,  0.5f, -0.5f}, {0,1,0}},
				{{-0.5f,  0.5f, -0.5f}, {0,1,0}},

				// Bottom (-Y)
				{{-0.5f, -0.5f, -0.5f}, {0,-1,0}},
				{{ 0.5f, -0.5f, -0.5f}, {0,-1,0}},
				{{ 0.5f, -0.5f,  0.5f}, {0,-1,0}},
				{{-0.5f, -0.5f,  0.5f}, {0,-1,0}}
			};

			indices = {
				// Front (+Z)
				0,1,2, 0,2,3,
				// Back (-Z)
				4,5,6, 4,6,7,
				// Left (-X)
				8,9,10, 8,10,11,
				// Right (+X)
				12,13,14, 12,14,15,
				// Top (+Y)
				16,17,18, 16,18,19,
				// Bottom (-Y)
				20,21,22, 20,22,23
			};

			return std::make_shared<Mesh>(vertices, indices, path_to_mesh);
		}

		struct TempVertex {
			int pIdx = -1, nIdx = -1, tIdx = -1;  //
			TempVertex() = default;
			TempVertex(const std::string& token) {
				// Parse formats: v/vt/vn or v//vn or v/vt or v
				if (token.find("//") != std::string::npos) {
					// Format: v//vn (no texture coords)
					sscanf(token.c_str(), "%d//%d", &pIdx, &nIdx);
				}
				else {
					// Format: v/vt/vn or v/vt or v
					int parsed = sscanf(token.c_str(), "%d/%d/%d", &pIdx, &tIdx, &nIdx);
					if (parsed == 2) {
						// Only got v/vt, move tIdx value to nIdx (some files use v/n format)
						nIdx = tIdx;
						tIdx = -1;
					}
				}
			}
		};

		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> texCoords;

#ifdef PN_PLATFORM_WINDOWS
		std::ifstream objStream(mesh_full);
		if (!objStream) {
			PN_CORE_ERROR("Could not open {}", mesh_full.string());
			assert(false);
		}
#else
		std::istringstream objStream(mesh_data);
#endif

		std::string line;
		while (std::getline(objStream, line)) {
			if (line.empty() || line[0] == '#') continue;

			std::istringstream ls(line);
			std::string token;
			ls >> token;

			if (token == "v") {
				glm::vec3 p;
				ls >> p.x >> p.y >> p.z;
				positions.push_back(p);
			}
			else if (token == "vt") {
				// Process texture coordinate
				float s, t;
				ls >> s >> t;
				texCoords.push_back(glm::vec2(s, t));
			}
			else if (token == "vn") {
				glm::vec3 n;
				ls >> n.x >> n.y >> n.z;
				normals.push_back(n);
			}
			else if (token == "f") {
				std::vector<TempVertex> faceVerts;
				std::string vStr;
				while (ls >> vStr) faceVerts.emplace_back(vStr);

				// Fan triangulation
				for (size_t i = 1; i + 1 < faceVerts.size(); i++) {
					TempVertex tv[3] = { faceVerts[0], faceVerts[i], faceVerts[i + 1] };
					for (int j = 0; j < 3; j++) {
						Vertex v{};
						// Check bounds for positions
						if (tv[j].pIdx > 0 && tv[j].pIdx - 1 < positions.size()) {
							v.pos = positions[tv[j].pIdx - 1];
						}

						// Check bounds for normals
						if (tv[j].nIdx > 0 && tv[j].nIdx - 1 < normals.size()) {
							v.normal = normals[tv[j].nIdx - 1];
						}

						// Check bounds for texture coordinates
						if (tv[j].tIdx > 0 && tv[j].tIdx - 1 < texCoords.size()) {
							v.uv = texCoords[tv[j].tIdx - 1];
						}

						vertices.push_back(v);
						indices.push_back((unsigned int)vertices.size() - 1);
					}
				}
			}
		}

		// can add deduplication
		// can add normal fallback
		// can add tangents (optional)
		// can add generalization
		// must add texcoords

		return std::make_shared<Mesh>(vertices, indices, path_to_mesh);
	}

	std::shared_ptr<Assets::Model> Scene::cacheModel(const std::string& vpath) {
		std::filesystem::path fsPath(vpath);
		std::string filename = fsPath.filename().string();

		PN_CORE_INFO("Loading .mesh: {}", filename);

		// check that model has .mesh extension
		{
			static constexpr char sep = '.';
			auto sep_idx = filename.find(sep);
			std::string ext{};
			if (sep_idx != filename.npos) {
				ext = filename.substr(sep_idx + 1);
			}

			if (ext != "mesh") {
				PN_CORE_ERROR("Invalid model format! Expected: .mesh, Given: {}", vpath);
				throw std::exception();
			}
		}

		std::shared_ptr<Assets::Manager> am = services->get<Assets::Manager>();
		Assets::Loader* ral = am->getRawAssetLoader();

		PN_CORE_TRACE("getRawAssetLoader address: {}", static_cast<const void*>(ral));

		auto loader = ral->GetLoader(Assets::Type::Model);
		const std::shared_ptr<Assets::IAsset> base_mdl = loader(vpath);
		std::shared_ptr<Assets::Model> mdl = std::dynamic_pointer_cast<Assets::Model>(base_mdl);

		// logging to check data
		{
			PN_CORE_TRACE("File: {}\nVertices: {}\nIndices: {}\nMaterials: {}", filename, mdl->vertices.size(), mdl->indices.size(), mdl->materials.size());
			PN_CORE_TRACE("Base roughness: {}\nBase metallic: {}\nBase color: {},{},{}", mdl->materials[0].roughness, mdl->materials[0].metallic, mdl->materials[0].baseColor.r, mdl->materials[0].baseColor.g, mdl->materials[0].baseColor.b);
		}

		modelCache[djb2_hash(vpath)] = mdl;

		PN_CORE_INFO("{} loaded", filename);

		return mdl;
	}

	uint32_t Scene::cacheMesh(const std::string& path)
	{
		std::filesystem::path fsPath(path);
		std::string filename = fsPath.filename().string();

		auto mesh = loadMesh(path);
		uint32_t mesh_id = djb2_hash(filename);
		meshCache[mesh_id] = mesh;

		return mesh_id;
	}

	uint32_t Scene::getModelId(const std::string& file_name)
	{
		std::filesystem::path fsPath(file_name);
		std::string filename = fsPath.filename().string();

		uint32_t mesh_id = djb2_hash(filename);
		return mesh_id;
	}

	std::shared_ptr<Assets::Model> Scene::getModel(uint32_t mesh_id)
	{
		auto it = modelCache.find(mesh_id);
		if (it != modelCache.end()) {
			return it->second;
		}

		PN_CORE_ERROR("UNABLE TO FIND MESH");
		return nullptr;
	}

	std::shared_ptr<Mesh> Scene::getMesh(uint32_t mesh_id)
	{
		auto it = meshCache.find(mesh_id);
		if (it != meshCache.end()) {
			return it->second;
		}

		PN_CORE_ERROR("UNABLE TO FIND MESH");
		return nullptr;
	}
}
