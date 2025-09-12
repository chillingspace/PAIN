#pragma once

#define PAIN_API

#include "Managers/Events/Event.h"

namespace PAIN {
	class AppLayer {
	private:
	public:
		//Event handler for app layer
		virtual void OnEvent(Event::Event& e) = 0;
	};
}
