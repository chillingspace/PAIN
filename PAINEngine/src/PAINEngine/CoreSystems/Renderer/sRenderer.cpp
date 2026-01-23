#include "sRenderer.h"
#include "Core.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Renderer/Windows/WindowsRenderer.h"
#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Scene/Scene.h"
#include "CoreSystems/Renderer/Light.h"
#include "CoreSystems/Renderer/Material.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Renderer/skybox.h"

// For windows event include
#include "CoreSystems/Events/GLFW/WindowEvents.h"

#include "ECS/Controller.h"
#include "ECS/Components/cBoundingVolume.h"

#include "CoreSystems/Windows/GLFW/GLFWWindow.h"

//For imgui viewport
#include "LayeredSystems/LevelEditor/Panels/ViewportPanel.h"
#include "LayeredSystems/LevelEditor/Editor.h"

#include "Systems/Collision/sBVHSystem.h"

namespace PAIN {

	void sRenderer::onDetach()
	{
		w_renderer = nullptr;
	}
	void sRenderer::onAttach() {

		//Create window render
		w_renderer = std::make_unique<WindowsRenderer>();

		w_renderer->Init(services);

		//Init scene
		//m_Scene = services->get<Scene::SceneManager>();

		//Call update one frame to ensure initialization
		onUpdate(AppTiming());

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after sRenderer attach: {}", err);
		}
	}

	void sRenderer::postProcessPass()
	{
		w_renderer->PostProcessPass();
	}

	void sRenderer::onUpdate(AppTiming timing) {

		//Upload textures
		if(getPendingTexUploadCount() > 0) processUploads();
	}

	void sRenderer::queueTexUpload(std::shared_ptr<Assets::Texture> tex) {
		//Unique lock for writing
		std::unique_lock<std::mutex> lock(tex_mutex);
		pending_textures.push_back(tex);
	}

	size_t sRenderer::getPendingTexUploadCount() const {
		//Unique lock for writing
		std::unique_lock<std::mutex> lock(tex_mutex);
		return pending_textures.size();
	}

	void sRenderer::processUploads(int max_per_frame) {
		//Unique lock for writing
		std::unique_lock<std::mutex> lock(tex_mutex);

		int uploaded = 0;
		auto it = pending_textures.begin();

		while (it != pending_textures.end() && (batch_upload || uploaded < max_per_frame)) {
			auto& tex = *it;

			if (!tex->gl_texture) {
				w_renderer->uploadTexture(tex);
			}

			it = pending_textures.erase(it);
			uploaded++;
		}

		//Reset batch upload flag
		if (batch_upload) batch_upload = false;
	}

	void sRenderer::onEvent(Event::Event& e) {
#ifdef PN_PLATFORM_WINDOWS
		if (e.getType() == Event::Type::WindowResize) {
			//PN_CORE_INFO("window resized");

			//glfwGetWindowSize(Window::GLFW_Window::getWindow(), &WindowsRenderer::winWidth, &WindowsRenderer::winHeight);

			//w_renderer->Cleanup();
			//w_renderer->Init(services);

			//auto ecs = services->get<ECS::Controller>();
			auto window_sys = services->get<Window::Window>();
			void* void_p_window = window_sys->getNativeWindow();

			if (void_p_window) {
				GLFWwindow* p_window = reinterpret_cast<GLFWwindow*>(void_p_window);

				glfwGetWindowSize(p_window, &WindowsRenderer::winWidth, &WindowsRenderer::winHeight);

				w_renderer->Cleanup();
				w_renderer->Init(services);

				if (!GS.use_instanced_rendering) {
					w_renderer->initSceneVbo();
				}
			}
			else {
				PN_CORE_ERROR("Cannot get wwindow pointer on window resize in sRender::onEvent!");
				throw std::runtime_error("");
			}

		}
#endif
	}

}