#include "pch.h"

#ifdef PN_PLATFORM_WINDOWS
#include "GLFWWindow.h"

#include "CoreSystems/Windows/OpenGL/OpenGLContext.h"
#include "CoreSystems/Renderer/GraphicsSettings.h"

#include "CoreSystems/Events/GLFW/WindowEvents.h"
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/AssetEvents.h"
#include "Applications/Application.h"

namespace {
	GLFWmonitor* SelectBestMonitorForWindow(GLFWwindow* window) {
		int monitorCount = 0;
		GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
		if (!monitors || monitorCount <= 0) {
			return glfwGetPrimaryMonitor();
		}

		if (!window) {
			POINT cursorPos{};
			if (GetCursorPos(&cursorPos)) {
				GLFWmonitor* cursorMonitor = monitors[0];
				for (int i = 0; i < monitorCount; ++i) {
					int mx = 0, my = 0, mw = 0, mh = 0;
					glfwGetMonitorWorkarea(monitors[i], &mx, &my, &mw, &mh);
					if (cursorPos.x >= mx && cursorPos.x < (mx + mw) &&
						cursorPos.y >= my && cursorPos.y < (my + mh)) {
						cursorMonitor = monitors[i];
						break;
					}
				}
				return cursorMonitor;
			}
			return glfwGetPrimaryMonitor();
		}

		int wx = 0, wy = 0, ww = 0, wh = 0;
		glfwGetWindowPos(window, &wx, &wy);
		glfwGetWindowSize(window, &ww, &wh);

		GLFWmonitor* bestMonitor = monitors[0];
		int bestOverlap = -1;

		for (int i = 0; i < monitorCount; ++i) {
			int mx = 0, my = 0, mw = 0, mh = 0;
			glfwGetMonitorWorkarea(monitors[i], &mx, &my, &mw, &mh);

			const int overlapW = std::max(0, std::min(wx + ww, mx + mw) - std::max(wx, mx));
			const int overlapH = std::max(0, std::min(wy + wh, my + mh) - std::max(wy, my));
			const int overlapArea = overlapW * overlapH;
			if (overlapArea > bestOverlap) {
				bestOverlap = overlapArea;
				bestMonitor = monitors[i];
			}
		}

		return bestMonitor ? bestMonitor : glfwGetPrimaryMonitor();
	}

	void LogWindowDisplayDiagnostics(GLFWwindow* window, const char* phaseTag) {
		if (!window) {
			return;
		}

		int winW = 0, winH = 0, fbW = 0, fbH = 0, posX = 0, posY = 0;
		glfwGetWindowSize(window, &winW, &winH);
		glfwGetFramebufferSize(window, &fbW, &fbH);
		glfwGetWindowPos(window, &posX, &posY);

		GLFWmonitor* monitor = glfwGetWindowMonitor(window);
		if (!monitor) {
			monitor = SelectBestMonitorForWindow(window);
		}

		const char* monitorName = monitor ? glfwGetMonitorName(monitor) : "unknown";
		const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;
		const int refreshHz = mode ? mode->refreshRate : -1;
		const int modeW = mode ? mode->width : -1;
		const int modeH = mode ? mode->height : -1;

		const int swapInterval = PAIN::GraphicsSettings::get().swap_interval;
		PN_CORE_INFO(
			"[Window][{}] monitor='{}' mode={}x{}@{}Hz win={}x{} fb={}x{} pos=({}, {}) swap_interval={}",
			phaseTag,
			monitorName ? monitorName : "unknown",
			modeW,
			modeH,
			refreshHz,
			winW,
			winH,
			fbW,
			fbH,
			posX,
			posY,
			swapInterval);
	}
}

namespace PAIN {
	namespace Window {

		//Create window
		Window* Window::create([[maybe_unused]] void* app, Package const& package) {
			return new GLFW_Window(package);
		}

