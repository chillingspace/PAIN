#pragma once

#ifndef GLFW_WINDOW_HPP
#define GLFW_WINDOW_HPP

#include "../Window.h"
#include "Events/Event.h"

namespace PAIN {
	namespace Window {

		//GLFW Window
		class GLFW_Window : public Window {
		private:
			//Window
			GLFWwindow* ptr_window;

			//Window buffer size
			glm::uvec2 frame_buffer;

			//Private internal functions
			virtual void init(Package const& package);
			virtual void shutdown();

			//Callbacks
			static void fbsize_cb([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] int width, [[maybe_unused]] int height);
			static void key_cb([[maybe_unused]] GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, [[maybe_unused]] int mods);
			static void mousebutton_cb([[maybe_unused]] GLFWwindow* window, int button, int action, [[maybe_unused]] int mods);
			static void mousepos_cb([[maybe_unused]] GLFWwindow* window, double xpos, double ypos);
			static void mousescroll_cb([[maybe_unused]] GLFWwindow* window, double xoffset, double yoffset);
			static void windowfocus_cb([[maybe_unused]] GLFWwindow* window, int focused);
			static void dropfile_cb([[maybe_unused]] GLFWwindow* window, int count, const char** paths);
			static void cursorenter_cb([[maybe_unused]] GLFWwindow* window, int entered);
		public:

			//Constructors & Destructors
			GLFW_Window(Package const& package);
			virtual ~GLFW_Window();

			//Update
			bool onUpdate() override;

			//Register callbacks
			void registerCallbacks(void* app) override;
		};
	}
}

#endif
