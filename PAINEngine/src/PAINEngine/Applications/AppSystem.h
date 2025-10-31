#pragma once

#ifndef APP_LAYER_HPP
#define APP_LAYER_HPP

#include "../CoreSystems/Events/Event.h"

#include "../Utility/CustomTemplates.h"

namespace PAIN {

	struct AppTiming {
		float dt = 0.0f;
		float fixed_dt = 1.0f / 60.0f;
		float alpha = 0.0f;
		int steps_this_frame = 0;
	};

	class Services : public Custom::ClassMap {
	public:
		Services() = default;
		~Services() override = default;
	};

	class AppSystem {
	private:
		friend class Application;
	protected:
		std::shared_ptr<Services> services;
	public:

		AppSystem() = default;
		virtual ~AppSystem() = default;

		//Optional virtual functions
		virtual void onAttach() {}
		virtual void onDetach() {}

		// Ngl if AppTiming has fixed_dt, i dont think onFixedUpdate is needed anymore 
		virtual void onFixedUpdate(AppTiming timing) {};
		virtual void onUpdate(AppTiming timing) {};
		virtual void onAppPause() {}
		virtual void onAppResume() {}

		//Event handler for app layer
		virtual void onEvent(Event::Event& e) = 0;
	};
}

#endif
