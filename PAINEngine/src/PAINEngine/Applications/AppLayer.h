#pragma once

#ifndef APP_LAYER_HPP
#define APP_LAYER_HPP

#include "../Managers/Events/Event.h"
#include <memory>

namespace PAIN {

	class AppLayer {
	private:
	public:
		//Optional virtual functions
		virtual void onAttach() {}
		virtual void onDetach() {}
		virtual void onUpdate() = 0;

		//Event handler for app layer
		virtual void onEvent(Event::Event& e) = 0;
	};

	class AppLayerStack {
	private:

		//Vector of app layer
		std::vector<std::shared_ptr<AppLayer>> layer_stack;

		//Boolean to check if app is running
		bool b_app_running = true;

	public:

		//Default constructor
		AppLayerStack() = default;

		//Dispatch events to layers
		void dispatchToLayers(Event::Event& e);

		//Reverse dispatching to layers
		void dispatchToLayersReversed(Event::Event& e);

		//Add systems
		void addLayer(std::shared_ptr<AppLayer> layer);

		//Update function
		void onUpdate();

		//Check app running
		bool checkAppRunning() const;

		//Terminate app
		void terminateApp();
	};
}

#endif
