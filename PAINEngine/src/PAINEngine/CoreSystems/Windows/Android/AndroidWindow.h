#pragma once

#ifdef PN_PLATFORM_ANDROID

#ifndef ANDROID_WINDOW_HPP
#define ANDROID_WINDOW_HPP

#include "../Window.h"

namespace PAIN {
	namespace Window {

		//GLFW Window
		class Android_Window : public Window {
		private:
			//Packaged for events
			struct EventsPackage {
				void* window = nullptr;
				void* app = nullptr;
			};

            EventsPackage m_EventPackage;

			ANativeWindow* m_Window = nullptr;
			android_app* m_App = nullptr;
			EGLDisplay m_Display = EGL_NO_DISPLAY;
			EGLConfig  config = nullptr;
			EGLSurface m_Surface = EGL_NO_SURFACE;
			EGLContext m_Context = EGL_NO_CONTEXT;

			//Android buffer size
			glm::uvec2 frame_buffer;

			//Boolean to track setup progress
			bool b_initialized = false;
			bool b_displayready = false;
			bool b_contextready = false;
			bool b_surfaceready = false;

			//Android setup functions
			bool initDisplay();
			bool setConfig();
			bool createContext();
			bool createSurface();
			bool makeCurrent();
			bool querySurfaceDimensions();

			//Android destroy functions
			void destroySurface();
			void destroyContext();
			void terminateDisplay();

			//Private internal functions
			virtual void init();
			virtual void shutdown();

			//Static callbacks
			static int32_t handle_input(android_app* app, AInputEvent* event);
			static void handle_cmd(android_app* app, int32_t cmd);

		public:

			//Constructors & Destructors
			Android_Window(void* app, Package const& package);
			virtual ~Android_Window();

			//Update
			void onUpdate() override;

			//Event call back
			void onEvent(Event::Event& e) override;

			//Register callbacks
			void registerCallbacks(void* app) override;

			//Get android native window
			void* getNativeWindow() const override { return m_Window; }

			//Poll for android window events
			void pollEvents() override;

			//Android swap buffers
			void swapBuffers() override;
		};
	}
}

#endif
#endif
