#include "pch.h"
#include "Application.h"

namespace PAIN {

	Application::Application()
	{
		//Create window
		app_window = std::shared_ptr<Window::Window>(Window::Window::create());
		app_window->registerCallbacks(this);
		layer_stack.push_back(app_window);
	}

	Application::~Application()
	{
	}

	void Application::Run() {

		//Application loop
		while (b_running) {

			//Iterate through all systems
			for (auto& system : layer_stack) {
				system->onUpdate();
			}
		};
	}

	void Application::Terminate() {
		b_running = false;
	}

	void Application::dispatchToLayers(Event::Event& e) {
		for (auto it = layer_stack.begin(); it != layer_stack.end(); ++it) {
			
			//Dispatch event down layers
			(*it)->onEvent(e);
			if (e.checkHandled()) break;
		}
	}

	void Application::dispatchToLayersReversed(Event::Event& e) {
		for (auto it = layer_stack.rbegin(); it != layer_stack.rend(); ++it) {

			//Dispatch event down layers
			(*it)->onEvent(e);
			if (e.checkHandled()) break;
		}
	}
}
