#pragma once

#ifndef APP_LAYER_HPP
#define APP_LAYER_HPP

#include "../CoreSystems/Events/Event.h"

#include "../Utility/CustomTemplates.h"

namespace PAIN {

	class Services : public Custom::ClassWeakMap {
	public:
		Services() = default;
	};

	class AppSystem {
	private:
		friend class Application;
	protected:
		std::shared_ptr<Services> services;
	public:

		//Optional virtual functions
		virtual void onAttach() {}
		virtual void onDetach() {}
		virtual void onUpdate(float dt) = 0;
		virtual void onAppPause() {}
		virtual void onAppResume() {}

		//Event handler for app layer
		virtual void onEvent(Event::Event& e) = 0;
	};
}

#endif
