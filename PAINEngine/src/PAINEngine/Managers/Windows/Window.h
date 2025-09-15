#pragma once

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>
#include "ECS/System/System.h"

#include "Applications/AppSystem.h"

namespace PAIN {
	namespace Window {

		struct Package {
			std::string title;
			unsigned int width;
			unsigned int height;

			Package(std::string const& title = "Pain Engine", unsigned int width = 1600, unsigned int height = 900) : title{ title }, width{ width }, height{ height } {}
		};

		//Virtual window class
		class Window : public AppSystem {
		public:
			virtual ~Window() = default;

			//Register callbacks
			virtual void registerCallbacks(void* app) = 0;

			//Update window
			virtual void onUpdate() = 0;

			//Event callback
			virtual void onEvent(Event::Event& e) = 0;

			//Create window
			static Window* create(Package const& package = Package());
		};
	}
}

#endif
