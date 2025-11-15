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
                PN_CORE_ERROR("eglGetDisplay failed");
                return false;
            }

            EGLint major, minor;
            if (!eglInitialize(m_Display, &major, &minor)) {
                PN_CORE_ERROR("eglInitialize failed");
                return false;
            }

            b_displayready = true;
            PN_CORE_INFO("EGL initialized: %d.%d", major, minor);
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
                PN_CORE_ERROR("eglChooseConfig failed");
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
                PN_CORE_ERROR("eglCreateContext failed");
                return false;
            }

            b_contextready = true;
            return true;
        }

        bool Android_Window::createSurface() {
            //Init app window
            m_Window = m_App->window;

            if (!m_Window) {
                PN_CORE_ERROR("Cannot initialize Android_Window without native window!");
                return false;
            }

            // Set native window format
            EGLint format = 0;
            eglGetConfigAttrib(m_Display, config, EGL_NATIVE_VISUAL_ID, &format);
            ANativeWindow_setBuffersGeometry(m_Window, 0, 0, format);

            m_Surface = eglCreateWindowSurface(m_Display, config, m_Window, nullptr);
            if (m_Surface == EGL_NO_SURFACE) {
                PN_CORE_ERROR("eglCreateWindowSurface failed");
                return false;
            }

            return true;
        }

        bool Android_Window::makeCurrent() {
            if (!eglMakeCurrent(m_Display, m_Surface, m_Surface, m_Context)) {
                PN_CORE_ERROR("eglMakeCurrent failed");
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
                if (!initDisplay()) PN_CORE_ERROR("DIPSLAY ERROR");
            }
            if (!b_initialized) { 
                if (!setConfig()) PN_CORE_ERROR("CONFIG ERROR");
            }
            if (!b_contextready) {
                if (!createContext()) PN_CORE_ERROR("CONTEXT ERROR");
            }

            //Setup surface
            if (!b_surfaceready) {
                if (!createSurface()) PN_CORE_ERROR("SURFACE ERROR");
                if (!makeCurrent()) PN_CORE_ERROR("CONTEXT ERROR");
                if (!querySurfaceDimensions()) PN_CORE_ERROR("QUERY ERROR");

                frame_buffer.x = ANativeWindow_getWidth(m_Window);
                frame_buffer.y = ANativeWindow_getHeight(m_Window);
                glViewport(0, 0, frame_buffer.x, frame_buffer.y);

                // Enable depth testing
                glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
            }

            //Set window to active
            b_active = true;
        }

        void Android_Window::shutdown() {

            if (b_active) {

                //Terminate all
                if (b_surfaceready)destroySurface();
                if (b_contextready)destroyContext();
                if (b_initialized || b_displayready)terminateDisplay();

                //Ensure proper clearing
                m_Window = nullptr;
                m_App = nullptr;
                m_Display = EGL_NO_DISPLAY;
                config = nullptr;
                m_Surface = EGL_NO_SURFACE;
                m_Context = EGL_NO_CONTEXT;
                PN_CORE_INFO("Android Window shut down");

                b_active = false;
            }
        }

        int32_t Android_Window::handle_input(android_app* app, AInputEvent* event)
        {
            auto* package = static_cast<EventsPackage*>(app->userData);
            auto* e_app = static_cast<PAIN::Application*>(package->app);

            // Always enqueue a catch-all (useful for debugging/raw access)
            e_app->pushEventQueue(std::make_shared<Event::AllEvent>(event));

            const int32_t type = AInputEvent_getType(event);
            if (type == AINPUT_EVENT_TYPE_MOTION)
            {
                const int32_t action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
                const float x = AMotionEvent_getX(event, 0);
                const float y = AMotionEvent_getY(event, 0);
                const int32_t pointerId = AMotionEvent_getPointerId(event, 0);

                switch (action) {
                case AMOTION_EVENT_ACTION_DOWN:
                case AMOTION_EVENT_ACTION_POINTER_DOWN:
                    e_app->pushEventQueue(std::make_shared<Event::TouchDown>(x, y, pointerId));
                    break;
                case AMOTION_EVENT_ACTION_UP:
                case AMOTION_EVENT_ACTION_POINTER_UP:
                    e_app->pushEventQueue(std::make_shared<Event::TouchUp>(x, y, pointerId));
                    break;
                case AMOTION_EVENT_ACTION_MOVE:
                    e_app->pushEventQueue(std::make_shared<Event::TouchMove>(x, y, pointerId));
                    break;
                case AMOTION_EVENT_ACTION_CANCEL:
                    e_app->pushEventQueue(std::make_shared<Event::TouchCancel>(x, y, pointerId));
                    break;
                default:
                    // Unrecognized motion action; keep the AllEvent
                    break;
                }
            }
            else if (type == AINPUT_EVENT_TYPE_KEY)
            {
                // No dedicated key event class provided; keep raw for now
                // The AllEvent already covers this case
            }

            return 0; // not consumed here
        }


        void Android_Window::handle_cmd(android_app* app, int32_t cmd) {
            auto* package = static_cast<EventsPackage*>(app->userData);
            auto* e_app = static_cast<PAIN::Application*>(package->app);
            auto* e_window = static_cast<Android_Window*>(package->window);

            // Optional: generic catch-all for diagnostics
            // e_app->pushEventQueue(std::make_shared<Event::AllEvent>(cmd));

            switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                e_window->init();
                e_app->pushEventQueue(std::make_shared<Event::SurfaceCreated>(e_window->getNativeWindow()));
                break;

            case APP_CMD_TERM_WINDOW:
                e_window->destroySurface();
                e_app->pushEventQueue(std::make_shared<Event::SurfaceDestroyed>(e_window->getNativeWindow()));
                break;

            case APP_CMD_WINDOW_RESIZED: {
                // If you track framebuffer size in e_window, pass the latest values
                const EGLint w = e_window->frame_buffer.x; // or query via eglQuerySurface
                const EGLint h = e_window->frame_buffer.y;
                e_app->pushEventQueue(std::make_shared<Event::SurfaceChanged>(e_window->getNativeWindow(), w, h));
                break;
            }

            case APP_CMD_GAINED_FOCUS:
                e_app->pushEventQueue(std::make_shared<Event::FocusGained>());
                break;

            case APP_CMD_LOST_FOCUS:
                e_app->pushEventQueue(std::make_shared<Event::FocusLost>());
                break;

            case APP_CMD_PAUSE:
                e_app->pushEventQueue(std::make_shared<Event::AppPause>());
                break;

            case APP_CMD_RESUME:
                e_app->pushEventQueue(std::make_shared<Event::AppResume>());
                break;

            case APP_CMD_START:
                e_app->pushEventQueue(std::make_shared<Event::AppStart>());
                break;

            case APP_CMD_STOP:
                e_app->pushEventQueue(std::make_shared<Event::AppStop>());
                break;

            case APP_CMD_CONFIG_CHANGED:
                e_app->pushEventQueue(std::make_shared<Event::ConfigurationChanged>());
                break;

            case APP_CMD_LOW_MEMORY:
                e_app->pushEventQueue(std::make_shared<Event::LowMemory>());
                break;

            case APP_CMD_DESTROY:
                e_app->pushEventQueue(std::make_shared<Event::AppDestroy>());
                e_app->terminate();
                break;

            default:
                // If you have an OtherCmd event, you can push it here
                // e_app->pushEventQueue(std::make_shared<Event::OtherCmd>(cmd));
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

        void Android_Window::onUpdate(AppTiming timing) {

        }

        void Android_Window::onEvent(Event::Event& e) {
        }

        glm::uvec2 Android_Window::getFrameBuffer() const {
            return frame_buffer;
        }

        void Android_Window::safeShutdown() {

        }
	}
}

#endif
