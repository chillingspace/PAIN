#pragma once

#ifdef _DEBUG

#include "Panels.h"
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Path/Path.h"
#include <chrono>
#include <queue>

// ========================================
// Move File and Dir structs OUTSIDE platform guards
// so they're accessible to other headers
// ========================================
namespace PAIN::Editor::Panel {

    //Global files and directories for payload
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

        // Use GUID for equality (most efficient)
        bool operator==(const File& other) const {
            return id == other.id;
        }
    };
}

namespace std {
    template<>
    struct hash<PAIN::Editor::Panel::File> {
        size_t operator()(const PAIN::Editor::Panel::File& file) const noexcept {
            // Assuming GUID already has a hash function
            return std::hash<PAIN::Assets::GUID>{}(file.id);
        }
    };
}

// ========================================
// Platform-specific ResourcePanel class
// ========================================
#ifdef PN_PLATFORM_WINDOWS

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
                void pushFileEvent(std::filesystem::path const& file, filewatch::Event const& event, std::function<void()>&& callback); //Thread safe insertion for file event queue
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
                struct EventEntry {
                    std::unordered_map<filewatch::Event, std::function<void()>> callback;
                    std::chrono::steady_clock::time_point last_updated;
                };
                std::queue<std::function<void()>> file_event_queue;
                std::unordered_map<std::filesystem::path, EventEntry> event_functions;
                std::mutex file_event_mutex;
                const std::chrono::milliseconds debounce_time{ 200 };

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
                std::unordered_map<std::filesystem::path, std::vector<std::filesystem::path>> directoryCache;

                void populateDirectoryCache(std::filesystem::path const& path);

                void DrawDirectoryTree(std::filesystem::path const& path);
                bool renderPopUpContext(File const& file);
                bool renderPopUpContext(Dir const& file);
                bool renderPopUpContext(std::string const& virtual_path);
                unsigned int fileIcon(std::filesystem::path const& relative_path); //Internal asset icon picking
                void renderAssetsBrowser(std::string const& virtual_path); //Internal rendering of an asset browser

                // ----------------------------
                // File Render
                // ----------------------------
                class MaterialPreview {
                private:
                    unsigned int preview_fbo = 0;
                    unsigned int preview_texture = 0;
                    unsigned int preview_depth_rbo = 0;
                    const glm::ivec2 preview_size{ 512, 512 };

                    struct PreviewSettings {
                        // Camera
                        float camera_distance = 3.0f;
                        float fov = 35.0f;

                        // Rotation
                        float rotation_y = -25.0f;
                        float rotation_x = 0.0f;

                        // Projection
                        bool use_orthographic = true;
                        float ortho_size = 1.1f;

                        // Lighting
                        float ambient_intensity = 0.15f;
                        float light_intensity = 15.0f;
                        glm::vec3 light_position = glm::vec3(2.0f, 3.0f, 2.0f);

                        //Preview disply size
                        glm::ivec2 display_size{ 256, 256 };
                    };

                    //Static sphere model
                    static std::shared_ptr<Assets::Model> sphere_model;

                    //Static shader
                    static std::shared_ptr<Assets::Shader> shader;

                    //Services
                    static std::weak_ptr<Services> services;

                    //Simple sphere mesh for preview
                    static unsigned int sphere_vao;
                    static unsigned int sphere_vbo;
                    static unsigned int sphere_ebo;

                public:

                    //Material preview contructor
                    MaterialPreview();

                    //Set preview settings
                    PreviewSettings preview_settings;

                    //Called only once for statics
                    void init(std::weak_ptr<Services> service);
                    void render(std::shared_ptr<const Assets::Material> material);
                    unsigned int getPreviewTexture() const {
                        return preview_texture;
                    }
                };

                //Create mat preview
                std::unordered_map<Assets::GUID, MaterialPreview> mat_previews;

                //Render material file functionality
                void renderMaterial(std::shared_ptr<Assets::Material> material, File const& file);

                //Open files manager
                const int MAX_FILES_OPEN = 5;
                std::unordered_set<File> open_files;
                void addFilesToOpen(File const& file);
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
                std::function<void(std::any const&)> newMaterialPopup(std::string const& popup_id);

                // ----------------------------
                // File Operations
                // ----------------------------
                void moveFileAcceptPayload(std::string const& virtual_path); //Moving file accept payload
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif // PN_PLATFORM_WINDOWS
#endif // _DEBUG
