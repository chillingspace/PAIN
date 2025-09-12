#pragma once

#ifndef WINDOW_EVENTS_HPP
#define WINDOW_EVENTS_HPP

#include "Event.h"

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

	}
}

#endif