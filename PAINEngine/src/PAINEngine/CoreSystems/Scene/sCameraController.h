#pragma once
#include "pch.h"

#include "Camera.h"
#include "Scene.h"
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"

namespace PAIN {

	class sCameraController : public AppSystem {
	public:
		sCameraController() = default;
		~sCameraController() = default;

		void onDetach() override;
		void onAttach() override;
		void onFixedUpdate(AppTiming timing) override {};
		void onUpdate(AppTiming timing) override;
		void onEvent([[maybe_unused]] Event::Event& e) override;

		// Camera Controls
		bool W_KEYDOWN = false;
		bool A_KEYDOWN = false;
		bool S_KEYDOWN = false;
		bool D_KEYDOWN = false;

		bool SPACE_KEYDOWN = false;
		bool LCTRL_KEYDOWN = false;

		bool mouseButtonDown = false;
		float xOffset = 0.0f;
		float yOffset = 0.0f;

		enum MOVE_MODES {
			CAMERA,
			NUM_MOVE_MODES
		};
		MOVE_MODES move_mode = CAMERA;

	private:
		Camera* camera;
		std::shared_ptr<Scene> m_Scene;
	};
}