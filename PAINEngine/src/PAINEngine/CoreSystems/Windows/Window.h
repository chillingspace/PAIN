#pragma once

#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <string>

#include "PAINEngine/Applications/AppSystem.h"

#include "GraphicsContext.h"

#include <memory>

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
		protected:
			// Rendering context (OpenGL for now)
			std::unique_ptr<GraphicsContext> m_Context;

			//Anrdoid state
			bool b_active = false;
		public:
			virtual ~Window() = default;

			//Update window
			virtual void onUpdate(float dt) override = 0;

			//Event callback
			virtual void onEvent(Event::Event& e) override = 0;

			//Register callbacks
			virtual void registerCallbacks(void* app) = 0;

			//Get native window
			virtual void* getNativeWindow() const = 0;

			//Pollevents
			virtual void pollEvents() = 0;

			//Swap buffers
			virtual void swapBuffers() = 0;

			//Get window active state
			bool getActive() const { return b_active; }

			//Create window
			static Window* create(void* app = nullptr, Package const& package = Package());
		};
	}
}

#endif
