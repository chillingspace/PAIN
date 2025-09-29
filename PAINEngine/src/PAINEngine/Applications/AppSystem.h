#pragma once

#ifndef APP_LAYER_HPP
#define APP_LAYER_HPP

#include "../CoreSystems/Events/Event.h"
#include "Services.h"

namespace PAIN {

	class AppSystem {
	private:
		friend class Application;
	protected:
		std::shared_ptr<Services> services;
	public:

		//Optional virtual functions
		virtual void onAttach() {}
		virtual void onDetach() {}
		virtual void onUpdate() = 0;

		//Event handler for app layer
		virtual void onEvent(Event::Event& e) = 0;
	};
}

#endif
