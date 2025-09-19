#pragma once

#ifndef APP_LAYER_HPP
#define APP_LAYER_HPP

#include "CoreSystems/Events/Event.h"

namespace PAIN {

	class AppSystem {
	private:
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
