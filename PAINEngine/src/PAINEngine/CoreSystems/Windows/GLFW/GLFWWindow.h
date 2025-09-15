#pragma once

#ifndef GLFW_WINDOW_HPP
#define GLFW_WINDOW_HPP

#include "../Window.h"
#include "CoreSystems/Windows/GraphicsContext.h"

namespace PAIN {
	namespace Window {

		//GLFW Window
		class GLFW_Window : public Window {
		private:
			//Window
			GLFWwindow* ptr_window;

			//Window buffer size
			glm::uvec2 frame_buffer;

			// Rendering context (OpenGL for now)
			std::unique_ptr<GraphicsContext> m_Context; 

			//Private internal functions
			virtual void init(Package const& package);
			virtual void shutdown();

			//Callbacks
			static void fbsize_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height);
			static void windowfocus_cb([[maybe_unused]] GLFWwindow* window, int focused);
			static void windowpos_cb([[maybe_unused]] GLFWwindow* window, int xpos, int ypos);
			static void windowclose_cb([[maybe_unused]] GLFWwindow* window);
			static void key_cb([[maybe_unused]] GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods);
			static void mousebutton_cb([[maybe_unused]] GLFWwindow* window, int button, int action, [[maybe_unused]] int mods);
			static void mousepos_cb([[maybe_unused]] GLFWwindow* window, double xpos, double ypos);
			static void mousescroll_cb([[maybe_unused]] GLFWwindow* window, double xoffset, double yoffset);
			static void cursorenter_cb([[maybe_unused]] GLFWwindow* window, int entered);
			static void dropfile_cb([[maybe_unused]] GLFWwindow* window, int count, const char** paths);

			void* getNativeWindow() const override { return ptr_window; }

		public:

			//Constructors & Destructors
			GLFW_Window(Package const& package);
			virtual ~GLFW_Window();

	

			//Register callbacks
			void registerCallbacks(void* app) override;

			//Update
			void onUpdate() override;

			//Event call back
			void onEvent(Event::Event& e) override;
		};
	}
}

#endif
