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
#include "GraphicsSettings.h"
#include "ThermalProfiler.h"
#include <filesystem>

// For windows event include
#include "CoreSystems/Events/GLFW/WindowEvents.h"
#ifdef PN_PLATFORM_ANDROID
#include "../Events/Android/SurfaceEvents.h"
#include "../Windows/Android/AndroidWindow.h"
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
		if (g_ThermalProfiler) {
			g_ThermalProfiler->Shutdown();
			g_ThermalProfiler.reset();
		}
		w_renderer = nullptr;
	}

	// Thermal Profiler - Set to 1 to enable, 0 to disable
#define PN_ENABLE_THERMAL_PROFILER 0

	void sRenderer::onAttach() {
#if PN_ENABLE_THERMAL_PROFILER && defined(PN_PLATFORM_ANDROID)
		g_ThermalProfiler = std::make_unique<ThermalProfiler>();
		g_ThermalProfiler->Init();

		if (g_ThermalProfiler) {
			auto window_sys = services->get<Window::Window>();
			auto* androidWindow = static_cast<Window::Android_Window*>(window_sys.get());
			std::string writablePath = androidWindow ? androidWindow->getWritablePath() : "";
			if (!writablePath.empty()) {
				std::filesystem::path logDir = std::filesystem::path(writablePath) / "PAIN";
				std::error_code ec;
				std::filesystem::create_directories(logDir, ec);
				if (!ec) {
					g_ThermalProfiler->EnableLogging((logDir / "thermal_profile.csv").string());
				} else {
					PN_CORE_WARN("Failed to create thermal log directory: {}", ec.message());
				}
			} else {
				PN_CORE_WARN("Android writable path unavailable, thermal CSV logging disabled");
			}
		}
#endif

		w_renderer = std::make_unique<WindowsRenderer>();
		w_renderer->Init(services);

		onUpdate(AppTiming());

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after sRenderer attach: {}", err);
		}
	}

	void sRenderer::postProcessPass(bool presentToSwapchain)
	{
		// Skip post-processing if disabled
		if (!GraphicsSettings::get().postprocess) {
			// Just blit the final_fbo to screen without any post-processing
			w_renderer->BlitFinalToScreen();
			return;
		}
		w_renderer->PostProcessPass(presentToSwapchain);
	}

	void sRenderer::onUpdate(AppTiming timing) {
		if (w_renderer->resizeDirty) {
			w_renderer->resizeDirty = false;
			w_renderer->_initDeferredShadingBuffers();
		}
		//Upload textures
		if(getPendingTexUploadCount() > 0) processUploads();

#ifdef PN_PLATFORM_ANDROID
		if (timing.dt > 0.0f) {
			static float smoothedFrameMs = 16.67f;
			static bool thermalReduced = false;
			const float frameMs = timing.dt * 1000.0f;
			smoothedFrameMs = smoothedFrameMs * 0.92f + frameMs * 0.08f;

			auto& gs = GraphicsSettings::get();
			if (!thermalReduced && smoothedFrameMs > 27.0f) {
				thermalReduced = true;
				gs.android_battery_saver_mode = true;
				gs.android_target_fps = gs.android_battery_saver_fps;
				gs.bloom_quality = std::max(1, gs.bloom_quality - 1);
				gs.volumetric_steps = std::max(6, gs.volumetric_steps - 2);
				gs.volumetric_resolution_scale = std::max(0.3f, gs.volumetric_resolution_scale - 0.1f);
				PN_CORE_WARN("Android thermal guard enabled (smoothed {:.2f} ms)", smoothedFrameMs);
			} else if (thermalReduced && smoothedFrameMs < 19.0f) {
				thermalReduced = false;
				gs.android_battery_saver_mode = false;
				gs.android_target_fps = 0;
				gs.bloom_quality = std::max(gs.bloom_quality, 2);
				gs.volumetric_steps = std::max(gs.volumetric_steps, 10);
				gs.volumetric_resolution_scale = std::max(gs.volumetric_resolution_scale, 0.4f);
				PN_CORE_INFO("Android thermal guard disabled (smoothed {:.2f} ms)", smoothedFrameMs);
			}
		}
#endif
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
