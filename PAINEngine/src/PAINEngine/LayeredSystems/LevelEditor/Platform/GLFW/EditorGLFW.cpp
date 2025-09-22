#include "pch.h"

#ifdef _DEBUG
#ifdef PN_PLATFORM_WINDOWS

#include "EditorGLFW.h"

namespace PAIN {
	namespace Editor {

		EditorPlatform* EditorPlatform::createEditorPlatform() {
			return new EditorGLFW();
		}

		void EditorGLFW::init() {
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;

			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			ImGui::StyleColorsDark();

			ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
			ImGui_ImplOpenGL3_Init("#version 450");

			ImGui::LoadIniSettingsFromDisk(io.IniFilename);
		}

		void EditorGLFW::shutdown() {
			ImGuiIO& io = ImGui::GetIO();
			ImGui::SaveIniSettingsToDisk(io.IniFilename);

			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
		}

		void EditorGLFW::beginFrame() {
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
		}
	}
}

#endif
#endif
