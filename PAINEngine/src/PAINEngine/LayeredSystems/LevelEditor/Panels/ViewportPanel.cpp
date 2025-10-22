#include "pch.h"
#include "ViewportPanel.h"

#ifdef _DEBUG
#include "../Editor.h" 

namespace PAIN {
	namespace Editor {
		namespace Panel {

			ViewportPanel::ViewportPanel()
				: renderTexture(0), texWidth(0), texHeight(0), isInputPaused(true), isSimulationPaused(false) // Start both paused by default
			{
				name = "##ViewportPanel";

				flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
					ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
					ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
					ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
			}

			void ViewportPanel::nextWindowSettings() {
				// No fullscreen behavior here — keep it dockable like AudioPanel
			}

			void ViewportPanel::setRenderTexture(ImTextureID texID, int width, int height) {
				renderTexture = texID;
				texWidth = width;
				texHeight = height;
			}

			void ViewportPanel::onAttach()
			{
			}

			float ViewportPanel::getTimeScale() const {
				return isSimulationPaused ? 0.0f : 1.0f;
			}

			void ViewportPanel::onUpdate(AppTiming timing) {

				if (!renderTexture) return;

				// Larger initial size (was 800x600, now 1280x720)
				ImVec2 initialSize(1280, 720);
				ImGui::SetNextWindowSize(initialSize, ImGuiCond_FirstUseEver);

				// Begin viewport window
				if (ImGui::Begin("Scene Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

					// Toolbar with Play/Pause buttons
					ImGui::BeginChild("##ViewportToolbar", ImVec2(0, 30), true, ImGuiWindowFlags_NoScrollbar);
					{
						auto editor = services->get<PAIN::Editor::Editor>();
						// Simulation controls
						if (ImGui::Button(editor->isPaused() ? "Play Scene" : "Pause Scene")) {
							editor->togglePause();
						}

					}
					ImGui::EndChild();

					ImVec2 avail = ImGui::GetContentRegionAvail();

					// Maintain aspect ratio
					float aspect = (float)texWidth / (float)texHeight;
					ImVec2 size = avail;
					if (size.x / size.y > aspect) {
						size.x = size.y * aspect;
					}
					else {
						size.y = size.x / aspect;
					}

					// Flip Y because ImGui expects UVs differently than many renderers
					ImGui::Image(renderTexture, size, ImVec2(0, 1), ImVec2(1, 0));

					contentHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup
						| ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
					isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

					// Forward input only when NOT paused AND the viewport wants it
					if (!isSimulationPaused && wantsInput()) {
						ImGuiIO& io = ImGui::GetIO();

						auto camera = services->get<sCameraController>();
						if (camera) {
							// Keyboard
							camera->W_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_W);
							camera->A_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_A);
							camera->S_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_S);
							camera->D_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_D);
							camera->SPACE_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_Space);
							camera->LCTRL_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

							// Mouse (LMB drag rotates in your code)
							camera->mouseButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

							// Provide per-frame mouse movement
							if (camera->mouseButtonDown) {
								camera->xOffset = io.MouseDelta.x;
								camera->yOffset = io.MouseDelta.y;
							}
						}
					}
					else {
						// When viewport loses focus/hover OR is paused, ensure keys don't "stick"
						if (auto camera = services->get<sCameraController>()) {
							camera->W_KEYDOWN = camera->A_KEYDOWN = camera->S_KEYDOWN = camera->D_KEYDOWN = false;
							camera->SPACE_KEYDOWN = camera->LCTRL_KEYDOWN = false;
							camera->mouseButtonDown = false;
						}
					}

				}
				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
