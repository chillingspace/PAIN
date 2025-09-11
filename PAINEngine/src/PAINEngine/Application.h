#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "Core.h"
#include "Windows/Window.h"
#include <memory>

namespace PAIN {

	class PAIN_API Application
	{ 
	private:
		std::unique_ptr<Window::Window> app_window;

	public:
		Application();
		virtual ~Application();

		void Run();
	};

	// Defined in client
	Application* CreateApplication();


}

#endif
