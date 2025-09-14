#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "AppLayer.h"

#include "Managers/Windows/Window.h"
#include "Managers/Events/Event.h"
#include "ECS/Controller.h"
#include <memory>
#include <vector>

namespace PAIN {

	class Application
	{ 
	private:

		//Create app layer stack
		std::shared_ptr<AppLayerStack> layer_stack;

	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// Defined in client
	Application* CreateApplication();
}

#endif
