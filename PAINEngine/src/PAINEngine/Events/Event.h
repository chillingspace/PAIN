#pragma once

#ifndef EVENT_HPP
#define EVENT_HPP

#include "../Core.h"
#include <string>

namespace PAIN {

	namespace Event {

		//Event Types
		enum class Type {
			None = 0,
			WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
			KeyPressed, KeyReleased, KeyRepeat,
			MouseButtonPressed, MouseButtonReleased, MouseButtonRepeat, MouseMoved, MouseScrolled
		};

		//Event Categories
		enum Category {
			None		= 1 << 0,
			Application	= 1 << 1,
			Input		= 1 << 2,
			Keyboard	= 1 << 3,
			Mouse		= 1 << 4,
		};

#define EVENT_CLASS_TYPE(type)\
    static Type getStaticType() { return EventType::type; }\
    Type getEventType() const override { return GetStaticType(); }\
    const char* getName() const override { return #type; }

#define EVENT_CLASS_CATEGORY(category)\
    int getCategoryFlags() const override { return category; }

		//Event Class
		class PAIN_API Event {
		private:

		protected:

			//Boolean for handling events
			bool b_event_handled = false;

		public:

			//Public getters
			virtual Type   getType() const = 0;
			virtual const char* GetName()      const = 0;
			virtual int         getCategoryFlags() const = 0;
			bool IsInCategory(Category c) const { return getCategoryFlags() & c; }
			virtual std::string toString() { return GetName(); }
		};
	}
}

#endif
