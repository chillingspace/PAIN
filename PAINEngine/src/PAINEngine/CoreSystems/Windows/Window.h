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

			Package(std::string const& title = ENGINE_NAME, unsigned int width = DEF_ENGINE_WIDTH, unsigned int height = DEF_ENGINE_HEIGHT) : title{ title }, width{ width }, height{ height } {}
		};

		//Virtual window class
		class Window : public AppSystem {
		protected:
			// Rendering context (OpenGL for now)
			std::unique_ptr<GraphicsContext> m_Context;

			//Android state
			bool b_active = false;
		public:
			virtual ~Window() = default;

			virtual void onAttach() override {}
			virtual void onDetach() override = 0;

			//Update window
			virtual void onFixedUpdate(AppTiming timing) override {}
			virtual void onUpdate(AppTiming timing) override = 0;

			//Event callback
			virtual void onEvent(Event::Event& e) override = 0;

			//Register callbacks
			virtual void registerCallbacks(void* app) = 0;

			//Get native window
			virtual void* getNativeWindow() const = 0;

			virtual std::string getWritablePath() const { return ""; }

			//Pollevents
			virtual void pollEvents() = 0;

			//Swap buffers
			virtual void swapBuffers() = 0;

			//Get window active state
			bool getActive() const { return b_active; }

			//Get window height and width
			virtual glm::uvec2 getFrameBuffer() const = 0;

			//Safe shutdown
			virtual void safeShutdown() = 0;

			//Get is minimized
			virtual bool isMinimized() const = 0;

			//Hide cursor 
			virtual void hideCursor(bool hide) = 0;

			//Fullscreen toggle (PC only, no-op on mobile)
			virtual void setFullscreen(bool fullscreen) {}

			//Create window
			static Window* create(void* app = nullptr, Package const& package = Package());

		};
	}
}

#endif
