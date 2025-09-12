#pragma once

#ifndef MOUSE_EVENTS_HPP
#define MOUSE_EVENTS_HPP

#include "Event.h"
#include <sstream>

namespace PAIN {
	namespace Event {

		//Base Mouse Btn
		class MouseBtn : public Event {
		protected:
			int btn_code;
		public:

			//Construct event
			MouseBtn(int btn_code) : btn_code{ btn_code } {}

			//Frame buffer fetch
			int getBtnCode() const { return btn_code; }

			//Register event
			EVENT_CLASS_CATEGORY(Category::Mouse | Category::Input)
		};

		class MouseBtnPressed : public MouseBtn {
		private:
		public:

			//Construct event
			MouseBtnPressed(int btn_code) : MouseBtn(btn_code) {}

			//Debug output
			std::string toString() override {
				std::stringstream ss;
				ss << "Mouse Button " << btn_code << " Pressed.";
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(MouseButtonPress);
		};

		class MouseBtnReleased : public MouseBtn {
		private:
		public:

			//Construct event
			MouseBtnReleased(int btn_code) : MouseBtn(btn_code) {}

			//Debug output
			std::string toString() override {
				std::stringstream ss;
				ss << "Mouse Button " << btn_code << " Released.";
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(MouseButtonRelease);
		};
	}
}

#endif
