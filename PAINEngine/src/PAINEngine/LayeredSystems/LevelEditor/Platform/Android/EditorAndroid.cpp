#include "pch.h"

#ifdef _DEBUG
#ifdef PN_PLATFORM_ANDROID

#include "EditorAndroid.h"
#include "CoreSystems/Events/Android/OtherEvents.h"

namespace PAIN {
	namespace Editor {

		EditorPlatform* EditorPlatform::createEditorPlatform(void* window) {
			return new EditorAndroid(static_cast<ANativeWindow*>(window));
		}

		void EditorAndroid::init() {
			// CRITICAL: Create ImGui context FIRST!
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();

			// Configure ImGui
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

			// For Android, you might want to scale the UI
			io.FontGlobalScale = 2.0f; // Adjust based on your DPI

			//Set style
			ImGui::StyleColorsDark();

			//Initialize platform and renderer backends
			ImGui_ImplAndroid_Init(a_window);
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

		void EditorAndroid::updateShortCuts(std::shared_ptr<CommandManager> command) {

		}

		void EditorAndroid::handleEvents(Event::Event& event) {
			//Early exit condition
			if(!event.isInCategory(Event::Category::All)) return;

			//Create event dispatcher
			Event::Dispatcher dispatcher(event);

			//Dispatch window resized event
			dispatcher.Dispatch<Event::AllEvent>([&](Event::AllEvent& event) -> bool {

				//Update frame buffer size
				if (ImGui_ImplAndroid_HandleInputEvent(event.getEvent())) return true;

				//Return false: continue dispatching, true = stop dispatching 
				return false;
				});
		}
	}
}

#endif
#endif
