#include "Scene.h"
#include "CoreSystems/Path/Path.h"
#include "ECS/Controller.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cMeshRenderer.h"

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

		// 3D objects to render, designate the first one as audio source
		// Note: Assuming Mesh::LoadObj now correctly returns a std::shared_ptr<Mesh> to match AddObject signature
		AddObject(Mesh::LoadObj("ogre.obj"), { 0.f, 1.f, 0.f }, glm::quat(), { 1.f, 1.f, 1.f });
		audioSourceObjectIndex = 0; // The first object is our audio source
		AddObject(Mesh::LoadObj("ogre.obj"), { 2.f, 1.f, 0.f }, glm::quat(), { 1.f, 1.f, 1.f });
		AddObject(Mesh::LoadObj("ogre.obj"), { -2.f, 1.f, 0.f }, glm::quat(), { 1.f, 1.f, 1.f });

		// --- Audio Demo Initialization ---
		auto audioManager = services->get<Audio::Audio>();
		if (audioManager)
		{
			// Define the rectangular path centered at the world origin
			float pathWidth = 16.0f;
			float pathDepth = 8.0f;
			glm::vec3 pathCenter = { 0.0f, 1.0f, 0.0f };

			pathCorners = {
				pathCenter + glm::vec3(-pathWidth / 2, 0.0f, -pathDepth / 2), // North-West
				pathCenter + glm::vec3(pathWidth / 2, 0.0f, -pathDepth / 2), // North-East
				pathCenter + glm::vec3(pathWidth / 2, 0.0f,  pathDepth / 2), // South-East
				pathCenter + glm::vec3(-pathWidth / 2, 0.0f,  pathDepth / 2)  // South-West
			};

			// Position the audio source object at its starting corner
			if (audioSourceObjectIndex != -1) {
				m_Objects[audioSourceObjectIndex].transform[3] = glm::vec4(pathCorners[0], 1.0f);
			}

			// 1. Load and play the looping music
			std::string loopingSoundPath = services->get<Path::Path>()->resolvePath("game_assets://Audio/Music/Boss_Music.wav");
			audioManager->loadSound(loopingSoundPath, true, true, false, 1.0f, 20.0f);
			auto channelOpt = audioManager->play(loopingSoundPath, pathCorners[0], 0.0f);
			if (channelOpt.has_value()) {
				audioSourceChannel = channelOpt.value();
			}

			// 2. Load the footstep playlist
			Audio::PlaylistDesc footstepPlaylist;
			footstepPlaylist.name = "FootstepsGrass";
			for (int i = 1; i <= 8; ++i)
			{
				std::string footstepFile = "Footstep_Grass_0" + std::to_string(i) + ".wav";
				std::string footstepPath = services->get<Path::Path>()->resolvePath("game_assets://Audio/SFX/MovingSFX/" + footstepFile);
				// Pre-load sounds with 3D attenuation settings
				audioManager->loadSound(footstepPath, true, false, false, 1.0f, 15.0f);
				footstepPlaylist.paths.push_back(footstepPath);
			}
			audioManager->loadPlaylist(footstepPlaylist);
		}
	}

	void Scene::onUpdate(AppTiming timing)
	{
		auto audioManager = services->get<Audio::Audio>();
		if (!audioManager || audioSourceObjectIndex == -1) return;

		// --- Update Listener Position ---
		// Update FMOD listener to match the camera's current state
		glm::vec3 camPos = camera->pos;
		glm::vec3 camVel = { 0, 0, 0 }; // Velocity is zero for a stationary listener
		glm::vec3 camFwd = camera->forward;
		glm::vec3 camUp = camera->up;
		audioManager->setListener(camPos, camVel, camFwd, camUp);

		// --- Animate Audio Source Object ---
		demoTime += timing.dt;
		float progress = fmod(demoTime, segmentDuration) / segmentDuration;

		// Determine which segment of the path we are on
		int segment = static_cast<int>(demoTime / segmentDuration) % 4;
		if (segment != currentPathSegment) {
			currentPathSegment = segment;
		}

		// Interpolate between the current segment's start and end corners
		glm::vec3 startPos = pathCorners[currentPathSegment];
		glm::vec3 endPos = pathCorners[(currentPathSegment + 1) % 4];
		glm::vec3 currentPosition = glm::mix(startPos, endPos, progress);

		// Update the legacy transform for systems that still use it (like our audio)
		m_Objects[audioSourceObjectIndex].transform[3] = glm::vec4(currentPosition, 1.0f);

		// This block updates the Transform component in the ECS for the renderer.
		// It uses the get-copy-modify-set pattern required by the new ECS API.
		auto ecs = services->get<ECS::Controller>();
		ECS::Entity::Type audioSourceEntity = static_cast<ECS::Entity::Type>(audioSourceObjectIndex);

		// 1. Get the unique ID for the Transform component type.
		auto transformTypeIdOpt = ecs->getComponentType<Transform>();
		if (transformTypeIdOpt)
		{
			// 2. Get a COPY of the component.
			std::shared_ptr<void> component_as_void = ecs->getCopiedEntityComponent(audioSourceEntity, *transformTypeIdOpt);
			if (component_as_void)
			{
				// 3. Cast the copy to the correct type and modify it.
				std::shared_ptr<Transform> transformComp = std::static_pointer_cast<Transform>(component_as_void);
				transformComp->position = currentPosition;

				// 4. Set the modified copy back into the ECS.
				ecs->setEntityComponent(audioSourceEntity, *transformTypeIdOpt, transformComp);
			}
		}

		// --- Update FMOD Sound Positions ---
		// Update the 3D position of the looping music channel
		if (isValid(audioSourceChannel)) {
			audioManager->setPosition(audioSourceChannel, currentPosition);
		}

		// --- Handle Footstep Playback ---
		footstepTimer -= timing.dt;
		if (footstepTimer <= 0.0f)
		{
			// Play a random footstep from the playlist at the object's current position
			audioManager->playRandom("FootstepsGrass", currentPosition, 0.0f);
			// Reset the timer for the next footstep
			footstepTimer = footstepInterval;
		}
	}

	void Scene::onEvent(Event::Event& e) {}

	std::shared_ptr<Mesh> Scene::AddObject(std::shared_ptr<Mesh> mesh, glm::vec3 pos, glm::quat rot, glm::vec3 scale)
	{
		auto ecs = services->get<ECS::Controller>();
		ECS::Entity::Type entity = ecs->createEntity();
		ecs->addEntityComponent(entity, MetaData::EntityName{ "Example Ogre" });
		ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
		ecs->addEntityComponent(entity, MeshRenderer{ mesh });

		// Populate the legacy m_Objects vector to support systems that haven't migrated to ECS yet (e.g., audio demo)
		glm::mat4 model = glm::translate(glm::mat4(1.0f), pos) * glm::mat4_cast(rot) * glm::scale(glm::mat4(1.0f), scale);
		m_Objects.push_back({ mesh.get(), model });

		return mesh;
	}

	void Scene::DeleteObject(int index)
	{
		m_Objects.erase(m_Objects.begin() + index);
	}

	const std::vector<SceneObject>& Scene::GetObjects() const
	{
		return m_Objects;
	}

	Camera* Scene::GetActiveCamera()
	{
		return camera.get();
	}
}