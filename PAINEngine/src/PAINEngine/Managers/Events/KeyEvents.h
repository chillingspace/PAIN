#pragma once

#ifndef KEY_EVENTS_HPP
#define KEY_EVENTS_HPP

#include "Event.h"

namespace PAIN {
	namespace Event {

		//Base key
		class Key : public Event {
		protected:
			int key_code;
		public:

			//Construct event
			Key(int key_code) : key_code{ key_code } {}

			//Frame buffer fetch
			int getKeyCode() const { return key_code; }

			//Register event
			EVENT_CLASS_CATEGORY(Category::Keyboard | Category::Input)
		};

		class KeyTriggered : public Key {
		private:
		public:

			//Construct event
			KeyTriggered(int key_code) : Key(key_code) {}

			//Debug output
			std::string toString() override {
				std::stringstream ss;
				ss << "Key Button " << key_code << " Triggered.";
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(KeyTrigger);
		};

		class KeyPressed : public Key {
		private:
		public:

			//Construct event
			KeyPressed(int key_code) : Key(key_code) {}

			//Debug output
			std::string toString() override {
				std::stringstream ss;
				ss << "Key Button " << key_code << " Pressed.";
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(KeyPress);
		};

		class KeyReleased : public Key {
		private:
		public:

			//Construct event
			KeyReleased(int key_code) : Key(key_code) {}

			//Debug output
			std::string toString() override {
				std::stringstream ss;
				ss << "Key Button " << key_code << " Released.";
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(KeyRelease);
		};

		class KeyRepeated : public Key {
		private:
		public:

			//Construct event
			KeyRepeated(int key_code) : Key(key_code) {}

			//Debug output
			std::string toString() override {
				std::stringstream ss;
				ss << "Key Button " << key_code << " Repeated.";
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(KeyRepeat);
		};
	}
}

#endif
