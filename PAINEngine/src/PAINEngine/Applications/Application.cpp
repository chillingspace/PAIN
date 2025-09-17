#include "pch.h"
#include "Application.h"
#include "CoreSystems/Events/Event.h"
#include "ECS/Controller.h"
#include "CoreSystems/Renderer/TestTriangleLayer.h"
#include "LayeredSystems/LevelEditor/Editor.h"

#if defined(PN_PLATFORM_WINDOWS)
#include "CoreSystems/Windows/GLFW/GLFWWindow.h"
#endif

namespace PAIN {

    Application* Application::s_Instance = nullptr;

    Application::Application()
    {
        s_Instance = this;
    }

    Application::~Application()
    {
        Shutdown();
    }

    void Application::InitializeDesktop()
    {
        #if defined(PN_PLATFORM_WINDOWS)
            m_Window = std::unique_ptr<Window::Window>(Window::Window::create());
            m_Window->registerCallbacks(this);
            core_stack.push_back(std::shared_ptr<Window::Window>(m_Window.get(), [](void*){}));
        #endif

        m_AudioManager = std::make_shared<AudioManager>();
        addCoreSystem(m_AudioManager);
        // addCoreSystem(std::make_shared<ECS::Controller>()); // Temporarily disabled
        addCoreSystem(std::make_shared<TestTriangleLayer>());

        // #if defined(_DEBUG) && defined(PN_PLATFORM_WINDOWS)
        //     addLayerSystem(std::make_shared<Editor::Editor>()); // Temporarily disabled
        // #endif
    }

    
    #if defined(PLATFORM_ANDROID)
    void Application::InitializeAndroid(AAssetManager* assetManager)
    {
        m_AudioManager = std::make_shared<AudioManager>();
        // TODO: Pass assetManager to AudioManager
        addCoreSystem(m_AudioManager);
        // addCoreSystem(std::make_shared<ECS::Controller>()); // Temporarily disabled
        addCoreSystem(std::make_shared<TestTriangleLayer>());
    }
    #endif

    void Application::addCoreSystem(std::shared_ptr<AppSystem> core_system) {
        core_system->onAttach();
        core_stack.push_back(core_system);
    }

    void Application::addLayerSystem(std::shared_ptr<AppSystem> layer_system) {
        layer_system->onAttach();
        layer_stack.push_back(layer_system);
    }

    void Application::Run() {
        #if defined(PN_PLATFORM_WINDOWS)
            while (b_app_running) {
                Update();
            }
        #endif
    }

    void Application::Update()
    {
        drainEventQueue();
        for (auto& layer : layer_stack) {
            layer->onUpdate();
        }
        for (auto& core : core_stack) {
            core->onUpdate();
        }
    }
    
    void Application::Shutdown()
    {
        for (auto& layer : layer_stack) {
            layer->onDetach();
        }
        layer_stack.clear();
        for (auto& core : core_stack) {
            core->onDetach();
        }
        core_stack.clear();
    }
    
    // ... (rest of the event dispatching functions remain the same) ...
	void Application::dispatchEventsForward(Event::Event& e) {
		bool handled = false;
		for (auto it = core_stack.begin(); it != core_stack.end(); ++it) {
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}
		if (handled) return;
		for (auto it = layer_stack.begin(); it != layer_stack.end(); ++it) {
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}
	}

	void Application::dispatchEventsReversed(Event::Event& e) {
		bool handled = false;
		for (auto it = layer_stack.rbegin(); it != layer_stack.rend(); ++it) {
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}
		if (handled) return;
		for (auto it = core_stack.rbegin(); it != core_stack.rend(); ++it) {
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}
	}

	void Application::dispatchEvent(Event::Event& e) {
		if (e.isInCategory(Event::Input)) {
			dispatchEventsReversed(e);
		}
		else {
			dispatchEventsForward(e);
		}
	}

	void Application::pushEventQueue(std::shared_ptr<Event::Event> e) {
		event_queue.push(e);
	}

	void Application::drainEventQueue() {
		while (!event_queue.empty()) {
			dispatchEvent(*event_queue.front());
			event_queue.pop();
		}
	}
}