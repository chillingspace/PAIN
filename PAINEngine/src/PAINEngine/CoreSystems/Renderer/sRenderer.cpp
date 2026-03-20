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
#ifdef PN_PLATFORM_ANDROID
#include "../Events/Android/SurfaceEvents.h"
#endif

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

	void sRenderer::postProcessPass(bool presentToSwapchain)
	{
		w_renderer->PostProcessPass(presentToSwapchain);
	}

	void sRenderer::onUpdate(AppTiming timing) {
		if (w_renderer->resizeDirty) {
			w_renderer->resizeDirty = false;
			w_renderer->_initDeferredShadingBuffers();
		}
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

			if (tex->gl_texture) {
				it = pending_textures.erase(it);
				uploaded++;
			} else {
				++it;
			}
		}

		//Reset batch upload flag
		if (batch_upload) batch_upload = false;
	}

	void sRenderer::onEvent(Event::Event& e) {
#ifdef PN_PLATFORM_WINDOWS
		if (e.getType() == Event::Type::WindowResize) {
			auto window_sys = services->get<Window::Window>();
			void* void_p_window = window_sys->getNativeWindow();

			if (void_p_window) {
				GLFWwindow* p_window = reinterpret_cast<GLFWwindow*>(void_p_window);
				int w, h;
				glfwGetFramebufferSize(p_window, &w, &h);
				if (w == 0 || h == 0) return;

				WindowsRenderer::winWidth = w;
				WindowsRenderer::winHeight = h;
				w_renderer->resizeDirty = true;
			}
			else {
				PN_CORE_ERROR("Cannot get window pointer on window resize in sRender::onEvent!");
				throw std::runtime_error("");
			}

		}
#endif

#ifdef PN_PLATFORM_ANDROID
		Event::Dispatcher dispatcher(e);

		dispatcher.Dispatch<Event::SurfaceCreated>([&](Event::SurfaceCreated& se) -> bool {
			WindowsRenderer::winWidth = se.getWidth();
			WindowsRenderer::winHeight = se.getHeight();
			if (se.contextWasLost) {
				PN_CORE_WARN("EGL context was lost - performing full renderer reinit");
				w_renderer->Cleanup();
				w_renderer->Init(services);
			} else {
				w_renderer->resizeDirty = true;
			}
			return false;
		});

		dispatcher.Dispatch<Event::SurfaceChanged>([&](Event::SurfaceChanged& se) -> bool {
			WindowsRenderer::winWidth = se.getWidth();
			WindowsRenderer::winHeight = se.getHeight();
			w_renderer->resizeDirty = true;
			return false;
		});

		dispatcher.Dispatch<Event::SurfaceDestroyed>([&](Event::SurfaceDestroyed&) -> bool {
			WindowsRenderer::winWidth = 0;
			WindowsRenderer::winHeight = 0;
			w_renderer->resizeDirty = false;
			return false;
		});
#endif
	}

}
