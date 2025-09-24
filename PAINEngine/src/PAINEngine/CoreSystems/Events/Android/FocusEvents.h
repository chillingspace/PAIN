#pragma once

#ifdef PN_PLATFORM_ANDROID
#ifndef FOCUS_EVENTS_HPP
#define FOCUS_EVENTS_HPP

#include "../Event.h"

namespace PAIN {
    namespace Event {

        // Focus Events
        class FocusEvent : public Event {
        protected:
            bool has_focus;
        public:
            FocusEvent(bool focus) : has_focus(focus) {}
            bool hasFocus() const { return has_focus; }
            EVENT_CLASS_CATEGORY(Category::Application)
        };

        class FocusGained : public FocusEvent {
        public:
            FocusGained() : FocusEvent(true) {}

            std::string toString() override {
                return "Android App Gained Focus";
            }

            EVENT_CLASS_TYPE(FocusGained);
        };

        class FocusLost : public FocusEvent {
        public:
            FocusLost() : FocusEvent(false) {}

            std::string toString() override {
                return "Android App Lost Focus";
            }

            EVENT_CLASS_TYPE(FocusLost);
        };
    }
}

#endif
#endif
