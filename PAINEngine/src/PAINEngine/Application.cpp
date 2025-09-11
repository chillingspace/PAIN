#include "pch.h"
#include "Application.h"

namespace PAIN {

	Application::Application()
	{
		//Create window
		app_window = std::shared_ptr<Window::Window>(Window::Window::create());
		app_window->registerCallbacks(this);
		layers.push_back(app_window);
	}

	Application::~Application()
	{
	}

	void Application::Run() {
		while (app_window->onUpdate()) {
		};
	}

	void Application::dispatchToLayers(Event::Event& e) {
		for (auto it = layers.begin(); it != layers.end(); ++it) {
			
			//Dispatch event down layers
			(*it)->OnEvent(e);
			if (e.checkHandled()) break;
		}
	}

	void Application::dispatchToLayersReversed(Event::Event& e) {
		for (auto it = layers.end(); it != layers.begin(); --it) {

			//Dispatch event down layers
			(*it)->OnEvent(e);
			if (e.checkHandled()) break;
		}
	}
}
