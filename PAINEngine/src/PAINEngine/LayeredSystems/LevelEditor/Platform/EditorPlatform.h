#pragma once

#ifdef _DEBUG
#ifndef EDITOR_PLATFORM_HPP
#define EDITOR_PLATFORM_HPP

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#undef APIENTRY

namespace PAIN {
	namespace Editor {

        //Editor platform interface
        class EditorPlatform {
        private:
            virtual void init() = 0;
            virtual void shutdown() = 0;
        public:
            EditorPlatform() = default;
            virtual ~EditorPlatform() = default;
            virtual void beginFrame() = 0;
            void endFrame() {
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            virtual void handleEvents(Event::Event& event) = 0;

            static EditorPlatform* createEditorPlatform(void* window);
        };
	}
}

#endif
#endif
