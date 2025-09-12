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
		while (app_window->onUpdate()) {
		};
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
