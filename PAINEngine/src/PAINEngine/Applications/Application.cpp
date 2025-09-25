#include "pch.h"
#include "Application.h"

#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Events/Event.h"
//#include "CoreSystems/Audio/Audio.h"
#include "ECS/Controller.h"
#include "CoreSystems/Renderer/TestTriangleLayer.h"
#include "LayeredSystems/LevelEditor/Editor.h"
#include "Audio/AudioManager.h"
#include "CoreSystems/Renderer/RendererLayer.h"
namespace PAIN {

	Application::Application()
	{
		//Create default services
		services = std::make_shared<Services>();
	}

	Application::~Application()
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

	template<typename T>
	void Application::addCoreSystem(std::shared_ptr<T> core_system) {
		core_system->onAttach();
		core_stack.push_back(core_system);
		services->set<T>(core_system);
	}

	template<typename T>
	void Application::addLayerSystem(std::shared_ptr<T> layer_system) {
		layer_system->onAttach();
		layer_stack.push_back(layer_system);
		services->set<T>(layer_system);
	}

	void Application::Init(void* app) {

		auto app_window = std::shared_ptr<Window::Window>(Window::Window::create(app));
		app_window->registerCallbacks(this);
		addCoreSystem(app_window);

		// Create and add the AudioManager to the core systems
		//m_AudioManager = std::make_shared<AudioManager>();
		//addCoreSystem(m_AudioManager);

		//Push other core systems into the stack
		addCoreSystem(std::make_shared<ECS::Controller>());

#ifdef PN_PLATFORM_ANDROID
		auto renderer = std::make_shared<RendererLayer>();
        addCoreSystem(renderer);
#else
		auto renderer = std::make_shared<TestTriangleLayer>();
		addCoreSystem(renderer);
#endif
		//addCoreSystem(std::make_shared<Audio::Controller>());

		//Editor only added when debug mode
#ifdef _DEBUG
		addLayerSystem(std::make_shared<Editor::Editor>(app_window->getNativeWindow()));
#endif

		//Mark engine as ready
		b_app_running = true;
	}

	void Application::Run() {

		//Application loop
		while (b_app_running) {

			//Poll events
			services->get<Window::Window>()->pollEvents();

			//Drain all events in queue
			drainEventQueue();

			//Skip all other systems when window is not active
			if (!services->get<Window::Window>()->getActive()) continue;

			//Update all core systems
			for (auto& core : core_stack) {
				core->onUpdate();
			}

			//Update all layered systems
			for (auto& layer : layer_stack) {
				layer->onUpdate();
			}

			//Swap buffer
			services->get<Window::Window>()->swapBuffers();
		};
	}

	void Application::terminate() {
		b_app_running = false;
	}

	void Application::dispatchEventsForward(Event::Event& e) {
		//Boolean flag for propogation
		bool handled = false;

		//Dispatch to core from bottom up
		for (auto it = core_stack.begin(); it != core_stack.end(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}

		//Check if handled
		if (handled) return;

		//Dispatch to layer bottom up
		for (auto it = layer_stack.begin(); it != layer_stack.end(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}
	}

	void Application::dispatchEventsReversed(Event::Event& e) {
		//Boolean flag for propogation
		bool handled = false;

		//Dispatch to layer top down
		for (auto it = layer_stack.rbegin(); it != layer_stack.rend(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}

		//Check if handled
		if (handled) return;

		//Dispatch to core top down
		for (auto it = core_stack.rbegin(); it != core_stack.rend(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			handled = e.checkHandled();
			if (handled) break;
		}
	}

	void Application::dispatchEvent(Event::Event& e) {

		//Check event type
		if (e.isInCategory(Event::Input)) {

			//Dispatch event in reverse order
			dispatchEventsReversed(e);
		}
		else {
			//Dispatch event in order
			dispatchEventsForward(e);
		}
	}

	void Application::pushEventQueue(std::shared_ptr<Event::Event> e) {
		event_queue.push(e);
	}

	void Application::drainEventQueue() {

		//Handle all events in queue
		while (!event_queue.empty()) {

			//Handle event and pop from queue
			dispatchEvent(*event_queue.front());
			event_queue.pop();
		}
	}
}