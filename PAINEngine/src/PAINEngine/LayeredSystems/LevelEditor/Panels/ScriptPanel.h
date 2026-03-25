/*****************************************************************/ /**
 * \file   ScriptPanel.h
 * \brief
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 *
 * \date   March 2026
 * All content  2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifdef _DEBUG
#ifndef SCRIPT_PANEL_HPP
#define SCRIPT_PANEL_HPP

#include "Panels.h"
#include <string>
#include <vector>
#include <filesystem>

namespace PAIN {
    namespace Editor {
        namespace Panel {

            class ScriptPanel : public IPanel {
            public:

                ScriptPanel();

                void onAttach() override;
                void onUpdate(PAIN::AppTiming timing) override;
                void nextWindowSettings() override;

                // Open a script file in the panel (called from ResourcePanel)
                void openScript(const std::string& filepath);

            private:

                // Represents a single open script tab
                struct ScriptTab {
                    std::string filepath;           // Full path to the script file
                    std::string filename;           // Display name (filename only)
                    std::vector<char> buffer;       // Edit buffer
                    bool is_dirty = false;          // Has unsaved changes
                    bool pending_close = false;     // Marked for close this frame
                    bool just_opened = false;       // Focus tab on first frame

                    static constexpr size_t BUFFER_SIZE = 1024 * 1024; // 1MB per script
                };

                std::vector<ScriptTab> m_tabs;  // All open script tabs
                int m_active_tab = 0;           // Currently focused tab index

                // Load file contents into a tab buffer
                void loadFileIntoTab(ScriptTab& tab);

                // Save a tab buffer back to disk
                void saveTab(ScriptTab& tab);

                // Render the top menu bar
                void renderMenuBar();

                // Render a single tab editor content
                void renderTabContent(ScriptTab& tab);

                // Check if a filepath is already open in a tab
                bool isFileOpen(const std::string& filepath) const;
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif