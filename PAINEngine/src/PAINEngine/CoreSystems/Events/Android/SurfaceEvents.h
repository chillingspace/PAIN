#pragma once

#ifdef PN_PLATFORM_ANDROID
#ifndef SURFACE_EVENTS_HPP
#define SURFACE_EVENTS_HPP

#include "../Event.h"

namespace PAIN {
    namespace Event {

        // Window/Surface Events
        class SurfaceEvent : public Event {
        protected:
            void* window_ptr;
        public:
            SurfaceEvent(void* window) : window_ptr(window) {}
            void* getWindow() const { return window_ptr; }
            EVENT_CLASS_CATEGORY(Category::Application)
        };

        class SurfaceCreated : public SurfaceEvent {
        private:
            int width, height;
        public:
            SurfaceCreated(void* window, int w, int h) 
                : SurfaceEvent(window), width(w), height(h) {}

            int getWidth() const { return width; }
            int getHeight() const { return height; }

            std::string toString() override {
                std::stringstream ss;
                ss << "Android Surface Created: " << width << "x" << height;
                return ss.str();
            }

            EVENT_CLASS_TYPE(SurfaceCreated);
        };

        class SurfaceChanged : public SurfaceEvent {
        private:
            int width, height;
        public:
            SurfaceChanged(void* window, int w, int h)
                : SurfaceEvent(window), width(w), height(h) {
            }

            int getWidth() const { return width; }
            int getHeight() const { return height; }

            std::string toString() override {
                std::stringstream ss;
                ss << "Android Surface Changed: " << width << "x" << height;
                return ss.str();
            }

            EVENT_CLASS_TYPE(SurfaceChanged);
        };

        class SurfaceDestroyed : public SurfaceEvent {
        public:
            SurfaceDestroyed(void* window) : SurfaceEvent(window) {}

            std::string toString() override {
                return "Android Surface Destroyed";
            }

            EVENT_CLASS_TYPE(SurfaceDestroyed);
        };
    }
}

#endif
#endif
