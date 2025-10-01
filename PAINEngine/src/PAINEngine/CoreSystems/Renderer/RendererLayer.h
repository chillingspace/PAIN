#pragma once

#include "pch.h"
#include "Scene/Scene.h"
#include "Applications/AppSystem.h"
#include "imgui.h"

#include "Windows/WindowsRenderer.h"



namespace PAIN {
	
	class RendererLayer : public AppSystem {
	public:
		RendererLayer() = default;
		~RendererLayer() = default;

		std::shared_ptr<Scene> m_Scene;

        void onDetach() override { }
        void onAttach() override;
        void onFixedUpdate(AppTiming timing) override {};
        void onUpdate(AppTiming timing) override;

		void onEvent([[maybe_unused]] Event::Event& e) override;

		ImTextureID getFramebufferTexture() const {
			return (ImTextureID)(intptr_t)WindowsRenderer::get().getFinalTexture();
		}
		int getFramebufferWidth() const { return winWidth; }
		int getFramebufferHeight() const { return winHeight; }


	private:

		//std::unique_ptr<WindowsRenderer> w_renderer;
		
		enum MOVE_MODES {
			CAMERA,
			LIGHT,
			NUM_MOVE_MODES
		};
		MOVE_MODES move_mode = CAMERA;

		bool W_KEYDOWN = false;
		bool A_KEYDOWN = false;
		bool S_KEYDOWN = false;
		bool D_KEYDOWN = false;

		bool SPACE_KEYDOWN = false;
		bool LCTRL_KEYDOWN = false;

		bool mouseButtonDown = false;
		float xOffset = 0.0f;
		float yOffset = 0.0f;

		//unsigned int fbo = 0;
		//unsigned int fboTexture = 0;
		//unsigned int rbo = 0;
		//int fbWidth = 1280;
		//int fbHeight = 720;

	};

}
