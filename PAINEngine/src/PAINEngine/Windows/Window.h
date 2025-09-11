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

			Package(std::string const& title = "Pain Engine", unsigned int width = 1280, unsigned height = 720) : title{ title }, width{ width }, height{ height } {}
		};

		//Virtual window class
		class PAIN_API Window {
		public:
			virtual ~Window() = default;
			virtual bool onUpdate() = 0;
			static Window* create(Package const& package = Package());
		};
	}
}

#endif
