#include <CoreSystems/Events/Android/OtherEvents.h>
#include "pch.h"

#ifdef PN_PLATFORM_ANDROID

#include "AndroidWindow.h"

#include "CoreSystems/Events/Android/AppEvents.h"
#include "CoreSystems/Events/Android/FocusEvents.h"
#include "CoreSystems/Events/Android/OtherEvents.h"
#include "CoreSystems/Events/Android/SurfaceEvents.h"
#include "CoreSystems/Events/Android/TouchEvents.h"

#include "Applications/Application.h"

namespace PAIN {
	namespace Window {

        Window* Window::create(void* app, Package const& package) {
            return new Android_Window(app, package);
        }

        Android_Window::Android_Window(void* app, Package const& package) {
            m_App = static_cast<android_app*>(app);
        }

        Android_Window::~Android_Window() {
            shutdown();
        }

        bool Android_Window::initDisplay() {
            m_Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (m_Display == EGL_NO_DISPLAY) {
                LOGE("eglGetDisplay failed");
                return false;
            }

            EGLint major, minor;
            if (!eglInitialize(m_Display, &major, &minor)) {
                LOGE("eglInitialize failed");
                return false;
            }

            b_displayready = true;
            LOGI("EGL initialized: %d.%d", major, minor);
            return true;
        }

        bool Android_Window::setConfig() {
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

            EGLint numConfigs;
            if (!eglChooseConfig(m_Display, attribs, &config, 1, &numConfigs)) {
                LOGE("eglChooseConfig failed");
                return false;
            }

            b_initialized = true;
            return true;
        }

        bool Android_Window::createContext() {
            const EGLint ctxAttribs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 3,
                EGL_NONE
            };

            m_Context = eglCreateContext(m_Display, config, EGL_NO_CONTEXT, ctxAttribs);
            if (m_Context == EGL_NO_CONTEXT) {
                LOGE("eglCreateContext failed");
                return false;
            }

            b_contextready = true;
            return true;
        }

        bool Android_Window::createSurface() {
            //Init app window
            m_Window = m_App->window;

            if (!m_Window) {
                LOGE("Cannot initialize Android_Window without native window!");
                return false;
            }

            // Set native window format
            EGLint format = 0;
            eglGetConfigAttrib(m_Display, config, EGL_NATIVE_VISUAL_ID, &format);
            ANativeWindow_setBuffersGeometry(m_Window, 0, 0, format);

            m_Surface = eglCreateWindowSurface(m_Display, config, m_Window, nullptr);
            if (m_Surface == EGL_NO_SURFACE) {
                LOGE("eglCreateWindowSurface failed");
                return false;
            }

            return true;
        }

        bool Android_Window::makeCurrent() {
            if (!eglMakeCurrent(m_Display, m_Surface, m_Surface, m_Context)) {
                LOGE("eglMakeCurrent failed");
                return false;
            }

            b_surfaceready = true;
            return true;
        }

        bool Android_Window::querySurfaceDimensions() {
            // Query actual surface size
            EGLint width, height;
            eglQuerySurface(m_Display, m_Surface, EGL_WIDTH, &width);
            eglQuerySurface(m_Display, m_Surface, EGL_HEIGHT, &height);
            frame_buffer.x = width;
            frame_buffer.y = height;

            return (width > 0 && height > 0);
        }

