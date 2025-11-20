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

			void DebugPanel::nextWindowSettings() {
			}

			void DebugPanel::onAttach() {
			}

			void DebugPanel::onUpdate(AppTiming timing) {
				ImGui::Begin(name.c_str(), nullptr, flags);

				// ===== 1. Performance Visualizer =====
				if (ImGui::CollapsingHeader("Performance Metrics", ImGuiTreeNodeFlags_DefaultOpen)) {
					// Keep a rolling history of frame times (100 samples)
					const int MAX_SAMPLES = 100;
					frameTimes.push_back(timing.dt * 1000.0f); // ms
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

				// ===== 2. Command History View =====
				if (ImGui::CollapsingHeader("Command History", ImGuiTreeNodeFlags_DefaultOpen)) {

					if (!command_manager) {
						ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "Command Manager not available");
					}
					else {
						// Display stack information
						size_t undo_count = command_manager->getUndoCount();
						size_t redo_count = command_manager->getRedoCount();

						ImGui::Text("Undo Stack: %zu action(s)", undo_count);
						ImGui::Text("Redo Stack: %zu action(s)", redo_count);

						ImGui::Spacing();

						// Keyboard shortcuts note
						ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Use Ctrl+Z to Undo | Ctrl+Y to Redo");

						ImGui::Spacing();
						ImGui::Separator();

						// Display next action descriptions
						if (command_manager->canUndo()) {
							std::string next_undo = command_manager->getNextUndoDescription();
							if (!next_undo.empty()) {
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
								ImGui::Text("Next Undo: %s", next_undo.c_str());
								ImGui::PopStyleColor();
							}
							else {
								ImGui::TextDisabled("Next Undo: [Unnamed Action]");
							}
						}
						else {
							ImGui::TextDisabled("Next Undo: N/A");
						}

						if (command_manager->canRedo()) {
							std::string next_redo = command_manager->getNextRedoDescription();
							if (!next_redo.empty()) {
								ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
								ImGui::Text("Next Redo: %s", next_redo.c_str());
								ImGui::PopStyleColor();
							}
							else {
								ImGui::TextDisabled("Next Redo: [Unnamed Action]");
							}
						}
						else {
							ImGui::TextDisabled("Next Redo: N/A");
						}

						ImGui::Spacing();

						// Optional: Visual representation
						if (ImGui::TreeNode("Visual Stack View")) {
							ImGui::BeginChild("StackView", ImVec2(0, 200), true);

							// Undo stack visualization
							ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Undo Stack (Top -> Bottom):");

							if (undo_count == 0) {
								ImGui::TextDisabled("  [Empty]");
							}
							else {
								// Show limited items (top 15)
								size_t display_count = std::min(undo_count, (size_t)15);
								for (size_t i = 0; i < display_count; ++i) {
									if (i == 0) {
										ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
										ImGui::BulletText("[NEXT] Position %zu", undo_count - i);
										ImGui::PopStyleColor();
									}
									else {
										ImGui::BulletText("Position %zu", undo_count - i);
									}
								}

								if (undo_count > 15) {
									ImGui::TextDisabled("  ... and %zu more", undo_count - 15);
								}
							}

							ImGui::Spacing();
							ImGui::Separator();
							ImGui::Spacing();

							// Redo stack visualization
							ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "Redo Stack (Top -> Bottom):");

							if (redo_count == 0) {
								ImGui::TextDisabled("  [Empty]");
							}
							else {
								// Show limited items (top 15)
								size_t display_count = std::min(redo_count, (size_t)15);
								for (size_t i = 0; i < display_count; ++i) {
									if (i == 0) {
										ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.4f, 1.0f));
										ImGui::BulletText("[NEXT] Position %zu", redo_count - i);
										ImGui::PopStyleColor();
									}
									else {
										ImGui::BulletText("Position %zu", redo_count - i);
									}
								}

								if (redo_count > 15) {
									ImGui::TextDisabled("  ... and %zu more", redo_count - 15);
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
