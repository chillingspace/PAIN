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

			//Init flag
			bool b_initialized = false;

			float m_AspectRatio = .0f;   
			struct Viewport { int x = 0, y = 0, w = 0, h = 0; } m_Viewport;
			void recomputeViewport(int fb_width, int fb_height);
			glm::uvec2 getFramebufferSize() const;

			bool vsync = true;


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
			virtual ~GLFW_Window();
	
			void set_Vsync(bool set) override;
			bool is_Vsync() const override;

			//Register callbacks
			void registerCallbacks(void* app) override;


			//Update
			void onUpdate(float dt) override;

			//Event call back
			void onEvent(Event::Event& e) override;


			//Get native window
			void* getNativeWindow() const override { return ptr_window; }

			void pollEvents() override;

			void swapBuffers() override;
		};
	}
}

#endif
#endif