#pragma once

#ifndef GLFW_WINDOW_HPP
#define GLFW_WINDOW_HPP

#include "../Window.h"

namespace PAIN {
	namespace Window {

		//GLFW Window
		class GLFW_Window : public Window {
		private:
			//Window
			GLFWwindow* ptr_window;

			//Private internal functions
			virtual void init(Package const& package);
			virtual void shutdown();
		public:

			//Constructors & Destructors
			GLFW_Window(Package const& package);
			virtual ~GLFW_Window();

			//Update
			bool onUpdate() override;
		};
	}
}

#endif
