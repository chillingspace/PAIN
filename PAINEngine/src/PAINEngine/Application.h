#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Core.h"
#include "Windows/Window.h"
#include "Events/Event.h"
#include <memory>
#include <vector>

namespace PAIN {

	class AppLayer {
	private:
	public:

		//Event handler for app layer
		virtual void OnEvent(Event::Event& e) = 0;
	};

	class PAIN_API Application
	{ 
	private:

		//Application window
		std::unique_ptr<Window::Window> app_window;

		//App layers
		std::vector<std::shared_ptr<AppLayer>> layers;

	public:
		Application();
		virtual ~Application();

		void Run();

		void dispatchToLayers(Event::Event& e);

		void dispatchToLayersReversed(Event::Event& e);
	};

	// Defined in client
	Application* CreateApplication();


}

#endif
