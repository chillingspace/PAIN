#pragma once

#ifndef EVENT_HPP
#define EVENT_HPP

#include <string>
#include <functional>
#include <stdexcept>


namespace PAIN {

	namespace Event {

#ifdef PN_PLATFORM_ANDROID
		//Android Event Types
		enum class Type {
			None = 0,
			// New Android-specific events
			AppStart, AppResume, AppPause, AppStop, AppDestroy,
			SurfaceCreated, SurfaceChanged, SurfaceDestroyed,
			FocusGained, FocusLost,
			TouchDown, TouchUp, TouchMove, TouchCancel,
			AndroidKeyDown, AndroidKeyUp, BackButton,
			LowMemory, ConfigurationChanged, SensorEvent,
			All
		};

		//Android Event Categories
		enum Category {
			None = 1 << 0,
			Application = 1 << 1,
			Input = 1 << 2,
			Keyboard = 1 << 3,
			Mouse = 1 << 4,
			Asset = 1 << 5,
			Touch = 1 << 6,  // New category for touch events
			Sensor = 1 << 7,   // New category for sensor events
			All = 1 << 8
		};
#else
		//Window Event Types
		enum class Type {
			None = 0,
			WindowResize, WindowFocus, WindowMove, WindowClosed,
			KeyTrigger, KeyPress, KeyRelease, KeyRepeat,
			MouseButtonPress, MouseButtonRelease, MouseMove, MouseScroll, CursorEnter,
			FileDrop, EntitiesChange
		};

		//Window Event Categories
		enum Category {
			None = 1 << 0,
			Application = 1 << 1,
			Input = 1 << 2,
			Keyboard = 1 << 3,
			Mouse = 1 << 4,
			Asset = 1 << 5,
			EntityChange = 1 << 6
		};
#endif

#define EVENT_CLASS_TYPE(type)\
    static PAIN::Event::Type getStaticType() { return PAIN::Event::Type::type; }\
    PAIN::Event::Type getType() const override { return getStaticType(); }\
    const char* getName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category)\
    int getCategoryFlags() const override { return category; }

		//Event Class
		class Event {
		private:
			friend class Dispatcher;
		protected:

			//Boolean for handling events
			bool b_event_handled = false;

		public:

			//Public getters
			virtual Type   getType() const = 0;
			virtual const char* getName()      const = 0;
			virtual int         getCategoryFlags() const = 0;
			bool isInCategory(Category c) const { return getCategoryFlags() & c; }
			virtual std::string toString() { return getName(); }

			bool checkHandled() const { return b_event_handled; }
		};

		//Event dispatcher
		class Dispatcher {
		private:
			Event& event;

			//Event function signature
			template<typename T>
			using Func = std::function<bool(T&)>;
		public:

			//Explicit dispatcher constructor
			explicit Dispatcher(Event& event) : event{ event }{}

			// Callable must be: bool(T& e)
			template<typename T>
			bool Dispatch(Func<T>&& func) {

				////Throw error why dispatching if type passed through is not valid event
				//if (std::is_base_of<Event, T>::value) {
				//	throw std::runtime_error("T must derive from Event");
				//}

				//Check event type
				if (event.getType() == T::getStaticType()) {

					//Execute function
					event.b_event_handled = func(*(T*)&event);
					return true;
				}

				//Return false continue dispatching
				return false;
			}
		};
	}
}

#endif
