#pragma once

#ifdef _DEBUG
#ifdef PN_PLATFORM_WINDOWS
#ifndef EDITOR_GLFW_HPP
#define EDITOR_GLFW_HPP

#include "../EditorPlatform.h"
#include "imgui_impl_glfw.h"

namespace PAIN {
    namespace Editor {

        //GLFW Editor
        class EditorGLFW : public EditorPlatform {
        private:
            virtual void init() override;
            virtual void shutdown() override;
        public:
            EditorGLFW() { init(); }
            ~EditorGLFW() override { shutdown(); }
            virtual void beginFrame() override;
        };
    }
}

#endif
#endif
#endif