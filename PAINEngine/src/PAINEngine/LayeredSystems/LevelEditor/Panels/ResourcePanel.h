#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG

#pragma once
#include "Panels.h"

#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Path/Path.h"

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
                void onDetach() override;
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
                void onEvent(Event::Event& event) override;

            private:

                // ----------------------------
                // File & Directory
                // ----------------------------
                std::shared_ptr<Path::Path> path_service;
                std::shared_ptr<Assets::Manager> asset_service;

                // ----------------------------
                // File & Directory
                // ----------------------------
                struct Dir {
                    std::filesystem::path path;
                    ImTextureID icon;
                    std::string file_name;
                };
                struct File {
                    std::filesystem::path path;
                    Assets::GUID id;
                    Assets::Type type;
                    ImTextureID icon;
                    std::string file_name;
                };
                std::vector<Dir> directories; //Directories
                std::vector<File> files; //Files

                //Function to population files
                void populateDirs(std::string const& virtual_path);
                void populateFiles(std::string const& virtual_path);

                std::string root_path; //Root Path
                std::filesystem::path root;
                std::string current_path; //Current Path

                // ----------------------------
                // State Variables
                // ----------------------------
                std::string search_filter; //Search filter
                ImVec2 icon_size; //Icon size

                //Payload
                std::string payload_typestring;

                //Vector of opened files
                std::vector<File> open_files;

                //Assets auto refresh timer
                float auto_refresh_timer = 0.0f;
                const float AUTO_REFRESH_INTERVAL = 2.0f;

                //Side bar size ratio
                const float SIDE_BAR_RATIO = 0.2f;

                //Show case file option
                bool b_show_desc_files = false;

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
                // Internal Helpers
                // ----------------------------
                std::unordered_map<std::string, std::vector<std::string>> directoryCache;

                void populateDirectoryCache(std::string const& virtual_dir);

                void DrawDirectoryTree(std::string const& virtual_dir);
                bool renderPopUpContext(File const& file);
                bool renderPopUpContext(Dir const& file);
                unsigned int fileIcon(std::filesystem::path const& relative_path); //Internal asset icon picking
                void renderAssetsBrowser(std::string const& virtual_path); //Internal rendering of an asset browser

                void renderOpenFiles();
                void renderFileEditor(); //Internal rendering of a file editor

                // ----------------------------
                // Popups
                // ----------------------------
                std::function<void(std::any const&)> deleteFilePopup(std::string const& popup_id);
                std::function<void(std::any const&)> renameFilePopup(std::string const& popup_id);
                std::function<void(std::any const&)> deleteFolderPopup(std::string const& popup_id);
                std::function<void(std::any const&)> renameFolderPopup(std::string const& popup_id);
                std::function<void(std::any const&)> newFolderPopup(std::string const& popup_id);


                // ----------------------------
                // File Operations
                // ----------------------------
                void moveFileAcceptPayload(std::string const& virtual_path); //Moving file accept payload
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif
