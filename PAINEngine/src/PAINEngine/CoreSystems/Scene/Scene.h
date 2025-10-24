#pragma once
#include "pch.h"

#include "CoreSystems/Assets/sLoader.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Audio/Audio.h"
#include "Camera.h"


namespace PAIN {

	class Scene : public AppSystem {
	public:
		Scene() = default;
		~Scene() = default;

		void onDetach() override;
		void onAttach() override;
		void onFixedUpdate(AppTiming timing) override {};
		void onUpdate(AppTiming timing) override;
		void onEvent([[maybe_unused]] Event::Event& e) override;

		// Modified to return the created entity's ID
		entt::entity AddObject(uint32_t mesh, const std::string& name, const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale);

		// TO BE MOVEDDDDDDDDDD INTO ASSETS LOADER
		std::unordered_map<uint32_t, std::shared_ptr<Mesh>> meshCache; // Mesh cache

		std::shared_ptr<Mesh> loadMesh(const std::string& path_to_mesh);
		uint32_t cacheMesh(const std::string& path);
		uint32_t getMeshId(const std::string& path);
		std::shared_ptr<Mesh> getMesh(uint32_t mesh_id);

		Camera* GetActiveCamera();

	private:
		std::unique_ptr<Camera> camera;

		// Audio Demo State Variables
		entt::entity audioSourceEntity = entt::null;
		Audio::AudioChannelId audioSourceChannel;

		// Path animation variables
		float demoTime = 0.0f;
		int currentPathSegment = 0;
		float segmentDuration = 4.0f;
		std::vector<glm::vec3> pathCorners;

		// Footstep variables
		float footstepTimer = 0.0f;
		const float footstepInterval = 0.4f;
	};
}