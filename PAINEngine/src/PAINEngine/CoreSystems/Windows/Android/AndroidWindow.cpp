#include "pch.h"

#ifdef PN_PLATFORM_ANDROID

#include "AndroidWindow.h"

namespace PAIN {
	namespace Window {

        Window* Window::create(void* app, Package const& package) {
            return new Android_Window(app, package);
        }

        void Android_Window::handle_cmd(android_app* app, int32_t cmd) {

            Android_Window* state = (Android_Window*)app->userData;

            switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                if (app->window != nullptr) {
                    // Create window with the native window
                    PAIN::Window::Package windowPackage;
                    windowPackage.width = ANativeWindow_getWidth(app->window);
                    windowPackage.height = ANativeWindow_getHeight(app->window);
                    windowPackage.title = "PAIN Engine Android";

                    // Create window with native handle
                    state->init(windowPackage);
                }
                break;

            case APP_CMD_TERM_WINDOW:
                break;

            case APP_CMD_GAINED_FOCUS:
                break;

            case APP_CMD_LOST_FOCUS:
                break;

            case APP_CMD_PAUSE:
                break;

            case APP_CMD_RESUME:
                break;
            }
        }

        Android_Window::Android_Window(void* app, Package const& package) {
            m_App = static_cast<android_app*>(app);
        }

        Android_Window::~Android_Window() {
            shutdown();
        }

        void Android_Window::init(Package const& package) {
            if (!m_Window) {
                PN_CORE_ERROR("Cannot initialize Android_Window without native window!");
                return;
            }

            frame_buffer.x = package.width;
            frame_buffer.y = package.height;

            // Initialize EGL
            const EGLint attribs[] = {
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_DEPTH_SIZE, 24,
                EGL_STENCIL_SIZE, 8,
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL_NONE
            };

            m_Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (m_Display == EGL_NO_DISPLAY) {
                PN_CORE_ERROR("eglGetDisplay failed");
                return;
            }

            EGLint major, minor;
            if (!eglInitialize(m_Display, &major, &minor)) {
                PN_CORE_ERROR("eglInitialize failed");
                return;
            }
            PN_CORE_INFO("EGL initialized: %d.%d", major, minor);

            EGLConfig config;
            EGLint numConfigs;
            if (!eglChooseConfig(m_Display, attribs, &config, 1, &numConfigs)) {
                PN_CORE_ERROR("eglChooseConfig failed");
                return;
            }

            // Set native window format
            EGLint format;
            eglGetConfigAttrib(m_Display, config, EGL_NATIVE_VISUAL_ID, &format);
            ANativeWindow_setBuffersGeometry(m_Window, 0, 0, format);

            m_Surface = eglCreateWindowSurface(m_Display, config, m_Window, nullptr);
            if (m_Surface == EGL_NO_SURFACE) {
                PN_CORE_ERROR("eglCreateWindowSurface failed");
                return;
            }

            const EGLint ctxAttribs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 3,
                EGL_NONE
            };

            m_Context = eglCreateContext(m_Display, config, EGL_NO_CONTEXT, ctxAttribs);
            if (m_Context == EGL_NO_CONTEXT) {
                PN_CORE_ERROR("eglCreateContext failed");
                return;
            }

            if (!eglMakeCurrent(m_Display, m_Surface, m_Surface, m_Context)) {
                PN_CORE_ERROR("eglMakeCurrent failed");
                return;
            }

            // Query actual surface size
            EGLint width, height;
            eglQuerySurface(m_Display, m_Surface, EGL_WIDTH, &width);
            eglQuerySurface(m_Display, m_Surface, EGL_HEIGHT, &height);
            frame_buffer.x = width;
            frame_buffer.y = height;

            PN_CORE_INFO("Android Window initialized: %dx%d", width, height);

            // Set viewport
            glViewport(0, 0, width, height);

            // Enable depth testing
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS);

            //Set animating to true
            b_animating = true;
        }

        void Android_Window::shutdown() {
            if (m_Display != EGL_NO_DISPLAY) {
                eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

                if (m_Context != EGL_NO_CONTEXT) {
                    eglDestroyContext(m_Display, m_Context);
                    m_Context = EGL_NO_CONTEXT;
                }

                if (m_Surface != EGL_NO_SURFACE) {
                    eglDestroySurface(m_Display, m_Surface);
                    m_Surface = EGL_NO_SURFACE;
                }

                eglTerminate(m_Display);
                m_Display = EGL_NO_DISPLAY;
            }

            m_Window = nullptr;
            PN_CORE_INFO("Android Window shut down");
        }

        void Android_Window::registerCallbacks(void* app) {
        }

        void Android_Window::onAttach() {
            m_App->userData = this;
            m_App->onAppCmd = handle_cmd;
            m_Window = m_App->window;
        }

        void Android_Window::onUpdate() {

            //Events
            int events;
            android_poll_source* source;

            // Process all pending events
            while (ALooper_pollOnce(b_animating ? 0 : -1, nullptr, &events,
                (void**)&source) >= 0) {
                if (source) {
                    source->process(m_App, source);
                }
            }

            //Swap buffers
            if (m_Display != EGL_NO_DISPLAY && m_Surface != EGL_NO_SURFACE) {
                eglSwapBuffers(m_Display, m_Surface);
            }
        }

        void Android_Window::onEvent(Event::Event& e) {
        }

	}
}

#endif
