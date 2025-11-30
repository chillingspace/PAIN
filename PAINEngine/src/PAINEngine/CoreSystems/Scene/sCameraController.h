#pragma once

#include "Camera.h"
#include "Scene.h"
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"
#include "CoreSystems/Events/Android/TouchEvents.h"
#include "CoreSystems/Events/Android/OtherEvents.h"
#include "CoreSystems/Events/Android/SurfaceEvents.h"
#include "CoreSystems/Renderer/GraphicsSettings.h"

#ifdef PN_PLATFORM_WINDOWS
#include "CoreSystems/Windows/Window.h"
#endif
#ifdef _DEBUG
#include "LayeredSystems/LevelEditor/Editor.h"
#endif


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

        void beginTouchControls(int pointerId, float x, float y);
        void updateTouchControls(int pointerId, float x, float y);
        void endTouchControls(int pointerId);

		glm::mat4 getViewMatrix() const;
		glm::mat4 getProjectionMatrix() const;


		float m_vpWidth = 0.f;
		float m_vpHeight = 0.f;
		float m_vpPosX = 0.f;
		float m_vpPosY = 0.f;
		bool vp_hovered = false;

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
			FREE_FLY,
			FIRST_PERSON
		};
		MOVE_MODES move_mode = FREE_FLY;

	private:
		Camera* camera;
		std::shared_ptr<Scene::SceneManager> m_Scene;

		bool m_isMuted = false; // For toggling all audio (testing)

		float m_surfaceWidth = 0.f;
		float m_surfaceHeight = 0.f;

		// Add these lines:
		float m_mouseLastX = 0.f;
		float m_mouseLastY = 0.f;

		// Mobile Movements
		bool  m_touchLooking = false;
		int   m_touchPointerId = -1;
		float m_touchLastX = 0.f;
		float m_touchLastY = 0.f;


        struct MoveStick {
            bool active = false;
            int id = -1;
            float start_x = 0;
            float start_y = 0;
            float last_x = 0;
            float last_y = 0;
        };
        MoveStick m_move;
        float m_moveRadiusPx = 120.f, m_moveDeadzonePx = 10.f, m_lookSensitivity = 1.0f, m_moveScale = 1.0f;
        float m_cachedMoveX = 0.f, m_cachedMoveY = 0.f;
	};

	struct MobileMoveAxes {
		float x = 0.f;
		float y = 0.f;
	};
	extern MobileMoveAxes g_MobileMoveAxes;

	struct MobileLookDelta {
		float dx = 0.f;  
		float dy = 0.f;  
	};
	extern MobileLookDelta g_MobileLookDelta;

}