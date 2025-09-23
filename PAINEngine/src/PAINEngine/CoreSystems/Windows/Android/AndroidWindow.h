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

			ANativeWindow* m_Window = nullptr;
			android_app* m_App = nullptr;
			EGLDisplay m_Display = EGL_NO_DISPLAY;
			EGLSurface m_Surface = EGL_NO_SURFACE;
			EGLContext m_Context = EGL_NO_CONTEXT;

			//Android buffer size
			glm::uvec2 frame_buffer;

			//Anrdoid state
			bool b_animating = false;
			bool b_initialized = false;

			//Private internal functions
			virtual void init(Package const& package);
			virtual void shutdown();

			//Get android native window
			void* getNativeWindow() const override { return m_Window; }

		public:

			//Constructors & Destructors
			Android_Window(void* app, Package const& package);
			virtual ~Android_Window();

			//Register callbacks
			void registerCallbacks(void* app) override;

			//On attach
			void onAttach() override;

			//Update
			void onUpdate() override;

			//Event call back
			void onEvent(Event::Event& e) override;
		};
	}
}

#endif
#endif
