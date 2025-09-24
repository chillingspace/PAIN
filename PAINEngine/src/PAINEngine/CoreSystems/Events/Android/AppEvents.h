#pragma once

#ifdef PN_PLATFORM_ANDROID
#ifndef APP_EVENTS_HPP
#define APP_EVENTS_HPP

#include "../Event.h"

namespace PAIN {
	namespace Event {

        // Android App Lifecycle Events
        class AppEvent : public Event {
        protected:
            int command;
        public:
            AppEvent(int cmd) : command(cmd) {}
            int getCommand() const { return command; }
            EVENT_CLASS_CATEGORY(Category::Application)
        };

        class AppStart : public AppEvent {
        public:
            AppStart() : AppEvent(0) {}

            std::string toString() override {
                return "Android App Started";
            }

            EVENT_CLASS_TYPE(AppStart);
        };

        class AppResume : public AppEvent {
        public:
            AppResume() : AppEvent(1) {}

            std::string toString() override {
                return "Android App Resumed";
            }

            EVENT_CLASS_TYPE(AppResume);
        };

        class AppPause : public AppEvent {
        public:
            AppPause() : AppEvent(2) {}

            std::string toString() override {
                return "Android App Paused";
            }

            EVENT_CLASS_TYPE(AppPause);
        };

        class AppStop : public AppEvent {
        public:
            AppStop() : AppEvent(3) {}

            std::string toString() override {
                return "Android App Stopped";
            }

            EVENT_CLASS_TYPE(AppStop);
        };

        class AppDestroy : public AppEvent {
        public:
            AppDestroy() : AppEvent(4) {}

            std::string toString() override {
                return "Android App Destroyed";
            }

            EVENT_CLASS_TYPE(AppDestroy);
        };
	}
}

#endif
#endif
