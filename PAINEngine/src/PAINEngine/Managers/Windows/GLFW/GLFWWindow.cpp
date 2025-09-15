#include "pch.h"
#include "GLFWWindow.h"

#include "Renderer/OpenGL/OpenGLContext.h"

#include "Managers/Events/WindowEvents.h"
#include "Managers/Events/KeyEvents.h"
#include "Managers/Events/MouseEvents.h"
#include "Managers/Events/AssetEvents.h"

#include "Applications/Application.h"

namespace PAIN {
	namespace Window {

		//Create window
		Window* Window::create(Package const& package) {
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

			glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
			glfwWindowHint(GLFW_DEPTH_BITS, 24);
			glfwWindowHint(GLFW_RED_BITS, 8); glfwWindowHint(GLFW_GREEN_BITS, 8);
			glfwWindowHint(GLFW_BLUE_BITS, 8); glfwWindowHint(GLFW_ALPHA_BITS, 8);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // window dimensions are static

			//Create window
			ptr_window = glfwCreateWindow(static_cast<int>(frame_buffer.x), static_cast<int>(frame_buffer.y), package.title.c_str(), nullptr, nullptr);
			if (!ptr_window) {
				PN_CORE_ERROR("Failed to create window");
				glfwTerminate();
				throw std::exception();
			}

			//Create rendering context
			m_Context = std::make_unique<OpenGLContext>(ptr_window);
			m_Context->Init();

			//Engine Init Successful
			PN_CORE_INFO("Window Created Successfully");
		}

		//Shutdown & release resource
		void GLFW_Window::shutdown() {
			//Clean up window
			glfwDestroyWindow(ptr_window);
			glfwTerminate();
		}

		void GLFW_Window::fbsize_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height) {

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

			//Stop application
			app->terminate();
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
		void GLFW_Window::onUpdate() {

			//Poll window events
			glfwPollEvents();
			m_Context->SwapBuffers();
		}

		void GLFW_Window::onEvent(Event::Event& e) {

			//Early exit condition
			if(!e.isInCategory(Event::Category::Application)) return;

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
			dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {

				PN_CORE_INFO(e.toString());

				//Return false: continue dispatching, true = stop dispatching 
				return false;
				});
		}
	}
}
