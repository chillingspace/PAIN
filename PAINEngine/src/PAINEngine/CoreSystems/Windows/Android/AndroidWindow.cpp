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

            PN_CORE_INFO("EGL initialized: version %d.%d", major, minor);
            b_displayready = true;
            return true;
        }

        bool Android_Window::setConfig() {
            const EGLint attribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_DEPTH_SIZE, 24,
                EGL_STENCIL_SIZE, 8,
                EGL_NONE
            };

            EGLint num_configs;
            if (!eglChooseConfig(m_Display, attribs, &config, 1, &num_configs)) {
                PN_CORE_ERROR("eglChooseConfig failed");
                return false;
            }

            if (num_configs == 0) {
                PN_CORE_ERROR("No matching EGL configs found");
                return false;
            }

            PN_CORE_INFO("EGL config set successfully");
            return true;
        }

        bool Android_Window::createContext() {
            const EGLint context_attribs[] = {
                EGL_CONTEXT_CLIENT_VERSION, 3,  // OpenGL ES 3.0
                EGL_NONE
            };

            m_Context = eglCreateContext(m_Display, config, EGL_NO_CONTEXT, context_attribs);

            if (m_Context == EGL_NO_CONTEXT) {
                PN_CORE_ERROR("eglCreateContext failed");
                return false;
            }

            PN_CORE_INFO("EGL context created successfully");
            b_contextready = true;
            return true;
        }

        bool Android_Window::createSurface() {
            if (!m_App->window) {
                PN_CORE_ERROR("Native window is null, cannot create surface");
                return false;
            }

            m_Window = m_App->window;

            // Set native window buffer format
            EGLint format;
            eglGetConfigAttrib(m_Display, config, EGL_NATIVE_VISUAL_ID, &format);
            ANativeWindow_setBuffersGeometry(m_Window, 0, 0, format);

            // Create window surface
            m_Surface = eglCreateWindowSurface(m_Display, config, m_Window, nullptr);

            if (m_Surface == EGL_NO_SURFACE) {
                PN_CORE_ERROR("eglCreateWindowSurface failed");
                return false;
            }

            PN_CORE_INFO("EGL surface created successfully");
            b_surfaceready = true;

            // Query actual surface dimensions
            if (!querySurfaceDimensions()) {
                PN_CORE_ERROR("Failed to query surface dimensions");
                return false;
            }

            // Make context current
            if (!makeCurrent()) {
                PN_CORE_ERROR("Failed to make context current");
                return false;
            }

            // Set viewport
            glViewport(0, 0, frame_buffer.x, frame_buffer.y);

            // Mark window as active
            b_active = true;

            PN_CORE_INFO("Window surface ready and active");
            return true;
        }

        bool Android_Window::makeCurrent() {
            if (!eglMakeCurrent(m_Display, m_Surface, m_Surface, m_Context)) {
                PN_CORE_ERROR("eglMakeCurrent failed");
                return false;
            }

            PN_CORE_INFO("EGL context made current");
            return true;
        }

        bool Android_Window::querySurfaceDimensions() {
            EGLint width, height;

            if (!eglQuerySurface(m_Display, m_Surface, EGL_WIDTH, &width)) {
                PN_CORE_ERROR("Failed to query surface width");
                return false;
            }

            if (!eglQuerySurface(m_Display, m_Surface, EGL_HEIGHT, &height)) {
                PN_CORE_ERROR("Failed to query surface height");
                return false;
            }

            frame_buffer.x = width;
            frame_buffer.y = height;

            PN_CORE_INFO("Surface dimensions: %dx%d", width, height);
            return true;
        }

        void Android_Window::destroySurface() {
            if (m_Surface != EGL_NO_SURFACE) {
                eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                eglDestroySurface(m_Display, m_Surface);
                m_Surface = EGL_NO_SURFACE;
                b_surfaceready = false;
                b_active = false;
                PN_CORE_INFO("EGL surface destroyed");
            }
        }

        void Android_Window::destroyContext() {
            if (m_Context != EGL_NO_CONTEXT) {
                eglDestroyContext(m_Display, m_Context);
                m_Context = EGL_NO_CONTEXT;
                b_contextready = false;
                PN_CORE_INFO("EGL context destroyed");
            }
        }

        void Android_Window::terminateDisplay() {
            if (m_Display != EGL_NO_DISPLAY) {
                eglTerminate(m_Display);
                m_Display = EGL_NO_DISPLAY;
                b_displayready = false;
                PN_CORE_INFO("EGL display terminated");
            }
        }

        Window* Window::create(void* app, Package const& package) {
            return new Android_Window(app, package);
        }

        Android_Window::Android_Window(void* app, Package const& package) {
            m_App = static_cast<android_app*>(app);

            if (!m_App) {
                PN_CORE_ERROR("android_app* is null!");
                throw std::runtime_error("Android app pointer is null");
            }

            // Store package info
            frame_buffer.x = package.width;
            frame_buffer.y = package.height;

            // Initialize window
            init();
        }

        Android_Window::~Android_Window() {
            shutdown();
        }

        void Android_Window::init() {
            PN_CORE_INFO("Initializing Android Window");

            // Initialize EGL display
            if (!initDisplay()) {
                PN_CORE_ERROR("Failed to initialize EGL display");
                return;
            }

            // Set EGL configuration
            if (!setConfig()) {
                PN_CORE_ERROR("Failed to set EGL config");
                return;
            }

            // Create EGL context
            if (!createContext()) {
                PN_CORE_ERROR("Failed to create EGL context");
                return;
            }

            // Note: Surface creation happens in handle_cmd when APP_CMD_INIT_WINDOW arrives
            // This is because m_App->window might not be ready yet

            b_initialized = true;
            PN_CORE_INFO("Android Window initialized successfully");
        }

        void Android_Window::shutdown() {

            if (!b_initialized) return;

            PN_CORE_INFO("Shutting down Android Window");

            destroySurface();
            destroyContext();
            terminateDisplay();

            b_initialized = false;
            b_active = false;

            PN_CORE_INFO("Android Window shutdown complete");
        }

        int32_t Android_Window::handle_input(android_app* app, AInputEvent* event)
        {
            auto* package = static_cast<EventsPackage*>(app->userData);
            auto* e_app = static_cast<PAIN::Application*>(package->app);

            if (!e_app || !event) {
                return 0;
            }

            int32_t event_type = AInputEvent_getType(event);

            // Handle Motion Events (Touch)
            if (event_type == AINPUT_EVENT_TYPE_MOTION) {
                int32_t action = AMotionEvent_getAction(event);
                int32_t action_code = action & AMOTION_EVENT_ACTION_MASK;
                size_t pointer_index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                    >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

                // Get pointer count
                size_t pointer_count = AMotionEvent_getPointerCount(event);
                if (pointer_count == 0) {
                    return 0;
                }

                // For multi-touch events (POINTER_DOWN/UP), use the specific pointer index
                // For single touch events (DOWN/UP/MOVE), use index 0
                if (action_code != AMOTION_EVENT_ACTION_POINTER_DOWN &&
                    action_code != AMOTION_EVENT_ACTION_POINTER_UP) {
                    pointer_index = 0;
                }

                // Ensure pointer_index is valid
                if (pointer_index >= pointer_count) {
                    pointer_index = 0;
                }

                // Extract touch data
                float x = AMotionEvent_getX(event, pointer_index);
                float y = AMotionEvent_getY(event, pointer_index);
                int32_t pointer_id = AMotionEvent_getPointerId(event, pointer_index);

                // Dispatch based on action
                switch (action_code) {
                case AMOTION_EVENT_ACTION_DOWN:
                case AMOTION_EVENT_ACTION_POINTER_DOWN:
                    e_app->pushEventQueue(std::make_shared<Event::TouchDown>(x, y, pointer_id));
                    return 1; // Event consumed

                case AMOTION_EVENT_ACTION_UP:
                case AMOTION_EVENT_ACTION_POINTER_UP:
                    e_app->pushEventQueue(std::make_shared<Event::TouchUp>(x, y, pointer_id));
                    return 1; // Event consumed

                case AMOTION_EVENT_ACTION_MOVE:
                    // Move events contain data for all active pointers
                    for (size_t i = 0; i < pointer_count; ++i) {
                        float move_x = AMotionEvent_getX(event, i);
                        float move_y = AMotionEvent_getY(event, i);
                        int32_t move_id = AMotionEvent_getPointerId(event, i);
                        e_app->pushEventQueue(std::make_shared<Event::TouchMove>(move_x, move_y, move_id));
                    }
                    return 1; // Event consumed

                case AMOTION_EVENT_ACTION_CANCEL:
                    e_app->pushEventQueue(std::make_shared<Event::TouchCancel>(x, y, pointer_id));
                    return 1; // Event consumed

                default:
                    // Unknown motion action
                    return 0;
                }
            }

            // Handle Key Events
            else if (event_type == AINPUT_EVENT_TYPE_KEY) {
                int32_t key_code = AKeyEvent_getKeyCode(event);
                int32_t action = AKeyEvent_getAction(event);
                int32_t meta_state = AKeyEvent_getMetaState(event);

                // Special handling for back button
                if (key_code == AKEYCODE_BACK) {
                    // Only dispatch on key up to avoid duplicates
                    if (action == AKEY_EVENT_ACTION_UP) {
                        e_app->pushEventQueue(std::make_shared<Event::BackButton>());
                    }
                    return 1; // Consume back button to prevent default app termination
                }

                // Handle other key events
                if (action == AKEY_EVENT_ACTION_DOWN) {
                    e_app->pushEventQueue(std::make_shared<Event::AndroidKeyDown>(key_code, meta_state));
                    return 1; // Event consumed
                }
                else if (action == AKEY_EVENT_ACTION_UP) {
                    e_app->pushEventQueue(std::make_shared<Event::AndroidKeyUp>(key_code, meta_state));
                    return 1; // Event consumed
                }
            }

            return 0; // Event not handled
        }


        void Android_Window::handle_cmd(android_app* app, int32_t cmd) {
            auto* package = static_cast<EventsPackage*>(app->userData);
            auto* e_app = static_cast<PAIN::Application*>(package->app);
            auto* e_window = static_cast<Android_Window*>(package->window);

            // Optional: generic catch-all for diagnostics
            // e_app->pushEventQueue(std::make_shared<Event::AllEvent>(cmd));

            switch (cmd) {
            case APP_CMD_INIT_WINDOW:
                PN_CORE_INFO("APP_CMD_INIT_WINDOW received");
                if (e_window) {
                    e_window->createSurface();

                    // Dispatch window resize event
                    if (e_app) {
                        e_app->pushEventQueue(std::make_shared<Event::SurfaceCreated>(
                            e_window->getNativeWindow(), 
                            e_window->frame_buffer.x, 
                            e_window->frame_buffer.y));
                    }
                }
                break;

            case APP_CMD_TERM_WINDOW:
                PN_CORE_INFO("APP_CMD_TERM_WINDOW received");
                e_window->destroySurface();
                if (e_app) {
                    e_app->pushEventQueue(std::make_shared<Event::SurfaceDestroyed>(e_window->getNativeWindow()));
                }
                break;

            case APP_CMD_WINDOW_RESIZED: {
                PN_CORE_INFO("APP_CMD_WINDOW_RESIZED received");
                if (e_window->b_surfaceready) {
                    e_window->querySurfaceDimensions();

                    // Dispatch window resize event
                    if (e_app) {
                        e_app->pushEventQueue(std::make_shared<Event::SurfaceChanged>(e_window->getNativeWindow(), e_window->frame_buffer.x, e_window->frame_buffer.y));
                    }
                }
                break;
            }

            case APP_CMD_GAINED_FOCUS:
                PN_CORE_INFO("APP_CMD_GAINED_FOCUS received");
                e_window->b_active = true;

                // Dispatch focus event
                if (e_app) {
                    e_app->pushEventQueue(std::make_shared<Event::FocusGained>());
                }
                break;

            case APP_CMD_LOST_FOCUS:
                PN_CORE_INFO("APP_CMD_LOST_FOCUS received");
                e_window->b_active = false;

                // Dispatch focus event
                if (e_app) {
                    e_app->pushEventQueue(std::make_shared<Event::FocusLost>());
                }
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
            m_EventPackage = EventsPackage{ this, app };
            m_App->userData = &m_EventPackage;

            //Register callbacks
            m_App->onAppCmd = handle_cmd;
            m_App->onInputEvent = handle_input;

            PN_CORE_INFO("Android callbacks registered");

            //Poll events under windows is ready and opengl is ready
            while (!b_surfaceready && !b_active) {
                pollEvents();
            }
        }

        void Android_Window::pollEvents() {

            int events;
            android_poll_source* source;

            // Poll with timeout of 0 = non-blocking
            // If window is not active, we could use -1 (blocking) to save battery
            while (ALooper_pollOnce(b_active ? 0 : -1, nullptr, &events, (void**)&source) >= 0) {
                if (source) {
                    source->process(m_App, source);
                }

                // Check if app is being destroyed
                if (m_App->destroyRequested) {
                    safeShutdown();
                    break;
                }
            }
        }

        void Android_Window::swapBuffers() {
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
            //auto* pain_app = static_cast<PAIN::Application*>(m_EventPackage.app);
            //if (pain_app) {
            //    pain_app->terminate();
            //}
            //else {
            //    shutdown();
            //}
            if (m_App && m_App->activity) {
                // This is the proper way to close an Android app
                ANativeActivity_finish(m_App->activity);
            }
        }

        bool Android_Window::isMinimized() const {
            return !b_active || !b_surfaceready;
        }

        void Android_Window::hideCursor(bool hidden) {

        }
	}
}

#endif
