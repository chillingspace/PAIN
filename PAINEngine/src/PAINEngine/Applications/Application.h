#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "AppSystem.h"
#include "PAINEngine/CoreSystems/Events/Event.h"
#include "PAINEngine/CoreSystems/Windows/Window.h"

#include <memory>
#include <vector>
#include <queue>

namespace PAIN {

	class Application
	{
	private:
		// Application instance
		static Application* s_Instance;

		//Window instance
		std::shared_ptr<Window::Window> app_window;

		//Create applications stacks
		std::vector<std::shared_ptr<AppSystem>> layer_stack;
		std::vector<std::shared_ptr<AppSystem>> core_stack;

		//Boolean running app running
		bool b_app_running = false;

		//Event queue
		std::queue<std::shared_ptr<Event::Event>> event_queue;

		// Direct pointer to the AudioManager
		//std::shared_ptr<AudioManager> m_AudioManager;

		//std::shared_ptr<Window::Window> m_Window;

		//Dispatch events to layers
		void dispatchEventsForward(Event::Event& e);

		//Reverse dispatching to layers
		void dispatchEventsReversed(Event::Event& e);

	public:
		Application();
		virtual ~Application();

		void addCoreSystem(std::shared_ptr<AppSystem> core_system);
		void addLayerSystem(std::shared_ptr<AppSystem> layer_system);
		void Init(void* app = nullptr);
		void Run();
		void terminate();
		void dispatchEvent(Event::Event& e);
		void pushEventQueue(std::shared_ptr<Event::Event> e);
		void drainEventQueue();

		//Get application state
		bool getAppState() const { return b_app_running; }

		// Static getter to access the application and audio manager
		inline static Application& Get() { return *s_Instance; }
		//inline AudioManager& GetAudioManager() { return *m_AudioManager; }

		inline Window::Window& GetWindow() { return *app_window; }
		inline const Window::Window& GetWindow() const { return *app_window; }
		inline std::shared_ptr<Window::Window> GetWindowPtr() const { return app_window; }
	};

	// Defined in client
	Application* CreateApplication();
}

#endif