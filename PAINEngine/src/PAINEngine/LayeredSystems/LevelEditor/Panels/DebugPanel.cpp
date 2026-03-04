#include "pch.h"
#include "DebugPanel.h"
#include "PAINEngine/Applications/Application.h"
#include "PAINEngine/CoreSystems/Renderer/sRenderer.h"

#ifdef _DEBUG

namespace PAIN {
	namespace Editor {
		namespace Panel {

			DebugPanel::DebugPanel() {
				name = "Debug Panel";
				flags = ImGuiWindowFlags_None;
			}

			void DebugPanel::nextWindowSettings() {}
			void DebugPanel::onAttach() {}

			void DebugPanel::onUpdate(AppTiming timing) {
				ImGui::Begin(name.c_str(), nullptr, flags);

				// ===== 1. Performance Metrics =====
				if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
					const int MAX_SAMPLES = 100;
					frameTimes.push_back(timing.dt * 1000.0f);
					if (frameTimes.size() > MAX_SAMPLES)
						frameTimes.erase(frameTimes.begin());

					float avgMs = 0.0f;
					for (float f : frameTimes) avgMs += f;
					avgMs /= frameTimes.size();
					float fps = 1000.0f / avgMs;

					ImGui::Text("Avg Frame Time: %.2f ms (%.1f FPS)", avgMs, fps);
					ImGui::PlotLines("Frame Time (ms)", frameTimes.data(), (int)frameTimes.size(),
						0, nullptr, 0.0f, 40.0f, ImVec2(0, 80));
				}

				ImGui::Separator();

				// ===== 2. Command History =====
				if (ImGui::CollapsingHeader("Command History", ImGuiTreeNodeFlags_DefaultOpen)) {

					if (!command_manager) {
						ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Command Manager not available");
					}
					else {
						size_t undo_count = command_manager->getUndoCount();
						size_t redo_count = command_manager->getRedoCount();

						// Summary bar
						ImGui::Text("Undo Stack: %zu / %zu", undo_count, command_manager->getMaxStackSize());
						ImGui::SameLine(200);
						ImGui::Text("Redo Stack: %zu", redo_count);

						ImGui::Spacing();
						ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
							"Ctrl+Z to Undo  |  Ctrl+Y to Redo");
						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();

						// Next action previews
						if (command_manager->canUndo()) {
							std::string desc = command_manager->getNextUndoDescription();
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
							ImGui::Text("Next Undo: %s", desc.empty() ? "[Unnamed]" : desc.c_str());
							ImGui::PopStyleColor();
						}
						else {
							ImGui::TextDisabled("Next Undo: N/A");
						}

						if (command_manager->canRedo()) {
							std::string desc = command_manager->getNextRedoDescription();
							ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
							ImGui::Text("Next Redo: %s", desc.empty() ? "[Unnamed]" : desc.c_str());
							ImGui::PopStyleColor();
						}
						else {
							ImGui::TextDisabled("Next Redo: N/A");
						}

						ImGui::Spacing();

						// ---- Full history view ----
						if (ImGui::TreeNode("Full History")) {
							ImGui::BeginChild("##HistoryView", ImVec2(0, 250), true);

							// -- Undo stack --
							// Deque: front = oldest, back = most recent (next to undo)
							const auto& undoHistory = command_manager->getUndoHistory();

							ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
								"Undo Stack (%zu)  [newest at top]", undoHistory.size());
							ImGui::Separator();

							if (undoHistory.empty()) {
								ImGui::TextDisabled("  [Empty]");
							}
							else {
								// Iterate back-to-front so newest appears at the top of the UI
								for (int i = (int)undoHistory.size() - 1; i >= 0; --i) {
									const auto& action = undoHistory[i];
									const std::string& desc = action.description.empty()
										? "[Unnamed Action]"
										: action.description;

									bool isNext = (i == (int)undoHistory.size() - 1);

									if (isNext) {
										// Highlight the action that would be undone next
										ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
										ImGui::BulletText("[NEXT] %s", desc.c_str());
										ImGui::PopStyleColor();
									}
									else {
										ImGui::BulletText("%s", desc.c_str());
									}
								}
							}

							ImGui::Spacing();
							ImGui::Separator();
							ImGui::Spacing();

							// -- Redo stack --
							const auto& redoHistory = command_manager->getRedoHistory();

							ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f),
								"Redo Stack (%zu)  [newest at top]", redoHistory.size());
							ImGui::Separator();

							if (redoHistory.empty()) {
								ImGui::TextDisabled("  [Empty]");
							}
							else {
								// Same — back-to-front so next redo is at the top
								for (int i = (int)redoHistory.size() - 1; i >= 0; --i) {
									const auto& action = redoHistory[i];
									const std::string& desc = action.description.empty()
										? "[Unnamed Action]"
										: action.description;

									bool isNext = (i == (int)redoHistory.size() - 1);

									if (isNext) {
										ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.3f, 1.0f));
										ImGui::BulletText("[NEXT] %s", desc.c_str());
										ImGui::PopStyleColor();
									}
									else {
										ImGui::BulletText("%s", desc.c_str());
									}
								}
							}

							ImGui::EndChild();
							ImGui::TreePop();
						}
					}
				}

				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif