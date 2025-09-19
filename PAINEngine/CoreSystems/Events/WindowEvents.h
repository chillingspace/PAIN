#pragma once

#ifndef WINDOW_EVENTS_HPP
#define WINDOW_EVENTS_HPP

#include "CoreSystems/Events/Event.h"

namespace PAIN {
	namespace Event {

		class WindowResized : public Event {
		private:
			glm::uvec2 frame_buffer;
		public:

			//Construct event
			WindowResized(glm::uvec2 frame_buffer) : frame_buffer{ frame_buffer }{}

			//Frame buffer fetch
			glm::uvec2 getFrameBuffer() const { return frame_buffer; }

			//Debug output
			std::string toString() override {
				std::stringstream ss;
				ss << "Frame Buffer Resized, X: " << frame_buffer.x << ", Y: " << frame_buffer.y;
				return ss.str();
			}

			//Register event
			EVENT_CLASS_TYPE(WindowResize)
			EVENT_CLASS_CATEGORY(Category::Application)
		};

		class WindowFocused : public Event {
		private:
			bool b_focus;
		public:

			//Construct event
			WindowFocused(bool b_focus) : b_focus{ b_focus } {}

			//Get window pos
			bool checkWindowFocus() const { return b_focus; }

			//Register Event
			EVENT_CLASS_TYPE(WindowFocus);
			EVENT_CLASS_CATEGORY(Category::Application)
		};

		class WindowMoved : public Event {
		private:
			glm::uvec2 window_pos;
		public:

			//Construct event
			WindowMoved(glm::uvec2 window_pos) : window_pos{ window_pos } {}

			//Get window pos
			glm::uvec2 getWindowPos() const { return window_pos; }

			//Debug string
			std::string toString() override {
				std::stringstream ss;
				ss << "Window Moved To, X: " << window_pos.x << ", Y: " << window_pos.y;
				return ss.str();
			}

			//Register Event
			EVENT_CLASS_TYPE(WindowMove);
			EVENT_CLASS_CATEGORY(Category::Application)
		};
	}
}

#endif
