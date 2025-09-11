#include "pch.h"
#include "GLFWWindow.h"

namespace PAIN {
	namespace Window {

		//Create window
		Window* Window::create(Package const& package) {
			return new GLFW_Window(package);
		}

		//Init GLFW
		void GLFW_Window::init(Package const& package) {
			GLenum err;

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
			ptr_window = glfwCreateWindow(static_cast<int>(package.width), static_cast<int>(package.height), package.title.c_str(), nullptr, nullptr);
			if (!ptr_window) {
				PN_CORE_ERROR("Failed to create window");
				glfwTerminate();
				throw std::exception();
			}

			glfwMakeContextCurrent(ptr_window);
			if (!glfwGetCurrentContext()) {
				throw std::exception("Context creation failed.");
			}

			// clear glErrors
			while (glGetError() != GL_NO_ERROR) {}

			err = glewInit();
			if (err != GLEW_OK) {
				PN_CORE_ERROR("GLEW init failed: {}", (const char*)(glewGetErrorString(err)));
				throw std::exception();
			}

			err = glGetError();
			if (err != GL_NO_ERROR) {
			}

			//Engine Init Successful
			PN_CORE_INFO("Window Created Successfully");
		}

		//Shutdown & release resource
		void GLFW_Window::shutdown() {
			//Clean up window
			glfwDestroyWindow(ptr_window);
			glfwTerminate();
		}

		//Construct window
		GLFW_Window::GLFW_Window(Package const& package) {
			init(package);
		}

		//Destruct window
		GLFW_Window::~GLFW_Window() {
			shutdown();
		}

		//Window update
		bool GLFW_Window::onUpdate() {

			//Poll window events
			glfwPollEvents();
			
			//Check for trigger to close window
			if (!glfwWindowShouldClose(ptr_window)) {
				return true;
			}

			return false;
		}
	}
}
