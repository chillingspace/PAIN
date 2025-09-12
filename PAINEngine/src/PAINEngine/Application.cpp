#include "pch.h"
#include "Application.h"

namespace PAIN {

	Application::Application()
	{
		//Create systems controller
		systems_controller = std::make_shared<ECS::Controller>();

		//Create window
		auto app_window = std::shared_ptr<Window::Window>(Window::Window::create());
		app_window->registerCallbacks(systems_controller.get());

		//Push into systems controller
		systems_controller->addSystems(app_window);
	}

	Application::~Application()
	{
	}

	void Application::Run() {

		//Application loop
		while (systems_controller->checkAppRunning()) {

			//systems controller update func
			systems_controller->onUpdate();
		};
	}
}
