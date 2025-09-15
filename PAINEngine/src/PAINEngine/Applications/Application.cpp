#include "pch.h"
#include "Application.h"

#include "Managers/Windows/Window.h"
#include "Managers/Events/Event.h"
#include "ECS/Controller.h"
#include "Renderer/TestTriangleLayer.h"

namespace PAIN {

	Application::Application()
	{
		//Create systems controller
		auto systems_controller = std::make_shared<ECS::Controller>();

		//Create window
		auto app_window = std::shared_ptr<Window::Window>(Window::Window::create());
		app_window->registerCallbacks(this);

		//Push itno the core systems stack
		core_stack.push_back(app_window);
		core_stack.push_back(systems_controller);
		core_stack.push_back(std::make_shared<PAIN::TestTriangleLayer>());

		//Push into the layered systems stack

	}

	Application::~Application()
	{
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
