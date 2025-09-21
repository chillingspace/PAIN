#pragma once

#ifndef MOUSE_EVENTS_HPP
#define MOUSE_EVENTS_HPP

#include "pch.h"
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

		class MouseMoved : public Event {
		private:
			glm::vec2 window_pos;
		public:

			//Construct event
			MouseMoved(glm::vec2 window_pos) : window_pos{ window_pos }{}

			//Get window pos
			glm::vec2 getWindowPos() const { return window_pos; }

			//Debug string
			std::string toString() override {
				std::stringstream ss;
				ss << "Mouse At, X: " << window_pos.x << ", Y: " << window_pos.y;
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(MouseMove);
			EVENT_CLASS_CATEGORY(Category::Mouse | Category::Input)
		};

		class MouseScrolled : public Event {
		private:
			glm::vec2 offset;
		public:

			//Construct event
			MouseScrolled(glm::vec2 offset) : offset{ offset } {}

			//Get window pos
			glm::vec2 getOffset() const { return offset; }

			//Debug string
			std::string toString() override {
				std::stringstream ss;
				ss << "Mouse Scroll Offset, X: " << offset.x << ", Y: " << offset.y;
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(MouseScroll);
			EVENT_CLASS_CATEGORY(Category::Mouse | Category::Input)
		};

		class CursorEntered : public Event {
		private:
			bool b_entered;
		public:

			//Construct event
			CursorEntered(bool b_entered) : b_entered{ b_entered } {}

			//Get window pos
			bool checkCursorEntered() const { return b_entered; }

			//Register Event
			EVENT_CLASS_TYPE(CursorEnter);
			EVENT_CLASS_CATEGORY(Category::Mouse | Category::Input)
		};
	}
}

#endif