        void Android_Window::destroySurface() {
            if (m_Surface != EGL_NO_SURFACE) {
                eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, m_Context);
                eglDestroySurface(m_Display, m_Surface);
                m_Surface = EGL_NO_SURFACE;
            }
            m_Window = nullptr;
            b_surfaceready = false;
            b_active = false;
        }

        void Android_Window::destroyContext() {
            if (m_Context != EGL_NO_CONTEXT) {
                eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                eglDestroyContext(m_Display, m_Context);
                m_Context = EGL_NO_CONTEXT;
            }
            b_contextready = false;
            b_active = false;
        }

        void Android_Window::terminateDisplay() {
            if (m_Display != EGL_NO_DISPLAY) {
                eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                eglTerminate(m_Display);
                m_Display = EGL_NO_DISPLAY;
            }
            b_displayready = false;
            b_initialized = false;
            b_active = false;
        }

        void Android_Window::init() {

            //Setup windows
            if (!b_displayready) {
                if (!initDisplay()) LOGE("DIPSLAY ERROR");
            }
            if (!b_initialized) { 
                if (!setConfig()) LOGE("CONFIG ERROR");
            }
            if (!b_contextready) {
                if (!createContext()) LOGE("CONTEXT ERROR");
            }

            //Setup surface
            if (!b_surfaceready) {
                if (!createSurface()) LOGE("SURFACE ERROR");
                if (!makeCurrent()) LOGE("CONTEXT ERROR");
                if (!querySurfaceDimensions()) LOGE("QUERY ERROR");

                // Set viewport
                glViewport(0, 0, frame_buffer.x, frame_buffer.y);

                // Enable depth testing
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
            }

            //Set window to active
            b_active = true;
        }

        void Android_Window::shutdown() {

            //Terminate all
            if(b_surfaceready)destroySurface();
            if(b_contextready)destroyContext();
            if(b_initialized || b_displayready)terminateDisplay();

            //Ensure proper clearing
            m_Window = nullptr;
            m_App = nullptr;
            m_Display = EGL_NO_DISPLAY;
            config = nullptr;
            m_Surface = EGL_NO_SURFACE;
            m_Context = EGL_NO_CONTEXT;
            LOGI("Android Window shut down");
        }

        int32_t Android_Window::handle_input(android_app* app, AInputEvent* event)
        {
            EventsPackage* package = (EventsPackage*)app->userData;
            PAIN::Application* e_app = (PAIN::Application*)package->app;

            //Dispatch all events
            e_app->pushEventQueue(std::make_shared<Event::AllEvent>(event));
            
            //// Let ImGui consume it first
            //if (ImGui_ImplAndroid_HandleInputEvent(event))
            //    return 1; // handled

            //// �your own game/editor handling�
            return 0;
        }

        void Android_Window::handle_cmd(android_app* app, int32_t cmd) {
            EventsPackage* package = (EventsPackage*)app->userData;
            PAIN::Application* e_app = (PAIN::Application*)package->app;
            Android_Window* e_window = (Android_Window*)package->window;

            switch (cmd) {
            case APP_CMD_INIT_WINDOW:

                //Initialize window
                e_window->init();

                //Dispatch events

                break;
            case APP_CMD_TERM_WINDOW:

                //Destroy window surface
                e_window->destroySurface();

                //Dispatch events
                break;
            case APP_CMD_GAINED_FOCUS:
                //Dispatch events
                break;

            case APP_CMD_LOST_FOCUS:
                //Dispatch events
                break;

            case APP_CMD_PAUSE:
                //Dispatch events
                break;

            case APP_CMD_RESUME:
                //Dispatch events
                break;

            case APP_CMD_DESTROY:

                //Shutdown window
                e_window->shutdown();

                //Signal app to terminate as well
                e_app->terminate();
                break;
            default:
                break;
            }
        }

        void Android_Window::registerCallbacks(void* app) {

            //Create events package for android app handled
            m_EventPackage = EventsPackage{ this, m_App->userData };
            m_App->userData = &m_EventPackage;

            //Register callbacks
            m_App->onAppCmd = handle_cmd;
            m_App->onInputEvent = handle_input;

            //Wait for window to fully init
            while (!b_active) {
                int events;
                android_poll_source* source;

                //Process events waiting for game to init
                while (ALooper_pollOnce(-1, nullptr, &events,
                    (void**)&source) >= 0) {
                    if (source) {
                        source->process(m_App, source);
                    }

                    if (b_active) break;
                }
            }
        }

        void Android_Window::pollEvents() {

            int events;
            android_poll_source* source;

            // Process all pending events
            while (ALooper_pollOnce(b_active ? 0 : -1, nullptr, &events,
                (void**)&source) >= 0) {
                if (source) {
                    source->process(m_App, source);
                }
            }
        }

        void Android_Window::swapBuffers() {

            //Swap buffers
            if (m_Display != EGL_NO_DISPLAY && m_Surface != EGL_NO_SURFACE) {
                eglSwapBuffers(m_Display, m_Surface);
            }
        }

        void Android_Window::onUpdate(float dt) {

        }

        void Android_Window::onEvent(Event::Event& e) {
        }
	}
}

#endif
