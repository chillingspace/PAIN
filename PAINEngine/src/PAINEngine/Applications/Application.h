#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "AppSystem.h"
#include "PAINEngine/CoreSystems/Events/Event.h"

#include <memory>
#include <vector>
#include <queue>
#include <chrono>

namespace PAIN {

	class Application
	{
	private:

		//Boolean running app running
		bool b_app_running = false;

		//App timings
		std::chrono::steady_clock::time_point last_time;
		float accumulator = 0.0f;
		AppTiming timing;

#ifdef  PN_PLATFORM_ANDROID
		int const MAX_STEPS = 4;
#else
		int const MAX_STEPS = 5;
#endif

		//Core services stack
		std::shared_ptr<Services> services;

		//Create applications stacks
		std::vector<std::shared_ptr<AppSystem>> layer_stack;
		std::vector<std::shared_ptr<AppSystem>> core_stack;

		//Event queue
		std::queue<std::shared_ptr<Event::Event>> event_queue;

		//Dispatch events to layers
		void dispatchEventsForward(Event::Event& e);

		//Reverse dispatching to layers
		void dispatchEventsReversed(Event::Event& e);

	public:
		Application();
		virtual ~Application();

		template<typename T>
		void addCoreSystem(std::shared_ptr<T> core_system);
		template<typename T>
		void addLayerSystem(std::shared_ptr<T> layer_system);
		void Init(void* app = nullptr);
		void Run();
		void terminate();
		void dispatchEvent(Event::Event& e);
		void pushEventQueue(std::shared_ptr<Event::Event> e);
		void drainEventQueue();

		//Get application state
		bool getAppState() const { return b_app_running; }
	};

	// Defined in client
	Application* CreateApplication();
}

#endif