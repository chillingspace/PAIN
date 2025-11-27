#pragma once

#include "pch.h"
#include "../Scene/Scene.h"
#include "Applications/AppSystem.h"
#include "imgui.h"

#include "Windows/WindowsRenderer.h"

#include "CoreSystems/Windows/Window.h"

namespace PAIN {
	
	class sRenderer : public AppSystem {
	public:
		sRenderer() = default;
		~sRenderer() = default;

		std::shared_ptr<Scene::SceneManager> m_Scene;

		void onDetach() override;
        void onAttach() override;
        void onFixedUpdate(AppTiming timing) override {};
		void postProcessPass();

        void onUpdate(AppTiming timing) override;


		void onEvent([[maybe_unused]] Event::Event& e) override;

		//Get final fbo
		unsigned int getFinalFbo() const {
			return w_renderer->getFinalFbo();
		}

		// Framebuffer for IMGUI Viewport
		ImTextureID getFramebufferTexture() const {
			return w_renderer->getFinalTexture();
		}
		int getFramebufferWidth() const { return services->get<Window::Window>()->getFrameBuffer().x; }
		int getFramebufferHeight() const { return services->get<Window::Window>()->getFrameBuffer().y; }

		std::unique_ptr<WindowsRenderer> w_renderer;

	};
}