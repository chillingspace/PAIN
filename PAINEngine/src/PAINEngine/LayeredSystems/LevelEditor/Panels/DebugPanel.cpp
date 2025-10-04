#include "pch.h"
#include "DebugPanel.h"
#include "PAINEngine/Applications/Application.h"
#include "PAINEngine/CoreSystems/Renderer/RendererLayer.h"

#ifdef _DEBUG

namespace PAIN {
	namespace Editor {
		namespace Panel {

			DebugPanel::DebugPanel() {
				name = "Debug Panel";
				flags = ImGuiWindowFlags_None;
			}

			void DebugPanel::nextWindowSettings() {
			}

			void DebugPanel::onUpdate(AppTiming timing) {
				ImGui::Begin(name.c_str(), nullptr, flags);


				// 1. Performance Visualizer

				// Keep a rolling history of frame times (100 samples)
				const int MAX_SAMPLES = 100;
				frameTimes.push_back(timing.dt * 1000.0f); // ms
				if (frameTimes.size() > MAX_SAMPLES)
					frameTimes.erase(frameTimes.begin());

				float avgMs = 0.0f;
				for (float f : frameTimes) avgMs += f;
				avgMs /= frameTimes.size();
				float fps = 1000.0f / avgMs;

				ImGui::Text("Performance Metrics:");
				ImGui::Text("Avg Frame Time: %.2f ms (%.1f FPS)", avgMs, fps);
				ImGui::PlotLines("Frame Time (ms)", frameTimes.data(), (int)frameTimes.size(),
					0, nullptr, 0.0f, 40.0f, ImVec2(0, 80));


				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif
