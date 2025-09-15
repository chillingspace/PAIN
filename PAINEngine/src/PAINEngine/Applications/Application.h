#pragma once

#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include "AppLayer.h"
#include "../Managers/Events/Event.h"

#include <memory>
#include <vector>
#include <queue>

namespace PAIN {

	class Application
	{ 
	private:

		//Create applications stacks
		std::vector<std::shared_ptr<AppLayer>> layer_stack;
		std::vector<std::shared_ptr<AppLayer>> core_stack;

		//Boolean running app running
		bool b_app_running = true;

		//Event queue
		std::queue<std::shared_ptr<Event::Event>> event_queue;

		//Dispatch events to layers
		void dispatchEventsForward(Event::Event& e);

		//Reverse dispatching to layers
		void dispatchEventsReversed(Event::Event& e);

	public:
		Application();
		virtual ~Application();

		void Run();

		void terminate();

		void dispatchEvent(Event::Event& e);

		void pushEventQueue(std::shared_ptr<Event::Event> e);

		void drainEventQueue();
	};

	// Defined in client
	Application* CreateApplication();
}

#endif
