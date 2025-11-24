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

		entt::entity AddObject(const std::shared_ptr<Assets::Model>& mdl, const std::string& name, const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale, Assets::GUID const& diff_id = Assets::GUID{}, Assets::GUID const& ao_id = Assets::GUID{});

		Camera* GetActiveCamera();
		void SetActiveCamera(Camera* cam);

		void SetEditorCamera();

		void SetGameCamera();


	private:
		Camera* active_camera = nullptr;
		std::unique_ptr<Camera> editor_camera;
		std::unique_ptr<Camera> game_camera;

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