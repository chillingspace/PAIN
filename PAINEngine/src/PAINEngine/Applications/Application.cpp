#include "pch.h"
#include "Application.h"

#include "Managers/Windows/Window.h"
#include "Managers/Events/Event.h"
#include "ECS/Controller.h"
#include "Renderer/TestTriangleLayer.h"

namespace PAIN {

	Application::Application()
	{
		//Create layer stack
		layer_stack = std::make_shared<AppLayerStack>();

		//Create systems controller
		auto systems_controller = std::make_shared<ECS::Controller>();

		//Create window
		auto app_window = std::shared_ptr<Window::Window>(Window::Window::create());
		app_window->registerCallbacks(layer_stack.get());

		//Push into systems controller
		layer_stack->addLayer(app_window);
		layer_stack->addLayer(systems_controller);
		layer_stack->addLayer(std::make_shared<PAIN::TestTriangleLayer>());
	}

	Application::~Application()
	{
	}

	void Application::Run() {

		//Application loop
		while (layer_stack->checkAppRunning()) {

			//systems controller update func
			layer_stack->onUpdate();
		};
	}
}
