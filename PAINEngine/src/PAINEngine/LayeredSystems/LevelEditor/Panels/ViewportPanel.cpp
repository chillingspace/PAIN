#include "pch.h"
#include "ViewportPanel.h"

#ifdef _DEBUG

namespace PAIN {
	namespace Editor {
		namespace Panel {

			ViewportPanel::ViewportPanel()
				: renderTexture(0), texWidth(0), texHeight(0)
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

			void ViewportPanel::onUpdate(AppTiming timing) {
				if (!renderTexture) return;

				ImVec2 initialSize(800, 600);
				ImGui::SetNextWindowSize(initialSize, ImGuiCond_FirstUseEver);

				// Begin viewport window
				if (ImGui::Begin("Scene Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
					ImVec2 avail = ImGui::GetContentRegionAvail();

					// Maintain aspect ratio
					float aspect = (float)texWidth / (float)texHeight;
					ImVec2 size = avail;
					if (size.x / size.y > aspect) {
						size.x = size.y * aspect;
					} else {
						size.y = size.x / aspect;
					}

					// Flip Y because ImGui expects UVs differently than many renderers
					ImGui::Image(renderTexture, size, ImVec2(0, 1), ImVec2(1, 0));

					contentHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup
						| ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
					isFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

					// Forward input only when the viewport wants it
					if (wantsInput()) {
						ImGuiIO& io = ImGui::GetIO();

						// Get your RendererLayer however you access services.
						// If your Panel base provides `services`, use it. Otherwise,
						// expose a getter on Editor to reach RendererLayer.
						auto renderer = services->get<RendererLayer>(); // adjust if needed
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

							// Provide per-frame mouse movement (your RendererLayer consumes xOffset/yOffset and then resets)
							if (renderer->mouseButtonDown) {
								renderer->xOffset = io.MouseDelta.x;
								renderer->yOffset = io.MouseDelta.y;
							}
						}
					}
					else {
						// When viewport loses focus/hover, ensure keys don’t “stick”
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
