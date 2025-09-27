#pragma once
#include "Panels.h"

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

            class ResourcePanel : public IPanel {
            public:
                ResourcePanel();
                void nextWindowSettings() override;
                void onUpdate() override;

                void init();
                void render();

                //Panel Name
                std::string getName() const {
                    return "Resouce Editor";
                }

            private:
                // Paths
                std::string root_path;
                std::string current_path;

                // File and directory lists
                std::vector<std::filesystem::path> directories;
                std::vector<std::filesystem::path> files;

                // Icon size
                ImVec2 icon_size;

                // Selected asset
                std::string selected_asset_id;

                // Search filter
                std::string search_filter;

                // Drag payload type
                std::string payload_typestring;

                // Directory mode
                int directory_mode = 0;

                // File event queue
                std::mutex file_event_mutex;
                std::queue<std::function<void()>> file_event_queue;

                // File editing
                std::unordered_map<std::string, std::string> file_editing_map;

                // Popups
                std::shared_ptr<std::string> error_msg;
                std::shared_ptr<std::string> success_msg;

                // ImGui dock ID
                ImGuiID dock_id;

                // File dropped flag
                bool b_file_dropped = false;

            private:
                // File icon
                unsigned int fileIcon(const std::filesystem::path& path);

                // Drag-and-drop file handling
                void moveFileAcceptPayload(const std::string& virtual_path);

                // File event queue helper
                void pushFileEvent(std::function<void()> callback);

                // Render all assets
                void renderAssetsBrowser(const std::string& virtual_path);

                // Popups
                std::function<void()> deleteAssetPopup(const std::string& popup_id);
                std::function<void()> deleteDirectoryPopup(const std::string& popup_id);
                std::function<void()> newFolderPopup(const std::string& popup_id);

                // File editor helpers
                static int TextCallback(ImGuiInputTextCallbackData* data);
                void extractCurrentWord(const std::string& content, size_t cursor_pos, std::string& buffer);
                void showLuaIntellisense(std::string& content, size_t cursor_pos, std::string& buffer);
                void renderFileEditor();
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
