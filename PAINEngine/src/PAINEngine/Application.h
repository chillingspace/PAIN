#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Core.h"
#include "Managers/Windows/Window.h"
#include "Managers/Events/Event.h"
#include "ECS/Controller.h"
#include <memory>
#include <vector>

namespace PAIN {

	class PAIN_API Application
	{ 
	private:

		//Main controller of systems
		std::shared_ptr<ECS::Controller> systems_controller;

	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// Defined in client
	Application* CreateApplication();
}

#endif