		//Init GLFW
		void GLFW_Window::init(Package const& package) {

			//Set buffer size
			frame_buffer.x = package.width;
			frame_buffer.y = package.height;

			if (!glfwInit()) {
				PN_CORE_ERROR("Failed to initialize GLFW");
				throw std::exception();
			}

			glfwSetErrorCallback([](int error, const char* description) {
				PN_CORE_ERROR("Error {}: {}" , error , description);
				throw std::exception();
				});

			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

			// Request a debug context
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

			glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
			glfwWindowHint(GLFW_DEPTH_BITS, 24);
			glfwWindowHint(GLFW_RED_BITS, 8); glfwWindowHint(GLFW_GREEN_BITS, 8);
			glfwWindowHint(GLFW_BLUE_BITS, 8); glfwWindowHint(GLFW_ALPHA_BITS, 8);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

			// Pick the best monitor for this window path.
			GLFWmonitor* monitor = SelectBestMonitorForWindow(nullptr);

			// Get the video mode of the selected monitor
			const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;

#ifdef _DEBUG
			//Create window
			ptr_window = glfwCreateWindow(static_cast<int>(frame_buffer.x), static_cast<int>(frame_buffer.y), package.title.c_str(), nullptr, nullptr);
			if (!ptr_window) {
				PN_CORE_ERROR("Failed to create window");
				glfwTerminate();
				throw std::exception();
			}
			is_fullscreen_ = false;
			LogWindowDisplayDiagnostics(ptr_window, "init_windowed");
#else
			// Create windowed window in Release mode (user can toggle fullscreen from settings).
			ptr_window = glfwCreateWindow(
				static_cast<int>(frame_buffer.x),
				static_cast<int>(frame_buffer.y),
				package.title.c_str(),
				nullptr,  // nullptr = windowed; fullscreen is toggled via setFullscreen()
				nullptr
			);

			if (!ptr_window) {
				PN_CORE_ERROR("Failed to create window");
				glfwTerminate();
				throw std::exception();
			}
			is_fullscreen_ = false;

			// Disable vsync by default in release mode for uncapped FPS and lowest input lag
			GraphicsSettings::get().swap_interval = 0;

			LogWindowDisplayDiagnostics(ptr_window, "init_windowed");
#endif

			// Persist current windowed reference size for future restore.
			// In release this becomes the preferred windowed fallback size.
			windowed_w_ = static_cast<int>(package.width);
			windowed_h_ = static_cast<int>(package.height);
			glfwGetWindowPos(ptr_window, &windowed_x_, &windowed_y_);

			//Create rendering context
			m_Context = std::make_unique<OpenGLContext>(ptr_window);
			m_Context->Init();
			
			//Setup gl
			glViewport(0, 0, frame_buffer.x, frame_buffer.y);

			//Engine Init Successful
			PN_CORE_INFO("Window Created Successfully");

			//Set window flag to active
			b_active = true;
		}

		//Shutdown & release resource
		void GLFW_Window::shutdown() {
			if (b_active) {

				//Clean up window
				glfwDestroyWindow(ptr_window);
				glfwTerminate();

				b_active = false;
			}
		}

		bool GLFW_Window::isMinimized() const {
			int width, height;
			glfwGetFramebufferSize(ptr_window, &width, &height);
			return (width == 0 || height == 0);
		}

