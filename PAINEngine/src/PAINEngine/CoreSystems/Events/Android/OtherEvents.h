#pragma once

#ifdef PN_PLATFORM_ANDROID
#ifndef OTHER_EVENTS_HPP
#define OTHER_EVENTS_HPP

#include "../Event.h"
#include <android_native_app_glue.h>
#include <sstream>

namespace PAIN {
    namespace Event {

        //All Event class
        class AllEvent : public Event {
        protected:
            AInputEvent* ai_event;
        public:
            AllEvent(AInputEvent* event) {
                //Validate before storing
                if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
                    int32_t pointerCount = AMotionEvent_getPointerCount(event);

                    if (pointerCount == 0) {
                        return;
                    }
                }

                ai_event = event;
            }
            AInputEvent* getEvent() const { return ai_event; }
            EVENT_CLASS_TYPE(All);
            EVENT_CLASS_CATEGORY(Category::All)
        };

        // Hardware Key Events (Android specific keys)
        class AndroidKey : public Event {
        protected:
            int key_code;
            int meta_state;
        public:
            AndroidKey(int code, int meta) : key_code(code), meta_state(meta) {}

            int getKeyCode() const { return key_code; }
            int getMetaState() const { return meta_state; }

            EVENT_CLASS_CATEGORY(Category::Keyboard | Category::Input)
        };

        class AndroidKeyDown : public AndroidKey {
        public:
            AndroidKeyDown(int code, int meta) : AndroidKey(code, meta) {}

            std::string toString() override {
                std::stringstream ss;
                ss << "Android Key " << key_code << " Down (Meta: " << meta_state << ")";
                return ss.str();
            }

            EVENT_CLASS_TYPE(AndroidKeyDown);
        };

        class AndroidKeyUp : public AndroidKey {
        public:
            AndroidKeyUp(int code, int meta) : AndroidKey(code, meta) {}

            std::string toString() override {
                std::stringstream ss;
                ss << "Android Key " << key_code << " Up (Meta: " << meta_state << ")";
                return ss.str();
            }

            EVENT_CLASS_TYPE(AndroidKeyUp);
        };

        // Back Button Event (Important for Android)
        class BackButton : public Event {
        public:
            BackButton() {}

            std::string toString() override {
                return "Android Back Button Pressed";
            }

            EVENT_CLASS_TYPE(BackButton);
            EVENT_CLASS_CATEGORY(Category::Input | Category::Keyboard | Category::Application)
        };

        // Memory Events
        class LowMemory : public Event {
        public:
            LowMemory() {}

            std::string toString() override {
                return "Android Low Memory Warning";
            }

            EVENT_CLASS_TYPE(LowMemory);
            EVENT_CLASS_CATEGORY(Category::Application)
        };

        // Configuration Change Events
        class ConfigurationChanged : public Event {
        public:
            ConfigurationChanged() {}

            std::string toString() override {
                return "Android Configuration Changed";
            }

            EVENT_CLASS_TYPE(ConfigurationChanged);
            EVENT_CLASS_CATEGORY(Category::Application)
        };

        // Sensor Events (if you plan to use accelerometer, gyroscope, etc.)
        class SensorEvent : public Event {
        protected:
            int sensor_type;
            float values[3];
            float accuracy;
        public:
            SensorEvent(int type, float x, float y, float z, float acc)
                : sensor_type(type), accuracy(acc) {
                values[0] = x;
                values[1] = y;
                values[2] = z;
            }

            int getSensorType() const { return sensor_type; }
            const float* getValues() const { return values; }
            float getAccuracy() const { return accuracy; }

            std::string toString() override {
                std::stringstream ss;
                ss << "Sensor " << sensor_type << " Data: ("
                    << values[0] << ", " << values[1] << ", " << values[2] << ")";
                return ss.str();
            }

            EVENT_CLASS_TYPE(SensorEvent);
            EVENT_CLASS_CATEGORY(Category::Input)
        };
    }
}

#endif
#endif
