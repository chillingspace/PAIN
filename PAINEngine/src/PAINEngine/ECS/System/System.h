#pragma once

#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include "Managers/Events/Event.h"

namespace PAIN {
	namespace ECS {

		class ISystem {
		private:
		public:
			//Event handler for app layer
			virtual void onEvent(Event::Event& e) = 0;
		};
	}
}

#endif