#pragma once

#ifndef KEY_EVENTS_HPP
#define KEY_EVENTS_HPP

#include "Event.h"

namespace PAIN {
	namespace Event {

		//Base key
		class Key : public Event {
		private:
			int key_code;
		public:

			//Construct event
			Key(int key_code) : key_code{ key_code } {}

			//Frame buffer fetch
			int getKeyCode() const { return key_code; }

			//Register event
			EVENT_CLASS_CATEGORY(Category::Keyboard)
		};

		//Key press
		class KeyPress : public Key {
		private:
		public:

			//Construct event
			KeyPress(int key_code) : Key(key_code) {}

			//Register Event
			EVENT_CLASS_TYPE(KeyPressed);
		};

	}
}

#endif#pragma once
