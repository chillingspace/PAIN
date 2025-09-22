#include "pch.h"

#ifdef _DEBUG
#ifdef PN_PLATFORM_ANDROID

#include "EditorAndroid.h"

namespace PAIN {
	namespace Editor {

		EditorPlatform* EditorPlatform::createEditorPlatform() {
			return new EditorAndroid();
		}

		void EditorAndroid::init() {
			ImGui_ImplOpenGL3_Init("#version 300 es");
		}

		void EditorAndroid::shutdown() {
			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplAndroid_Shutdown();
			ImGui::DestroyContext();
		}

		void EditorAndroid::beginFrame() {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplAndroid_NewFrame();
			ImGui::NewFrame();
		}
	}
}

#endif
#endif
