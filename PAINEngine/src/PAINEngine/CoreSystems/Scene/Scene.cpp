#include "Scene.h"
#include "CoreSystems/Path/Path.h"
#include "ECS/Controller.h"
#include "ECS/Components/cMetaData.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cMeshRenderer.h"

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
		auto ogre_mesh = Mesh::LoadObj(obj_path);
		auto cube_mesh = Mesh::LoadObj();

		// Create the audio source object and store its entity ID
		audioSourceEntity = AddObject(cube_mesh, "audio_src", { 0.f, 1.f, 0.f }, glm::quat(), { 1.f, 1.f, 1.f });

		// Create the other static objects
		AddObject(ogre_mesh, "ogre_1", { 0.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		AddObject(ogre_mesh, "ogre_2", { 2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });
		AddObject(ogre_mesh, "ogre_3", { -2.f, 1.f, 0.f }, { 0.f,0.f,0.f, 0.f }, { 1.f, 1.f, 1.f });

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
		if (!audioManager || audioSourceEntity == ECS::Entity::INVALID) return;

		// Handle audio pause/resume based on simulation state
		static bool wasPaused = false;
		if (isPaused != wasPaused) {
			if (isPaused) {
				// Pause the looping music channel
				if (isValid(audioSourceChannel)) {
					audioManager->pauseChannel(audioSourceChannel);
				}
			}
			else {
				// Resume the looping music channel
				if (isValid(audioSourceChannel)) {
					audioManager->resumeChannel(audioSourceChannel);
				}
			}
			wasPaused = isPaused;
		}

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
		auto transformTypeIdOpt = ecs->getComponentType<Transform>();
		if (transformTypeIdOpt)
		{
			// Get a copy of the component
			auto component_as_void = ecs->getCopiedEntityComponent(audioSourceEntity, *transformTypeIdOpt);
			if (component_as_void)
			{
				// Cast the copy to the correct type and modify it
				auto transformComp = std::static_pointer_cast<Transform>(component_as_void);
				transformComp->position = currentPosition;

				// Set the modified copy back into the ECS
				ecs->setEntityComponent(audioSourceEntity, *transformTypeIdOpt, transformComp);
			}
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

	ECS::Entity::Type Scene::AddObject(std::shared_ptr<Mesh> mesh, std::string name, glm::vec3 pos, glm::quat rot, glm::vec3 scale)
	{
		auto ecs = services->get<ECS::Controller>();
		ECS::Entity::Type entity = ecs->createEntity();
		ecs->addEntityComponent(entity, MetaData::EntityName{ name });
		ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
		ecs->addEntityComponent(entity, MeshRenderer{ mesh });

		return entity;
	}

	Camera* Scene::GetActiveCamera()
	{
		return camera.get();
	}
}
