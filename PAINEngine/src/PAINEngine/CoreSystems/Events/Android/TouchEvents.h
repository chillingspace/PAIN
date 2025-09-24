#pragma once

#ifdef PN_PLATFORM_ANDROID
#ifndef TOUCH_EVENTS_HPP
#define TOUCH_EVENTS_HPP

#include "../Event.h"

namespace PAIN {
    namespace Event {

        // Touch Events
        class TouchEvent : public Event {
        protected:
            float x, y;
            int pointer_id;
        public:
            TouchEvent(float x_pos, float y_pos, int id)
                : x(x_pos), y(y_pos), pointer_id(id) {
            }

            float getX() const { return x; }
            float getY() const { return y; }
            int getPointerId() const { return pointer_id; }

            EVENT_CLASS_CATEGORY(Category::Input | Category::Mouse)
        };

        class TouchDown : public TouchEvent {
        public:
            TouchDown(float x, float y, int id) : TouchEvent(x, y, id) {}

            std::string toString() override {
                std::stringstream ss;
                ss << "Touch Down at (" << x << ", " << y << ") ID: " << pointer_id;
                return ss.str();
            }

            EVENT_CLASS_TYPE(TouchDown);
        };

        class TouchUp : public TouchEvent {
        public:
            TouchUp(float x, float y, int id) : TouchEvent(x, y, id) {}

            std::string toString() override {
                std::stringstream ss;
                ss << "Touch Up at (" << x << ", " << y << ") ID: " << pointer_id;
                return ss.str();
            }

            EVENT_CLASS_TYPE(TouchUp);
        };

        class TouchMove : public TouchEvent {
        public:
            TouchMove(float x, float y, int id) : TouchEvent(x, y, id) {}

            std::string toString() override {
                std::stringstream ss;
                ss << "Touch Move at (" << x << ", " << y << ") ID: " << pointer_id;
                return ss.str();
            }

            EVENT_CLASS_TYPE(TouchMove);
        };

        class TouchCancel : public TouchEvent {
        public:
            TouchCancel(float x, float y, int id) : TouchEvent(x, y, id) {}

            std::string toString() override {
                std::stringstream ss;
                ss << "Touch Cancel at (" << x << ", " << y << ") ID: " << pointer_id;
                return ss.str();
            }

            EVENT_CLASS_TYPE(TouchCancel);
        };
    }
}

#endif
#endif
