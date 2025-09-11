#pragma once

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>

namespace PAIN {
	namespace Window {

		struct PAIN_API Package {
			std::string title;
			unsigned int width;
			unsigned int height;

			Package(std::string const& title = "Pain Engine", unsigned int width = 1600, unsigned int height = 900) : title{ title }, width{ width }, height{ height } {}
		};

		//Virtual window class
		class PAIN_API Window {
		public:
			virtual ~Window() = default;

			//Register callbacks
			virtual void registerCallbacks(void* app) = 0;

			//Update window
			virtual bool onUpdate() = 0;

			//Create window
			static Window* create(Package const& package = Package());
		};
	}
}

#endif
