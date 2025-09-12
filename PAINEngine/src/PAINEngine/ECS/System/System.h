#pragma once

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "Managers/Events/Event.h"

namespace PAIN {
	namespace ECS {

		//Base ECS Class
		class ISystem {
		private:
		public:

			//Optional virtual functions
			virtual void onAttach(){}
			virtual void onDetach(){}
			virtual void onUpdate() = 0;

			//Event handler for app layer
			virtual void onEvent(Event::Event& e) = 0;
		};
	}
}

#endif