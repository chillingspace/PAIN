#pragma once

#include "pch.h"
#include "../Scene/Scene.h"
#include "Applications/AppSystem.h"
#include "imgui.h"

#include "Windows/WindowsRenderer.h"



namespace PAIN {
	
	class RendererLayer : public AppSystem {
	public:
		RendererLayer() = default;
		~RendererLayer() = default;

		std::shared_ptr<Scene> m_Scene;

		void onDetach() override;
        void onAttach() override;
        void onFixedUpdate(AppTiming timing) override {};
        void onUpdate(AppTiming timing) override;

		void renderScene();

		void onEvent([[maybe_unused]] Event::Event& e) override;

		ImTextureID getFramebufferTexture() const {
			return w_renderer->getFinalTexture();
		}
		int getFramebufferWidth() const { return winWidth; }
		int getFramebufferHeight() const { return winHeight; }


		bool W_KEYDOWN = false;
		bool A_KEYDOWN = false;
		bool S_KEYDOWN = false;
		bool D_KEYDOWN = false;

		bool SPACE_KEYDOWN = false;
		bool LCTRL_KEYDOWN = false;

		bool mouseButtonDown = false;
		float xOffset = 0.0f;
		float yOffset = 0.0f;

		// Audio mute state for Windows keybinds
		bool m_musicMuted = false;
		bool m_sfxMuted = false;

		std::unique_ptr<WindowsRenderer> w_renderer;
		
		enum MOVE_MODES {
			CAMERA,
			NUM_MOVE_MODES
		};
		MOVE_MODES move_mode = CAMERA;


		//unsigned int fbo = 0;
		//unsigned int fboTexture = 0;
		//unsigned int rbo = 0;
		//int fbWidth = 1280;
		//int fbHeight = 720;
	};
}