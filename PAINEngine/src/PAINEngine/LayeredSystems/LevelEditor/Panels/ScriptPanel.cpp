/*****************************************************************/ /**
 * \file   ScriptPanel.cpp
 * \brief
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 *
 * \date   March 2026
 * All content  2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#ifdef _DEBUG

#include "pch.h"
#include "ScriptPanel.h"
#include "../Editor.h"

#include <fstream>
#include <sstream>

namespace PAIN {
    namespace Editor {
        namespace Panel {

            ScriptPanel::ScriptPanel() {
                name = "Script Editor";
                always_active = true;
            }

            void ScriptPanel::nextWindowSettings() {
                // Let it be freely dockable and resizable - no special constraints
            }

            void ScriptPanel::onAttach() {
                // Nothing to initialize - tabs are opened on demand
            }

            void ScriptPanel::onUpdate(AppTiming timing) {

                if (ImGui::Begin("Script Editor", nullptr,
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoScrollWithMouse |
                    ImGuiWindowFlags_MenuBar)) {

                    renderMenuBar();

                    // No scripts open
                    if (m_tabs.empty()) {
                        ImVec2 avail = ImGui::GetContentRegionAvail();
                        ImGui::SetCursorPos(ImVec2(
                            avail.x * 0.5f - 120.0f,
                            avail.y * 0.5f - 10.0f));
                        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f),
                            "No scripts open. Double-click a script in the Resource Panel.");
                        ImGui::End();
                        return;
                    }

                    if (!m_tabs.empty() && m_active_tab >= 0 && m_active_tab < (int)m_tabs.size()) {
                        ScriptTab& active = m_tabs[m_active_tab];

                        // Save
                        bool can_save = active.is_dirty;
                        if (!can_save) ImGui::BeginDisabled();
                        if (ImGui::Button("Save")) saveTab(active);
                        if (!can_save) ImGui::EndDisabled();

                        ImGui::SameLine();

                        // Save All
                        bool any_dirty = false;
                        for (auto& t : m_tabs) if (t.is_dirty) { any_dirty = true; break; }
                        if (!any_dirty) ImGui::BeginDisabled();
                        if (ImGui::Button("Save All")) {
                            for (auto& t : m_tabs) if (t.is_dirty) saveTab(t);
                        }
                        if (!any_dirty) ImGui::EndDisabled();

                        ImGui::SameLine();
                        if (ImGui::Button("Close")) m_tabs[m_active_tab].pending_close = true;

                        ImGui::SameLine();
                        if (ImGui::Button("Open in VS Code")) {
                            std::string command = "code \"" + active.filepath + "\"";
                            system(command.c_str());
                        }

                        ImGui::SameLine();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        ImGui::Text("  %s", active.filepath.c_str());
                        ImGui::PopStyleColor();

                        ImGui::Separator();
                    }

                    // Render tab bar
                    if (ImGui::BeginTabBar("##ScriptTabs",
                        ImGuiTabBarFlags_Reorderable |
                        ImGuiTabBarFlags_AutoSelectNewTabs)) {

                        for (int i = 0; i < (int)m_tabs.size(); ++i) {
                            ScriptTab& tab = m_tabs[i];

                            // Tab title - show * if unsaved
                            std::string tab_title = tab.filename;
                            if (tab.is_dirty) tab_title += " *";
                            tab_title += "##" + tab.filepath;

                            // Tab flags
                            ImGuiTabItemFlags tab_flags = ImGuiTabItemFlags_None;
                            if (tab.just_opened) {
                                tab_flags |= ImGuiTabItemFlags_SetSelected;
                                tab.just_opened = false;
                            }

                            bool tab_open = true;
                            if (ImGui::BeginTabItem(tab_title.c_str(), &tab_open, tab_flags)) {
                                m_active_tab = i;
                                renderTabContent(tab);
                                ImGui::EndTabItem();
                            }

                            // Tab was closed via X button
                            if (!tab_open) {
                                // If dirty, could prompt save - for now just mark pending close
                                tab.pending_close = true;
                            }
                        }

                        ImGui::EndTabBar();
                    }

                    // Remove tabs marked for close (iterate in reverse to preserve indices)
                    for (int i = (int)m_tabs.size() - 1; i >= 0; --i) {
                        if (m_tabs[i].pending_close) {
                            m_tabs.erase(m_tabs.begin() + i);
                            if (m_active_tab >= (int)m_tabs.size())
                                m_active_tab = (int)m_tabs.size() - 1;
                        }
                    }
                }
                ImGui::End();
            }

            void ScriptPanel::renderMenuBar() {
                if (!ImGui::BeginMenuBar()) return;

                if (!m_tabs.empty()) {
                    if (m_active_tab < 0 || m_active_tab >= (int)m_tabs.size()) {
                         ImGui::EndMenuBar();
                         return;
                    }
                    ScriptTab& active = m_tabs[m_active_tab];

                    // Save button - greyed out if no changes
                    if (!active.is_dirty) ImGui::BeginDisabled();
                    if (ImGui::Button("Save") ||
                        (!ImGui::GetIO().WantTextInput &&
                         (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) &&
                         ImGui::IsKeyPressed(ImGuiKey_S))) {
                        saveTab(active);
                    }
                    if (!active.is_dirty) ImGui::EndDisabled();

                    ImGui::SameLine();

                    // Save All button
                    bool any_dirty = false;
                    for (auto& t : m_tabs) if (t.is_dirty) { any_dirty = true; break; }
                    if (!any_dirty) ImGui::BeginDisabled();
                    if (ImGui::Button("Save All")) {
                        for (auto& t : m_tabs) {
                            if (t.is_dirty) saveTab(t);
                        }
                    }
                    if (!any_dirty) ImGui::EndDisabled();

                    ImGui::SameLine();

                    // Close current tab
                    if (ImGui::Button("Close")) {
                        m_tabs[m_active_tab].pending_close = true;
                    }

                    ImGui::SameLine();

                    // Open in VS Code
                    if (ImGui::Button("Open in VS Code")) {
                        std::string command = "code \"" + active.filepath + "\"";
                        system(command.c_str());
                    }

                    ImGui::SameLine();

                    // Show current file path in muted text
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::Text("  %s", active.filepath.c_str());
                    ImGui::PopStyleColor();
                }

                ImGui::EndMenuBar();
            }

            void ScriptPanel::renderTabContent(ScriptTab& tab) {

                // Available space for the editor
                ImVec2 avail = ImGui::GetContentRegionAvail();

                // Reserve bottom bar height
                float bottom_bar_height = 28.0f;
                ImVec2 editor_size(avail.x, avail.y - bottom_bar_height);

                // Text editor
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.10f, 0.10f, 0.12f, 1.0f));
                ImGui::PushFont(ImGui::GetIO().Fonts->Fonts.Size > 1
                    ? ImGui::GetIO().Fonts->Fonts[1]   // Use monospace font if available (index 1)
                    : ImGui::GetIO().Fonts->Fonts[0]);  // Fallback to default

                if (ImGui::InputTextMultiline(
                    ("##editor_" + tab.filepath).c_str(),
                    tab.buffer.data(),
                    tab.buffer.size(),
                    editor_size,
                    ImGuiInputTextFlags_AllowTabInput)) {
                    tab.is_dirty = true;
                }

                ImGui::PopFont();
                ImGui::PopStyleColor();

                // Bottom status bar
                ImGui::Separator();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

                // Line/column count (approximate from buffer)
                int line_count = 1;
                int char_count = 0;
                for (char c : tab.buffer) {
                    if (c == '\0') break;
                    if (c == '\n') ++line_count;
                    ++char_count;
                }
                ImGui::Text("Lines: %d  |  Chars: %d  |  %s",
                    line_count,
                    char_count,
                    tab.is_dirty ? "Unsaved changes" : "Saved");

                ImGui::PopStyleColor();
            }

            void ScriptPanel::openScript(const std::string& filepath) {
                // Don't open duplicates - just focus existing tab
                if (isFileOpen(filepath)) {
                    for (int i = 0; i < (int)m_tabs.size(); ++i) {
                        if (m_tabs[i].filepath == filepath) {
                            m_active_tab = i;
                            m_tabs[i].just_opened = true;
                            return;
                        }
                    }
                }

                // Create new tab
                ScriptTab tab;
                tab.filepath = filepath;
                tab.filename = std::filesystem::path(filepath).filename().string();
                tab.buffer.resize(ScriptTab::BUFFER_SIZE, '\0');
                tab.just_opened = true;

                loadFileIntoTab(tab);

                m_tabs.push_back(std::move(tab));
                m_active_tab = (int)m_tabs.size() - 1;
            }

            void ScriptPanel::loadFileIntoTab(ScriptTab& tab) {
                std::ifstream file(tab.filepath, std::ios::in | std::ios::binary);
                if (!file) {
                    PN_CORE_WARN("[ScriptPanel] Failed to open file: {}", tab.filepath);
                    return;
                }

                // Check file fits in buffer
                file.seekg(0, std::ios::end);
                std::streamsize file_size = file.tellg();
                file.seekg(0, std::ios::beg);

                if (file_size >= (std::streamsize)tab.buffer.size()) {
                    PN_CORE_WARN("[ScriptPanel] File too large: {} ({} bytes)", tab.filepath, file_size);
                    std::string msg = "-- File too large to edit here. Open in VS Code instead.";
                    std::copy(msg.begin(), msg.end(), tab.buffer.begin());
                    tab.buffer[msg.size()] = '\0';
                    return;
                }

                std::fill(tab.buffer.begin(), tab.buffer.end(), '\0');
                file.read(tab.buffer.data(), file_size);
                tab.buffer[file.gcount()] = '\0';

                tab.is_dirty = false;
                PN_CORE_INFO("[ScriptPanel] Opened: {} ({} bytes)", tab.filepath, file_size);
            }

            void ScriptPanel::saveTab(ScriptTab& tab) {
                std::ofstream file(tab.filepath, std::ios::out | std::ios::binary);
                if (!file) {
                    PN_CORE_WARN("[ScriptPanel] Failed to save file: {}", tab.filepath);
                    return;
                }

                file.write(tab.buffer.data(), (std::streamsize)strlen(tab.buffer.data()));
                tab.is_dirty = false;
                PN_CORE_INFO("[ScriptPanel] Saved: {}", tab.filepath);
            }

            bool ScriptPanel::isFileOpen(const std::string& filepath) const {
                for (const auto& tab : m_tabs) {
                    if (tab.filepath == filepath) return true;
                }
                return false;
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif