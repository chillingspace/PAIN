#include "pch.h"
#include "DebugPanel.h"
#include "LayeredSystems/LevelEditor/EditorTheme.h"
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
						ImGui::TextColored(Theme::current().textError, "Command Manager not available");
					}
					else {
						size_t undo_count = command_manager->getUndoCount();
						size_t redo_count = command_manager->getRedoCount();

						// Summary bar
						ImGui::Text("Undo Stack: %zu / %zu", undo_count, command_manager->getMaxStackSize());
						ImGui::SameLine(200);
						ImGui::Text("Redo Stack: %zu", redo_count);

						ImGui::Spacing();
						ImGui::TextColored(Theme::current().textMuted,
							"Ctrl+Z to Undo  |  Ctrl+Y to Redo");
						ImGui::Spacing();
						ImGui::Separator();
						ImGui::Spacing();

						// Next action previews
						if (command_manager->canUndo()) {
							std::string desc = command_manager->getNextUndoDescription();
							ImGui::PushStyleColor(ImGuiCol_Text, Theme::current().historyUndo);
							ImGui::Text("Next Undo: %s", desc.empty() ? "[Unnamed]" : desc.c_str());
							ImGui::PopStyleColor();
						}
						else {
							ImGui::TextDisabled("Next Undo: N/A");
						}

						if (command_manager->canRedo()) {
							std::string desc = command_manager->getNextRedoDescription();
							ImGui::PushStyleColor(ImGuiCol_Text, Theme::current().historyRedo);
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

							ImGui::TextColored(Theme::current().historyUndo,
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
										ImGui::PushStyleColor(ImGuiCol_Text, Theme::current().historyNext);
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

							ImGui::TextColored(Theme::current().historyRedo,
								"Redo Stack (%zu)  [newest at top]", redoHistory.size());
							ImGui::Separator();

							if (redoHistory.empty()) {
								ImGui::TextDisabled("  [Empty]");
							}
							else {
								// Same - back-to-front so next redo is at the top
								for (int i = (int)redoHistory.size() - 1; i >= 0; --i) {
									const auto& action = redoHistory[i];
									const std::string& desc = action.description.empty()
										? "[Unnamed Action]"
										: action.description;

									bool isNext = (i == (int)redoHistory.size() - 1);

									if (isNext) {
										ImGui::PushStyleColor(ImGuiCol_Text, Theme::current().historyNext);
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

				ImGui::Separator();

				// ===== 3. Log Console =====
				if (ImGui::CollapsingHeader("Log Console", ImGuiTreeNodeFlags_DefaultOpen)) {

					ImGui::PushStyleColor(ImGuiCol_Text, Theme::current().logTrace);
					ImGui::Checkbox("Trace##logfilter", &log_filter_trace);
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, Theme::current().logInfo);
					ImGui::Checkbox("Info##logfilter", &log_filter_info);
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, Theme::current().logWarn);
					ImGui::Checkbox("Warn##logfilter", &log_filter_warn);
					ImGui::PopStyleColor();
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, Theme::current().logError);
					ImGui::Checkbox("Error##logfilter", &log_filter_error);
					ImGui::PopStyleColor();
					ImGui::SameLine();
					if (ImGui::SmallButton("Clear"))
						DebugConsoleSink::clear();
					ImGui::SameLine();
					ImGui::Checkbox("Auto-scroll", &log_scroll_to_bottom);

					ImGui::Separator();

					if (ImGui::BeginChild("##LogConsole", ImVec2(0, 200), true,
						ImGuiWindowFlags_HorizontalScrollbar)) {

						std::lock_guard<std::mutex> lock(DebugConsoleSink::getEntriesMutex());
						for (const auto& entry : DebugConsoleSink::getEntries()) {

							if (entry.level == spdlog::level::trace && !log_filter_trace) continue;
							if (entry.level == spdlog::level::info && !log_filter_info)  continue;
							if (entry.level == spdlog::level::warn && !log_filter_warn)  continue;
							if ((entry.level == spdlog::level::err ||
								entry.level == spdlog::level::critical) && !log_filter_error) continue;

							ImVec4 color;
							const auto& th = Theme::current();
							switch (entry.level) {
							case spdlog::level::trace:    color = th.logTrace;    break;
							case spdlog::level::info:     color = th.logInfo;     break;
							case spdlog::level::warn:     color = th.logWarn;     break;
							case spdlog::level::err:      color = th.logError;    break;
							case spdlog::level::critical: color = th.logCritical; break;
							default:                      color = th.textDefault; break;
							}

							ImGui::PushStyleColor(ImGuiCol_Text, color);
							ImGui::TextUnformatted(entry.message.c_str());
							ImGui::PopStyleColor();
						}

						if (log_scroll_to_bottom)
							ImGui::SetScrollHereY(1.0f);
					}
					ImGui::EndChild();
				}

				ImGui::End();
			}

		} // namespace Panel
	} // namespace Editor
} // namespace PAIN

#endif