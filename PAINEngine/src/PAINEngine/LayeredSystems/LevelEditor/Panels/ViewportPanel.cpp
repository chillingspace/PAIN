#include "pch.h"
#include "ViewportPanel.h"

#ifdef _DEBUG

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
						// Simulation controls
						if (ImGui::Button("Play Scene")) {
							isSimulationPaused = false;
						}

						ImGui::SameLine();

						if (ImGui::Button("Pause Scene")) {
							isSimulationPaused = true;
						}

						ImGui::SameLine();
						ImGui::Text(isSimulationPaused ? "| Scene: Paused" : "| Scene: Playing");

						ImGui::SameLine();
						ImGui::Text("  ");  // Spacer

						// Input controls
						ImGui::SameLine();
						if (ImGui::Button("Enable Input")) {
							isInputPaused = false;
						}

						ImGui::SameLine();

						if (ImGui::Button("Disable Input")) {
							isInputPaused = true;
							// Clear input state when disabling input
							if (auto renderer = services->get<RendererLayer>()) {
								renderer->W_KEYDOWN = renderer->A_KEYDOWN = renderer->S_KEYDOWN = renderer->D_KEYDOWN = false;
								renderer->SPACE_KEYDOWN = renderer->LCTRL_KEYDOWN = false;
								renderer->mouseButtonDown = false;
								renderer->xOffset = renderer->yOffset = 0.0f;
							}
						}

						ImGui::SameLine();
						ImGui::Text(isInputPaused ? "| Input: Disabled" : "| Input: Enabled");
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
					if (!isInputPaused && wantsInput()) {
						ImGuiIO& io = ImGui::GetIO();

						auto renderer = services->get<RendererLayer>();
						if (renderer) {
							// Keyboard
							renderer->W_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_W);
							renderer->A_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_A);
							renderer->S_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_S);
							renderer->D_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_D);
							renderer->SPACE_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_Space);
							renderer->LCTRL_KEYDOWN = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

							// Mouse (LMB drag rotates in your code)
							renderer->mouseButtonDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

							// Provide per-frame mouse movement
							if (renderer->mouseButtonDown) {
								renderer->xOffset = io.MouseDelta.x;
								renderer->yOffset = io.MouseDelta.y;
							}
						}
					}
					else {
						// When viewport loses focus/hover OR is paused, ensure keys don't "stick"
						if (auto renderer = services->get<RendererLayer>()) {
							renderer->W_KEYDOWN = renderer->A_KEYDOWN = renderer->S_KEYDOWN = renderer->D_KEYDOWN = false;
							renderer->SPACE_KEYDOWN = renderer->LCTRL_KEYDOWN = false;
							renderer->mouseButtonDown = false;
						}
					}

				}
				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
