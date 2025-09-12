#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Core.h"
#include "Managers/Windows/Window.h"
#include "Managers/Events/Event.h"
#include <memory>
#include <vector>

namespace PAIN {

	class PAIN_API Application
	{ 
	private:

		//Application window
		std::shared_ptr<Window::Window> app_window;

		//App layers
		std::vector<std::shared_ptr<ECS::ISystem>> layer_stack;

		//Boolean for running app
		bool b_running = true;

	public:
		Application();
		virtual ~Application();

		void Run();

		void Terminate();

		void dispatchToLayers(Event::Event& e);

		void dispatchToLayersReversed(Event::Event& e);
	};

	// Defined in client
	Application* CreateApplication();
}

#endif