		void GLFW_Window::hideCursor(bool hide)
		{
			int currentMode = glfwGetInputMode(ptr_window, GLFW_CURSOR);
			bool isCurrentlyLocked = (currentMode == GLFW_CURSOR_DISABLED);

			if (hide != isCurrentlyLocked)
			{
				if (hide) {
					// Hides cursor and locks it to center (set it only once)
					int width, height;
					glfwGetWindowSize(ptr_window, &width, &height);

					// Reset to center
					glfwSetCursorPos(ptr_window, width / 2.0, height / 2.0);

					glfwSetInputMode(ptr_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}
				else {
					// Shows cursor and allows free movement
					glfwSetInputMode(ptr_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				}
			}
		}

		void GLFW_Window::setFullscreen(bool fullscreen)
		{
			if (!ptr_window) {
				return;
			}
			if (fullscreen == is_fullscreen_) {
				return;
			}

			GLFWmonitor* monitor = SelectBestMonitorForWindow(ptr_window);
			const GLFWvidmode* mode = monitor ? glfwGetVideoMode(monitor) : nullptr;

			if (fullscreen) {
				// Save current windowed position and size before entering fullscreen.
				glfwGetWindowPos(ptr_window, &windowed_x_, &windowed_y_);
				glfwGetWindowSize(ptr_window, &windowed_w_, &windowed_h_);

				// Use the monitor's native resolution for true exclusive fullscreen.
				// This adapts to the user's display (1080p, 1440p, 4K, etc.) and
				// gives maximum performance by eliminating compositor overhead.
				const int width  = mode ? mode->width  : std::max(640, windowed_w_);
				const int height = mode ? mode->height : std::max(480, windowed_h_);
				const int refreshRate = mode ? mode->refreshRate : GLFW_DONT_CARE;

				// Exclusive fullscreen reduces compositor-induced latency and pacing jitter.
				glfwSetWindowAttrib(ptr_window, GLFW_DECORATED, GLFW_FALSE);
				glfwSetWindowMonitor(ptr_window, monitor, 0, 0, width, height, refreshRate);
				is_fullscreen_ = true;
				PN_CORE_INFO("[Window] Switched to exclusive fullscreen {}x{} @ {}Hz", width, height, refreshRate);
				LogWindowDisplayDiagnostics(ptr_window, "set_fullscreen_on");
			}
			else {
				// Restore previous windowed bounds.
				int winW = std::max(640, windowed_w_);
				int winH = std::max(480, windowed_h_);
				int posX = windowed_x_;
				int posY = windowed_y_;
				if (mode && (posX == 0 && posY == 0)) {
					posX = (mode->width - winW) / 2;
					posY = (mode->height - winH) / 2;
				}

				// Re-enable window decorations for windowed mode
				glfwSetWindowAttrib(ptr_window, GLFW_DECORATED, GLFW_TRUE);
				glfwSetWindowMonitor(ptr_window, nullptr, posX, posY, winW, winH, GLFW_DONT_CARE);
				is_fullscreen_ = false;
				PN_CORE_INFO("[Window] Switched to windowed {}x{} at ({},{})", winW, winH, posX, posY);
				LogWindowDisplayDiagnostics(ptr_window, "set_fullscreen_off");
			}

			// CRITICAL: Reapply swap interval after monitor mode change
			// glfwSetWindowMonitor can reset the swap interval to driver defaults.
			// Respect current graphics settings for consistent frame pacing behavior.
			const int swapInterval = GraphicsSettings::get().swap_interval;
			glfwSwapInterval(swapInterval);
			PN_CORE_INFO("[Window] Swap interval reapplied: {} ({})", swapInterval, swapInterval == 0 ? "no VSync" : "VSync enabled");

			// Update frame buffer after mode change
			int w, h;
			glfwGetFramebufferSize(ptr_window, &w, &h);
			frame_buffer = { w, h };
		}

		void GLFW_Window::fbsize_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height) {
			// Ignore 0x0 resize events
			if (width == 0 || height == 0) {
				return;
			}

			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Dispatch event to app layerssts
			app->pushEventQueue(std::make_shared<Event::WindowResized>(glm::uvec2(width, height)));
		}

		void GLFW_Window::windowfocus_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int focused) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Dispatch event to app layers
			app->pushEventQueue(std::make_shared<Event::WindowFocused>(static_cast<bool>(focused)));
		}

