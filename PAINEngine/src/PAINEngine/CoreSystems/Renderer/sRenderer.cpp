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
#include <algorithm>
#include <array>
#include <cctype>

#ifdef PN_PLATFORM_ANDROID
#include <sys/system_properties.h>
#endif

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
#ifdef PN_PLATFORM_ANDROID
	namespace {
		enum class AndroidGraphicsTier {
			High = 0,
			Mid = 1,
			Low = 2
		};

		struct AndroidGraphicsBaseline {
			bool captured = false;
			bool bloom = true;
			int bloom_quality = 2;
			bool volumetric = true;
			int volumetric_steps = 6;
			float volumetric_resolution_scale = 0.4f;
			float postprocess_resolution_scale = 0.75f;
			GraphicsSettings::SHADOW_TYPES shadow_type = GraphicsSettings::SHADOW_TYPES::HARD;
			int android_target_fps = 0;
			bool android_battery_saver_mode = false;
		};

		struct AndroidGraphicsScalerState {
			bool initialized = false;
			AndroidGraphicsTier startupTier = AndroidGraphicsTier::High;
			AndroidGraphicsTier activeTier = AndroidGraphicsTier::High;
			float smoothedFrameMs = 16.67f;
			int degradeFrames = 0;
			int recoverFrames = 0;
			int cooldownFrames = 0;
		};

		struct AndroidDeviceFingerprint {
			std::string vendor;
			std::string renderer;
			std::string version;
			std::string manufacturer;
			std::string model;
			std::string hardware;
		};

		AndroidGraphicsBaseline g_androidGraphicsBaseline{};
		AndroidGraphicsScalerState g_androidGraphicsScaler{};

		std::string ToLowerAscii(std::string value) {
			std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
				return static_cast<char>(std::tolower(c));
			});
			return value;
		}

		std::string GetSystemProperty(const char* key) {
			char value[PROP_VALUE_MAX] = {0};
			const int len = __system_property_get(key, value);
			if (len <= 0) {
				return {};
			}
			return std::string(value, static_cast<size_t>(len));
		}

		std::string SafeGlString(GLenum valueType) {
			const GLubyte* value = glGetString(valueType);
			return value ? std::string(reinterpret_cast<const char*>(value)) : std::string();
		}

		AndroidGraphicsTier LowerTier(AndroidGraphicsTier tier) {
			switch (tier) {
			case AndroidGraphicsTier::High: return AndroidGraphicsTier::Mid;
			case AndroidGraphicsTier::Mid: return AndroidGraphicsTier::Low;
			case AndroidGraphicsTier::Low: return AndroidGraphicsTier::Low;
			}
			return AndroidGraphicsTier::Low;
		}

		AndroidGraphicsTier HigherTier(AndroidGraphicsTier tier) {
			switch (tier) {
			case AndroidGraphicsTier::Low: return AndroidGraphicsTier::Mid;
			case AndroidGraphicsTier::Mid: return AndroidGraphicsTier::High;
			case AndroidGraphicsTier::High: return AndroidGraphicsTier::High;
			}
			return AndroidGraphicsTier::High;
		}

		const char* TierToString(AndroidGraphicsTier tier) {
			switch (tier) {
			case AndroidGraphicsTier::High: return "HIGH";
			case AndroidGraphicsTier::Mid: return "MID";
			case AndroidGraphicsTier::Low: return "LOW";
			}
			return "UNKNOWN";
		}

		void CaptureAndroidGraphicsBaselineIfNeeded() {
			if (g_androidGraphicsBaseline.captured) {
				return;
			}
			auto& gs = GraphicsSettings::get();
			g_androidGraphicsBaseline.captured = true;
			g_androidGraphicsBaseline.bloom = gs.bloom;
			g_androidGraphicsBaseline.bloom_quality = gs.bloom_quality;
			g_androidGraphicsBaseline.volumetric = gs.volumetric;
			g_androidGraphicsBaseline.volumetric_steps = gs.volumetric_steps;
			g_androidGraphicsBaseline.volumetric_resolution_scale = gs.volumetric_resolution_scale;
			g_androidGraphicsBaseline.postprocess_resolution_scale = gs.postprocess_resolution_scale;
			g_androidGraphicsBaseline.shadow_type = gs.shadow_type;
			g_androidGraphicsBaseline.android_target_fps = gs.android_target_fps;
			g_androidGraphicsBaseline.android_battery_saver_mode = gs.android_battery_saver_mode;
		}

		void ApplyAndroidTierPreset(AndroidGraphicsTier tier) {
			CaptureAndroidGraphicsBaselineIfNeeded();
			auto& gs = GraphicsSettings::get();
			const auto& base = g_androidGraphicsBaseline;

			if (tier == AndroidGraphicsTier::High) {
				gs.bloom = base.bloom;
				gs.bloom_quality = base.bloom_quality;
				gs.volumetric = base.volumetric;
				gs.volumetric_steps = base.volumetric_steps;
				gs.volumetric_resolution_scale = base.volumetric_resolution_scale;
				gs.postprocess_resolution_scale = base.postprocess_resolution_scale;
				gs.shadow_type = base.shadow_type;
				gs.android_target_fps = base.android_target_fps;
				gs.android_battery_saver_mode = false;
				return;
			}

			gs.shadow_type = GraphicsSettings::SHADOW_TYPES::HARD;
			gs.android_battery_saver_mode = false;
			gs.android_target_fps = 0;

			if (tier == AndroidGraphicsTier::Mid) {
				gs.postprocess_resolution_scale = std::min(base.postprocess_resolution_scale, 0.67f);
				gs.bloom = base.bloom;
				gs.bloom_quality = base.bloom ? std::min(base.bloom_quality, 1) : base.bloom_quality;
				gs.volumetric = base.volumetric;
				gs.volumetric_steps = base.volumetric ? std::min(base.volumetric_steps, 4) : base.volumetric_steps;
				gs.volumetric_resolution_scale = base.volumetric
					? std::min(base.volumetric_resolution_scale, 0.33f)
					: base.volumetric_resolution_scale;
				return;
			}

			gs.postprocess_resolution_scale = std::min(base.postprocess_resolution_scale, 0.5f);
			gs.bloom = false;
			gs.bloom_quality = std::min(base.bloom_quality, 1);
			gs.volumetric = false;
			gs.volumetric_steps = std::min(base.volumetric_steps, 2);
			gs.volumetric_resolution_scale = std::min(base.volumetric_resolution_scale, 0.25f);
		}

		AndroidDeviceFingerprint DetectAndroidDeviceFingerprint() {
			AndroidDeviceFingerprint out{};
			out.vendor = ToLowerAscii(SafeGlString(GL_VENDOR));
			out.renderer = ToLowerAscii(SafeGlString(GL_RENDERER));
			out.version = ToLowerAscii(SafeGlString(GL_VERSION));
			out.manufacturer = ToLowerAscii(GetSystemProperty("ro.product.manufacturer"));
			out.model = ToLowerAscii(GetSystemProperty("ro.product.model"));
			out.hardware = ToLowerAscii(GetSystemProperty("ro.hardware"));
			return out;
		}

		AndroidGraphicsTier ClassifyAndroidStartupTier(const AndroidDeviceFingerprint& fp) {
			const std::string merged = fp.vendor + " " + fp.renderer + " " + fp.manufacturer + " " + fp.model + " " + fp.hardware;

			if (merged.find("pixel 9 pro") != std::string::npos ||
				merged.find("poco f8 pro") != std::string::npos ||
				merged.find("adreno 750") != std::string::npos ||
				merged.find("adreno 740") != std::string::npos ||
				merged.find("adreno 730") != std::string::npos) {
				return AndroidGraphicsTier::High;
			}

			if (merged.find("s23 fe") != std::string::npos ||
				merged.find("xclipse 920") != std::string::npos ||
				merged.find("mali-g715") != std::string::npos ||
				merged.find("mali-g710") != std::string::npos ||
				merged.find("adreno 720") != std::string::npos) {
				return AndroidGraphicsTier::Mid;
			}

			if (merged.find("a55") != std::string::npos ||
				merged.find("a65") != std::string::npos ||
				merged.find("mali-g68") != std::string::npos ||
				merged.find("mali-g57") != std::string::npos ||
				merged.find("mali-g52") != std::string::npos ||
				merged.find("adreno 6") != std::string::npos) {
				return AndroidGraphicsTier::Low;
			}

			if (merged.find("adreno") != std::string::npos) {
				return AndroidGraphicsTier::Mid;
			}
			if (merged.find("mali") != std::string::npos || merged.find("powervr") != std::string::npos) {
				return AndroidGraphicsTier::Low;
			}

			return AndroidGraphicsTier::Mid;
		}

		void InitializeAndroidGraphicsScaler() {
			if (g_androidGraphicsScaler.initialized) {
				return;
			}

			CaptureAndroidGraphicsBaselineIfNeeded();
			const AndroidDeviceFingerprint fp = DetectAndroidDeviceFingerprint();
			const AndroidGraphicsTier startupTier = ClassifyAndroidStartupTier(fp);
			ApplyAndroidTierPreset(startupTier);

			g_androidGraphicsScaler.initialized = true;
			g_androidGraphicsScaler.startupTier = startupTier;
			g_androidGraphicsScaler.activeTier = startupTier;
			g_androidGraphicsScaler.smoothedFrameMs = 16.67f;
			g_androidGraphicsScaler.degradeFrames = 0;
			g_androidGraphicsScaler.recoverFrames = 0;
			g_androidGraphicsScaler.cooldownFrames = 120;

			PN_CORE_INFO(
				"Android graphics scaler initialized: tier={} vendor='{}' renderer='{}' manufacturer='{}' model='{}'",
				TierToString(startupTier),
				fp.vendor,
				fp.renderer,
				fp.manufacturer,
				fp.model);
		}

		void UpdateAndroidGraphicsScaler(float dtSeconds) {
			if (dtSeconds <= 0.0f) {
				return;
			}
			if (!g_androidGraphicsScaler.initialized) {
				InitializeAndroidGraphicsScaler();
			}

			auto& state = g_androidGraphicsScaler;
			state.smoothedFrameMs = state.smoothedFrameMs * 0.90f + (dtSeconds * 1000.0f) * 0.10f;

			constexpr float kDegradeThresholdMs = 20.0f;
			constexpr float kRecoverThresholdMs = 15.5f;
			constexpr int kDegradeWindowFrames = 120;
			constexpr int kRecoverWindowFrames = 300;
			constexpr int kTierChangeCooldownFrames = 120;

			if (state.cooldownFrames > 0) {
				--state.cooldownFrames;
			}

			if (state.smoothedFrameMs > kDegradeThresholdMs) {
				++state.degradeFrames;
				state.recoverFrames = 0;
			}
			else if (state.smoothedFrameMs < kRecoverThresholdMs) {
				++state.recoverFrames;
				state.degradeFrames = 0;
			}
			else {
				state.degradeFrames = 0;
				state.recoverFrames = 0;
			}

			if (state.cooldownFrames > 0) {
				return;
			}

			if (state.degradeFrames >= kDegradeWindowFrames) {
				const AndroidGraphicsTier nextTier = LowerTier(state.activeTier);
				if (nextTier != state.activeTier) {
					state.activeTier = nextTier;
					ApplyAndroidTierPreset(state.activeTier);
					PN_CORE_WARN(
						"Android graphics scaled down to {} (smoothed {:.2f}ms)",
						TierToString(state.activeTier),
						state.smoothedFrameMs);
				}
				state.degradeFrames = 0;
				state.recoverFrames = 0;
				state.cooldownFrames = kTierChangeCooldownFrames;
				return;
			}

			if (state.recoverFrames >= kRecoverWindowFrames) {
				const AndroidGraphicsTier nextTier = HigherTier(state.activeTier);
				if (nextTier != state.activeTier) {
					state.activeTier = nextTier;
					ApplyAndroidTierPreset(state.activeTier);
					PN_CORE_INFO(
						"Android graphics scaled up to {} (smoothed {:.2f}ms)",
						TierToString(state.activeTier),
						state.smoothedFrameMs);
				}
				state.degradeFrames = 0;
				state.recoverFrames = 0;
				state.cooldownFrames = kTierChangeCooldownFrames;
			}
		}
	}
#endif

	void sRenderer::onDetach()
	{
		if (g_ThermalProfiler) {
			g_ThermalProfiler->Shutdown();
			g_ThermalProfiler.reset();
		}
		w_renderer = nullptr;
	}

	// Thermal Profiler - Set to 1 to enable, 0 to disable
#define PN_ENABLE_THERMAL_PROFILER 1

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

#ifdef PN_PLATFORM_ANDROID
		InitializeAndroidGraphicsScaler();
#endif

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
			if (presentToSwapchain) {
				// Runtime mode: present scene directly to swapchain.
				w_renderer->BlitFinalToScreen();
			}
			else {
				// Editor mode: keep rendering into renderer-owned final target.
				glBindFramebuffer(GL_FRAMEBUFFER, getFinalFbo());
			}
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
		UpdateAndroidGraphicsScaler(timing.dt);
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
		batch_upload = false;
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
