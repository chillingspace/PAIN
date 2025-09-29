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
				}
				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