		void GLFW_Window::windowpos_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int xpos, [[maybe_unused]] int ypos) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Dispatch event to app layers
			app->pushEventQueue(std::make_shared<Event::WindowMoved>(glm::uvec2(xpos, ypos)));
		}

		void GLFW_Window::windowclose_cb([[maybe_unused]] GLFWwindow* window) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));
		
			#ifdef _DEBUG
				app->pushEventQueue(std::make_shared<Event::WindowClosed>());
			#else
				//Stop application
				app->terminate();
			#endif // _DEBUG
		}

		void GLFW_Window::key_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int key, [[maybe_unused]] int scancode, [[maybe_unused]] int action, [[maybe_unused]] int mods) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Action switch
			switch (action) {
			case GLFW_PRESS:
			{
				//Create key pressed event
				Event::KeyPressed press_event(key);
				Event::KeyTriggered trigger_event(key);

				//Dispatch event to app layers
				app->pushEventQueue(std::make_shared<Event::KeyPressed>(key));
				app->pushEventQueue(std::make_shared<Event::KeyTriggered>(key));

				break;
			}
			case GLFW_REPEAT: {

				//Create key pressed event
				Event::KeyPressed press_event(key);
				Event::KeyRepeated repeat_event(key);

				//Dispatch event to app layers
				app->pushEventQueue(std::make_shared<Event::KeyPressed>(key));
				app->pushEventQueue(std::make_shared<Event::KeyRepeated>(key));
				break;
			}
			case GLFW_RELEASE: {
				//Create key released event
				Event::KeyReleased event(key);

				//Dispatch event to app layers
				app->pushEventQueue(std::make_shared<Event::KeyReleased>(key));
				break;
			}
			default: {
				PN_CORE_WARN("Invalid Key Event Detected");
				break;
			}
			}
		}

		void GLFW_Window::mousebutton_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int button, [[maybe_unused]] int action, [[maybe_unused]] int mods) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Action switch
			switch (action) {
			case GLFW_PRESS:
			{
				//Create key pressed event
				Event::MouseBtnPressed event(button);

				//Dispatch event to app layers
				app->pushEventQueue(std::make_shared<Event::MouseBtnPressed>(button));
				break;
			}
			case GLFW_RELEASE: {
				//Create key released event
				Event::MouseBtnReleased event(button);

				//Dispatch event to app layers
				app->pushEventQueue(std::make_shared<Event::MouseBtnReleased>(button));
				break;
			}
			default: {
				PN_CORE_WARN("Invalid Button Event Detected");
				break;
			}
			}
		}

		void GLFW_Window::mousepos_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] double xpos, [[maybe_unused]] double ypos) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Dispatch event to app layers
			app->pushEventQueue(std::make_shared<Event::MouseMoved>(glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos))));
		}

		void GLFW_Window::mousescroll_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] double xoffset, [[maybe_unused]] double yoffset) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Dispatch event to app layers
			app->pushEventQueue(std::make_shared<Event::MouseScrolled>(glm::vec2(static_cast<float>(xoffset), static_cast<float>(yoffset))));
		}

		void GLFW_Window::cursorenter_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int entered) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Dispatch event to app layers
			app->pushEventQueue(std::make_shared<Event::CursorEntered>(static_cast<bool>(entered)));
		}

		void GLFW_Window::dropfile_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int count, [[maybe_unused]] const char** paths) {
			//Fetch window class
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

			//Dispatch event to app layers
			app->pushEventQueue(std::make_shared<Event::FileDropped>(count, paths));
		}

		//Construct window
		GLFW_Window::GLFW_Window(Package const& package) {
			init(package);
		}

		//Destruct window
		GLFW_Window::~GLFW_Window() {
			shutdown();
		}

		void GLFW_Window::registerCallbacks(void* app) {

			//Register all callbacks
			glfwSetFramebufferSizeCallback(ptr_window, fbsize_cb);
			glfwSetWindowFocusCallback(ptr_window, windowfocus_cb);
			glfwSetWindowPosCallback(ptr_window, windowpos_cb);
			glfwSetWindowCloseCallback(ptr_window, windowclose_cb);
			glfwSetKeyCallback(ptr_window, key_cb);
			glfwSetMouseButtonCallback(ptr_window, mousebutton_cb);
			glfwSetCursorPosCallback(ptr_window, mousepos_cb);
			glfwSetScrollCallback(ptr_window, mousescroll_cb);
			glfwSetCursorEnterCallback(ptr_window, cursorenter_cb);
			glfwSetDropCallback(ptr_window, dropfile_cb);

			//Storing class in glfw
			glfwSetWindowUserPointer(ptr_window, app);
		}

		void GLFW_Window::pollEvents() {
			//Poll window events
			glfwPollEvents();
		}

		void GLFW_Window::swapBuffers() {
			m_Context->SwapBuffers();
		}
		void GLFW_Window::onUpdate(AppTiming timing) {
		}

		void GLFW_Window::onEvent(Event::Event& e) {

			//Early exit condition
			//if(!e.isInCategory(Event::Category::Application)) return;

			//Create event dispatcher
			Event::Dispatcher dispatcher(e);

			//Dispatch window resized event
			dispatcher.Dispatch<Event::WindowResized>([&](Event::WindowResized& e) -> bool {

				//Update frame buffer size
				frame_buffer = e.getFrameBuffer();

				//Return false: continue dispatching, true = stop dispatching 
				return false;
			});

			//Dispatch window resized event
			dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {

				//PN_CORE_INFO(e.toString());

				//Return false: continue dispatching, true = stop dispatching 
				return false;
				});
			
		}

		glm::uvec2 GLFW_Window::getFrameBuffer() const {
			return frame_buffer;
		}

		void GLFW_Window::safeShutdown()
		{

			if (!ptr_window) return;

			// Fetch your Application instance stored in GLFW user pointer
			auto* app = static_cast<Application*>(glfwGetWindowUserPointer(ptr_window));
			if (app) {
				// Call terminate() on your app, which should handle cleanup and quitting properly
				app->terminate();
			}
			else {
				// Fallback: if app pointer missing, fallback to destroying window directly
				if (b_active) {
					glfwDestroyWindow(ptr_window);
					glfwTerminate();
					b_active = false;
				}
			}


		}

	}
}

#endif
