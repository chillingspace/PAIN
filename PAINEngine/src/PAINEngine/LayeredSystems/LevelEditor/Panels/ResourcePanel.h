#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG

#pragma once
#include "Panels.h"

namespace PAIN {
    namespace Editor {
        namespace Panel {

            class ResourcePanel : public IPanel {
            public:

                // ----------------------------
                // Core Overrides
                // ----------------------------
                ResourcePanel();
                ~ResourcePanel() override = default;
                void nextWindowSettings() override;
                void onUpdate(AppTiming timing) override;

                // ----------------------------
                // Life Cycle
                // ----------------------------

                void onAttach() override;
                void render();

                // ----------------------------
                // Panel Info
                // ----------------------------
                std::string getName() const { // Get Panel Name
                    return "Resource Management";
                }
                static std::string getStaticName() { //Static panel name
                    return "Resource Management";
                }

                // ----------------------------
                // File Event Queue
                // ----------------------------
                void pushFileEvent(std::function<void()> callback); //Thread safe insertion for file event queue
                void onEvent(PAIN::Event::Event& event);

            private:

                // ----------------------------
                // File & Directory
                // ----------------------------
                std::vector<std::filesystem::path> directories; //Directories
                std::vector<std::filesystem::path> files; //Files

                std::string root_path; //Root Path
                std::string current_path; //Current Path

                // ----------------------------
                // State Variables
                // ----------------------------
                std::string search_filter; //Search filter
                ImVec2 icon_size; //Icon size

                std::string selected_asset_id; //Selected file
                std::string payload_typestring; //File payload type string

                int directory_mode; //Selected directory mode
                bool b_file_dropped; //File dropped


                // ----------------------------
                // File
                // ----------------------------
                std::queue<std::function<void()>> file_event_queue; //File Watching Queue
                std::mutex file_event_mutex; //Mutex for thread safety

                std::unordered_map<std::string, std::string> file_editing_map; //Map of file content

                struct EditorState { //File editor state
                    int cursor_pos = 0;
                };

                static int TextCallback(ImGuiInputTextCallbackData* data); //Text callback
                void extractCurrentWord(std::string const& content, size_t cursor_pos, std::string& buffer); //Extract current word being edited
                void showLuaIntellisense(std::string& content, size_t cursor_pos, std::string& buffer); //Lua intellisense


                // ----------------------------
                // Feedback
                // ----------------------------
                std::shared_ptr<std::string> error_msg; //Setting error message ( Usage: Editing error popup message )
                std::shared_ptr<std::string> success_msg; //Setting success message ( Usage: Editing success popup message )


                // ----------------------------
                // Internal Helpers
                // ----------------------------
                unsigned int fileIcon(std::filesystem::path const& path); //Internal asset icon picking
                unsigned int isTextureLoaded(const char* file_path, const std::string& ref);
                void renderAssetsBrowser(std::string const& virtual_path); //Internal rendering of an asset browser
                void renderFileEditor(); //Internal rendering of a file editor

                // ----------------------------
                // Popups
                // ----------------------------
                std::function<void()> deleteAssetPopup(std::string const& popup_id); //Delete asset popup
                std::function<void()> deleteDirectoryPopup(std::string const& popup_id); //Delete directory content popup
                std::function<void()> newFolderPopup(std::string const& popup_id); //New folder popup


                // ----------------------------
                // File Operations
                // ----------------------------
                void moveFileAcceptPayload(std::string const& virtual_path); //Moving file accept payload

                //Entities panel for string reference
                //std::weak_ptr<EntitiesPanel> entities_panel;
                //On drop file event
                //void onEvent(std::shared_ptr<Assets::FileDropEvent> event) override;
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif
