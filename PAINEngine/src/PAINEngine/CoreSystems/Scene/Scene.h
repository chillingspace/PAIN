#pragma once
#include "pch.h"

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

		entt::entity AddObject(const Assets::Model& mdl, const std::string& name, const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale);

		// TO BE MOVEDDDDDDDDDD INTO ASSETS LOADER
		std::unordered_map<uint32_t, std::shared_ptr<Mesh>> meshCache; // Mesh cache
		std::unordered_map <uint32_t, std::shared_ptr<Assets::Model>> modelCache;

		std::shared_ptr<Mesh> loadMesh(const std::string& path_to_mesh);
		//uint32_t cacheMesh(const std::string& path);
		std::shared_ptr<Assets::Model> cacheModel(const std::string& vpath);
		uint32_t getModelId(const std::string& path);
		//std::shared_ptr<Mesh> getMesh(uint32_t mesh_id);
		std::shared_ptr<Assets::Model> getModel(uint32_t model_id);

		Camera* GetActiveCamera();

	private:
		std::unique_ptr<Camera> camera;

		// Audio Demo State Variables
		entt::entity m_audioSourceEntity = entt::null;

		// Path animation variables
		float m_demoTime = 0.0f;
		int m_currentPathSegment = 0;
		float m_segmentDuration = 4.0f;
		std::vector<glm::vec3> m_pathCorners;

		// Footstep variables
		float m_footstepTimer = 0.0f;
		const float m_footstepInterval = 0.4f;
	};
}