#include "pch.h"

#ifdef PN_PLATFORM_ANDROID

#include "AndroidWindow.h"

namespace PAIN {
	namespace Window {

        Window* Window::create(Package const& package) {
            return new Android_Window(package);
        }

        Android_Window::Android_Window(Package const& package) {
            init(package);
        }

        Android_Window::~Android_Window() {
            shutdown();
        }

        void Android_Window::init(Package const& package) {
            frame_buffer.x = package.width;
            frame_buffer.y = package.height;

            m_Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            eglInitialize(m_Display, nullptr, nullptr);

            EGLConfig config;
            EGLint numConfigs;
            EGLint attribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
                EGL_NONE
            };
            eglChooseConfig(m_Display, attribs, &config, 1, &numConfigs);

            m_Surface = eglCreateWindowSurface(m_Display, config, m_Window, nullptr);

            EGLint ctxAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE };
            m_Context = eglCreateContext(m_Display, config, EGL_NO_CONTEXT, ctxAttribs);

            eglMakeCurrent(m_Display, m_Surface, m_Surface, m_Context);
        }

        void Android_Window::shutdown() {
            if (m_Display != EGL_NO_DISPLAY) {
                eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                if (m_Context != EGL_NO_CONTEXT) eglDestroyContext(m_Display, m_Context);
                if (m_Surface != EGL_NO_SURFACE) eglDestroySurface(m_Display, m_Surface);
                eglTerminate(m_Display);
            }
            m_Display = EGL_NO_DISPLAY;
            m_Context = EGL_NO_CONTEXT;
            m_Surface = EGL_NO_SURFACE;
        }

        void Android_Window::registerCallbacks(void* app) {
        }

        void Android_Window::onUpdate() {
            eglSwapBuffers(m_Display, m_Surface);
        }

        void Android_Window::onEvent(Event::Event& e) {
        }

	}
}

#endif
