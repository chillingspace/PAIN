#include "pch.h"
#include "Application.h"

namespace PAIN {

	Application::Application()
	{
		//Create window
		app_window = std::unique_ptr<Window::Window>(Window::Window::create());
	}

	Application::~Application()
	{
	}

	void Application::Run() {
		while (app_window->onUpdate()) {
		};
	}
}
