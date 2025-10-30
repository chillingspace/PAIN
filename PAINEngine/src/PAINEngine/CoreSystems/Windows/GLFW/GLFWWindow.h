#pragma once

#ifdef PN_PLATFORM_WINDOWS

#ifndef GLFW_WINDOW_HPP
#define GLFW_WINDOW_HPP

#include "../Window.h"

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
			void init(Package const& package);
			void shutdown();

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

		public:

			//Constructors & Destructors
			GLFW_Window(Package const& package);
			~GLFW_Window() override;	

			void onDetach() override { shutdown(); }

			//Update
			void onUpdate(AppTiming timing) override;

			//Event call back
			void onEvent(Event::Event& e) override;

			//Register callbacks
			void registerCallbacks(void* app) override;

			//Get native window
			void* getNativeWindow() const override { return ptr_window; }

			void pollEvents() override;

			void swapBuffers() override;

			glm::uvec2 getFrameBuffer() const override;
		};
	}
}

#endif
#endif