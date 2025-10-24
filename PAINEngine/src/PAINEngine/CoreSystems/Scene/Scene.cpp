#include "Scene.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Assets/sAssets.h"
#include "ECS/Controller.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cMeshRenderer.h"
#include "CoreSystems/Renderer/texture.h"

#ifdef _DEBUG
#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#endif

namespace PAIN {
	void Scene::onDetach() {}

	void Scene::onAttach()
	{

		// Camera and Scene Setup
		glm::vec3 pos{ 0.f, 2.f, 4.f };
		glm::vec3 forward{ -glm::normalize(pos) };
		glm::vec3 up{ 0.f, 1.f, 0.f };
		float fov{ 90.f };
		float near_plane{ 0.1f };
		float far_plane{ 100.f };
		float width_ratio{ 16.f };
		float height_ratio{ 9.f };
		camera = std::make_unique<Camera>(pos, forward, up, fov, near_plane, far_plane, width_ratio, height_ratio);

		// Demo Object and Audio Setup
		auto audioManager = services->get<Audio::Audio>();
		auto pathService = services->get<Path::Path>();

		auto obj_path = services->get<Path::Path>()->resolvePath("game_assets://Models/ogre.obj");

		cacheMesh("");
		cacheMesh(obj_path);

		auto quad_path = services->get<Path::Path>()->resolvePath("engine_assets://Models/quad.obj");
		cacheMesh(quad_path);

		auto cube_mesh = getMeshId("");
		auto ogre_mesh = getMeshId("ogre.obj");
		auto quad_mesh_id = getMeshId("quad.obj");

		auto quad_mesh = getMesh(quad_mesh_id);
		auto texture_path = services->get<Path::Path>()->resolvePath("engine_assets://Textures/sunshine.png");
		quad_mesh->texture_id = TextureManager::get().load(texture_path.c_str(), "sunshine");
		
		// Create the audio source object and store its entity ID
		audioSourceEntity = AddObject(cube_mesh, "audio_src", { 0.f, 1.f, 0.f }, glm::quat(), { 1.f, 1.f, 1.f });

		Material texturedMat;
		texturedMat.useTex = true;
		texturedMat.tex = quad_mesh->texture_id;
		texturedMat.color = { 1.f, 0.f, 1.f };

		quad_mesh->material = texturedMat;

		// Create the other static objects
		AddObject(ogre_mesh, "ogre_1", { 0.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		AddObject(ogre_mesh, "ogre_2", { 2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		AddObject(ogre_mesh, "ogre_3", { -2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		AddObject(quad_mesh_id, "screen", { 0.f, 3.f, 0.f }, { 0.f, 0.f, 0.f, 0.f }, { 1.f, 1.f, 1.f });

		if (audioManager)
		{
			// Define the rectangular path
			float pathWidth = 16.0f;
			float pathDepth = 8.0f;
			glm::vec3 pathCenter = { 0.0f, 1.0f, 0.0f };
			pathCorners = {
				pathCenter + glm::vec3(-pathWidth / 2, 0.0f, -pathDepth / 2),
				pathCenter + glm::vec3(pathWidth / 2, 0.0f, -pathDepth / 2),
				pathCenter + glm::vec3(pathWidth / 2, 0.0f,  pathDepth / 2),
				pathCenter + glm::vec3(-pathWidth / 2, 0.0f,  pathDepth / 2)
			};

			// Load and play the looping music
			std::string loopingSoundPath = pathService->resolvePath("game_assets://Audio/Music/Boss_Music.wav");
			audioManager->loadSound(loopingSoundPath, true, true, false, 1.0f, 20.0f);
			auto channelOpt = audioManager->play(loopingSoundPath, pathCorners[0], 0.0f);
			if (channelOpt.has_value()) {
				audioSourceChannel = channelOpt.value();
			}

			// Load the footstep playlist
			Audio::PlaylistDesc footstepPlaylist;
			footstepPlaylist.name = "FootstepsGrass";
			for (int i = 1; i <= 8; ++i)
			{
				std::string footstepFile = "Footstep_Grass_0" + std::to_string(i) + ".wav";
				std::string footstepPath = pathService->resolvePath("game_assets://Audio/SFX/MovingSFX/" + footstepFile);
				audioManager->loadSound(footstepPath, true, false, false, 1.0f, 15.0f);
				footstepPlaylist.paths.push_back(footstepPath);
			}
			audioManager->loadPlaylist(footstepPlaylist);
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

		// Apply time scale to deltaTime for simulation
		float scaledDt = timing.dt * timeScale;

		auto ecs = services->get<ECS::Controller>();
		auto audioManager = services->get<Audio::Audio>();
		if (!audioManager || audioSourceEntity == entt::null) return;

		// Handle audio pause/resume based on simulation state
		//static bool wasPaused = false;
		//if (isPaused != wasPaused) {
		//	if (isPaused) {
		//		// Pause the looping music channel
		//		if (isValid(audioSourceChannel)) {
		//			audioManager->pauseChannel(audioSourceChannel);
		//		}
		//	}
		//	else {
		//		// Resume the looping music channel
		//		if (isValid(audioSourceChannel)) {
		//			audioManager->resumeChannel(audioSourceChannel);
		//		}
		//	}
		//	wasPaused = isPaused;
		//}

		// Update listener position to match the camera's current state (always runs)
		audioManager->setListener(camera->pos, { 0,0,0 }, camera->forward, camera->up);

		// Animate the audio source object along a predefined path (respects pause)
		demoTime += scaledDt; // Changed from timing.dt
		float progress = fmod(demoTime, segmentDuration) / segmentDuration;
		int segment = static_cast<int>(demoTime / segmentDuration) % 4;
		if (segment != currentPathSegment) {
			currentPathSegment = segment;
		}
		glm::vec3 startPos = pathCorners[currentPathSegment];
		glm::vec3 endPos = pathCorners[(currentPathSegment + 1) % 4];
		glm::vec3 currentPosition = glm::mix(startPos, endPos, progress);

		// Update the Transform component in the ECS for the renderer
		if (auto transform = ecs->getEntityComponent<Transform>(audioSourceEntity)) {
			transform->get().position = currentPosition;
		}
		else {
			// Component doesn't exist - log warning
			PN_CORE_WARN("Audio source entity {} has no Transform component",
				static_cast<uint32_t>(audioSourceEntity));
		}

		// Update the 3D position of the looping music channel (only when not paused)
		if (!isPaused && isValid(audioSourceChannel)) {
			audioManager->setPosition(audioSourceChannel, currentPosition);
		}

		// Handle footstep playback at intervals (respects pause)
		footstepTimer -= scaledDt; // Changed from timing.dt
		if (footstepTimer <= 0.0f)
		{
			audioManager->playRandom("FootstepsGrass", currentPosition, 0.0f);
			footstepTimer = footstepInterval;
		}
	}

	void Scene::onEvent(Event::Event& e) {}

	entt::entity Scene::AddObject(uint32_t mesh, const std::string& name, const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scale)
	{
		auto ecs = services->get<ECS::Controller>();
		entt::entity entity = ecs->createEntity();
		ecs->addEntityComponent(entity, MetaData::EntityName{ name });
		ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
		ecs->addEntityComponent(entity, MeshRenderer{ mesh });

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

			return std::make_shared<Mesh>(vertices, indices);;
		}

		struct TempVertex {
			int pIdx = -1, nIdx = -1;
			TempVertex() = default;
			TempVertex(const std::string& token) {
				// Parse formats: v//n or v/n
				if (token.find("//") != std::string::npos) {
					sscanf(token.c_str(), "%d//%d", &pIdx, &nIdx);
				}
				else {
					sscanf(token.c_str(), "%d/%d", &pIdx, &nIdx);
				}
			}
		};

		std::vector<glm::vec3> positions;
		std::vector<glm::vec3> normals;

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
			//else if (token == "vt") {
			//	// Process texture coordinate
			//	float s, t;
			//	ls >> s >> t;
			//	texCoords.push_back(glm::vec2(s, t));
			//}
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
						if (tv[j].pIdx > 0) v.pos = positions[tv[j].pIdx - 1];
						if (tv[j].nIdx > 0) v.normal = normals[tv[j].nIdx - 1];

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

		return std::make_shared<Mesh>(vertices, indices);;
	}

	uint32_t Scene::cacheMesh(const std::string& path)
	{
		std::filesystem::path fsPath(path);
		std::string filename = fsPath.filename().string();

		auto mesh = loadMesh(path);
		uint32_t mesh_id = std::hash<std::string>{}(filename);
		meshCache[mesh_id] = mesh;

		return mesh_id;
	}

	uint32_t Scene::getMeshId(const std::string& file_name)
	{
		std::filesystem::path fsPath(file_name);
		std::string filename = fsPath.filename().string();

		uint32_t mesh_id = std::hash<std::string>{}(filename);
		return mesh_id;
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
