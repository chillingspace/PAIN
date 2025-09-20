#include "pch.h"
#include "Application.h"

#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Events/Event.h"
//#include "CoreSystems/Audio/Audio.h"
#include "ECS/Controller.h"
#include "CoreSystems/Renderer/TestTriangleLayer.h"
#include "LayeredSystems/LevelEditor/Editor.h"
#include "PAINEngine/Audio/AudioManager.h"

namespace PAIN {

	// Define the static instance
	Application* Application::s_Instance = nullptr;

	Application::Application()
	{
		// Set the static instance
		s_Instance = this;

		auto window_app = std::shared_ptr<Window::Window>(Window::Window::create());
		window_app->registerCallbacks(this);

		// Create and add the AudioManager to the core systems
		//m_AudioManager = std::make_shared<AudioManager>();
		//addCoreSystem(m_AudioManager);

		//Push other core systems into the stack
		addCoreSystem(window_app);
		addCoreSystem(std::make_shared<ECS::Controller>());
		addCoreSystem(std::make_shared<TestTriangleLayer>());
		//addCoreSystem(std::make_shared<Audio::Controller>());

		//Editor only added when debug mode
#ifdef _DEBUG
		addLayerSystem(std::make_shared<Editor::Editor>());
#endif
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

	void Application::addCoreSystem(std::shared_ptr<AppSystem> core_system) {
		core_system->onAttach();
		core_stack.push_back(core_system);
	}

	void Application::addLayerSystem(std::shared_ptr<AppSystem> layer_system) {
		layer_system->onAttach();
		layer_stack.push_back(layer_system);
	}

	void Application::Run() {

		//Application loop
		while (b_app_running) {

			//Drain all events in queue
			drainEventQueue();

			//Update all layered systems
			for (auto& layer : layer_stack) {
				layer->onUpdate();
			}

			//Update all core systems
			for (auto& core : core_stack) {
				core->onUpdate();
			}
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