#pragma once
#include "pch.h"

#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Audio/Audio.h"
#include "Camera.h"

namespace PAIN {

	struct SceneObject {
		Mesh* mesh;
		glm::mat4 transform;  // model matrix
	};

	class Scene : public AppSystem {
	public:
		Scene() = default;
		~Scene() = default;

		void onDetach() override;
		void onAttach() override;
		void onFixedUpdate(AppTiming timing) override {};
		void onUpdate(AppTiming timing) override;
		void onEvent([[maybe_unused]] Event::Event& e) override;

		std::shared_ptr<Mesh> AddObject(std::shared_ptr<Mesh> mesh, glm::vec3 pos, glm::quat quat, glm::vec3 scale);
		void DeleteObject(int index);

		const std::vector<SceneObject>& GetObjects() const;
		Camera* GetActiveCamera();
		std::vector<Mesh*> m_Meshes;
		//Entity CreateEntity(const std::string name);
		//void DestroyEntity(Entity entity);

	private:

		std::vector<SceneObject> m_Objects;
		std::unique_ptr<Camera> camera;

		//std::vector<Camera*> cameras; // multiple cams impl
		
		
		//std::unique_ptr<ECS::Controller> ecs;

		// --- Audio Demo State Variables ---
		int audioSourceObjectIndex = -1; // Index of the object that will be our audio source
		Audio::AudioChannelId audioSourceChannel; // Channel ID for the looping sound

		// Path animation variables
		float demoTime = 0.0f;
		int currentPathSegment = 0;
		float segmentDuration = 4.0f; // Time in seconds to travel one segment
		std::vector<glm::vec3> pathCorners;

		// Footstep variables
		float footstepTimer = 0.0f;
		const float footstepInterval = 0.4f; // Play a footstep every 0.4 seconds
	};
}