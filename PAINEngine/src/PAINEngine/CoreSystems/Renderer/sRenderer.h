#pragma once

#include "pch.h"
#include "../Scene/Scene.h"
#include "Applications/AppSystem.h"
#include "imgui.h"

#include "Windows/WindowsRenderer.h"



namespace PAIN {
	
	class sRenderer : public AppSystem {
	public:
		sRenderer() = default;
		~sRenderer() = default;

		std::shared_ptr<Scene> m_Scene;



		void onDetach() override;
        void onAttach() override;
        void onFixedUpdate(AppTiming timing) override {};

		void shadowPass();
		void geometryPass();
		void lightingPass();
		void debugPass(bool show_debug);
		void postProcessPass();
		void uiPass();

        void onUpdate(AppTiming timing) override;


		void onEvent([[maybe_unused]] Event::Event& e) override;

		// Framebuffer for IMGUI Viewport
		ImTextureID getFramebufferTexture() const {
			return w_renderer->getFinalTexture();
		}
		int getFramebufferWidth() const { return winWidth; }
		int getFramebufferHeight() const { return winHeight; }

		std::unique_ptr<WindowsRenderer> w_renderer;

	};
}