#include "pch.h"
#include "ResourcePanel.h"

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include "CoreSystems/Assets/sPath.h"
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Assets/sLoader.h"

#define PN_PATH_SERVICE  services->get<Path::Service>()
#define PN_LOADER_SERVICE  services->get<Loader::Service>()
#define PN_ASSET_SERVICE  services->get<Assets::Service>()

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

            ResourcePanel::ResourcePanel() {
                name = "Resource Panel";

                flags = ImGuiWindowFlags_None;

                // Default icon size
                icon_size = { 128.0f, 128.0f };

                // Initialize root and current path
                std::filesystem::path actual_path = "assets/";
                root_path = actual_path.string();
                current_path = root_path;

                // Search filter
                search_filter.resize(32);
                search_filter = "";
            }

            void ResourcePanel::nextWindowSettings() {
                // Default behavior (no special fullscreen/docking hacks)
            }

            void ResourcePanel::onUpdate() {
                // Currently empty, could be used for background updates
                static bool initialized = false;
                if (!initialized) {
                    init();
                    initialized = true;
                }

                //render();
            }

            void ResourcePanel::init() {

                assert(services && "Services pointer is null in ResourcePanel::init()");
                auto assetService = services->get<Assets::Service>();
                assert(assetService && "Assets::Service not registered in ServiceContainer");

                // Register engine icons
                //PN_ASSET_SERVICE->scanAssetDirectory(current_path, false);
                // Initialize directories and files
                //directories = PN_PATH_SERVICE->listDirectories(current_path);
                //files = PN_PATH_SERVICE->listFiles(current_path);

                // Initialize popups
                error_msg = std::make_shared<std::string>("Error");
                success_msg = std::make_shared<std::string>("Success");

                /*registerPopUp("Error", defPopUp("Error", error_msg));
                registerPopUp("Success", defPopUp("Success", success_msg));
                registerPopUp("Delete Asset", deleteAssetPopup("Delete Asset"));
                registerPopUp("Clear Directory", deleteDirectoryPopup("Clear Directory"));
                registerPopUp("New Folder", newFolderPopup("New Folder"));*/

                // TODO: Setup directory watching if needed
            }

            void ResourcePanel::render() {
                // Handle file events
                while (!file_event_queue.empty()) {
                    try {
                        if (file_event_queue.front()) file_event_queue.front()();
                        file_event_queue.pop();
                    }
                    catch (std::exception const&) {
                        PN_CORE_WARN("Invalid Callback From FileWatcher Handled. Loop Continues.");
                    }
                }

                if (!ImGui::Begin(getName().c_str(), nullptr, ImGuiWindowFlags_MenuBar)) {
                    ImGui::End();
                    return;
                }

                ImGui::BeginMenuBar();

                // Back button
                if (!current_path.empty() && ImGui::Button("< Back")) {
                    if (current_path != root_path) {
                        current_path = PN_PATH_SERVICE->getVirtualParentPath(current_path);
                        directories = PN_PATH_SERVICE->listDirectories(current_path);
                        files = PN_PATH_SERVICE->listFiles(current_path);
                    }
                }

                moveFileAcceptPayload(PN_PATH_SERVICE->getVirtualParentPath(current_path));

                ImGui::Spacing();

                // New folder
                //if (ImGui::Button("New Folder")) openPopUp("New Folder");

                ImGui::Spacing();

                // Directory mode dropdown
                const char* load_directory[] = { "Current", "Current *", "Root *" };
                ImGui::PushItemWidth(100.0f);
                ImGui::Combo("##Directory", &directory_mode, load_directory, IM_ARRAYSIZE(load_directory));
                ImGui::PopItemWidth();

                // Load / Unload / Delete directory buttons
                if (ImGui::Button("Load All")) {
                    switch (directory_mode) {
                    case 0: PN_ASSET_SERVICE->cacheAssetDirectory(current_path, false); break;
                    case 1: PN_ASSET_SERVICE->cacheAssetDirectory(current_path, true); break;
                    case 2: PN_ASSET_SERVICE->cacheAssetDirectory(root_path, true); break;
                    }
                    success_msg->assign("Assets loaded.");
                    //openPopUp("Success");
                }

                if (ImGui::Button("Unload All")) {
                    switch (directory_mode) {
                    case 0: PN_ASSET_SERVICE->uncacheAssetDirectory(current_path, false); break;
                    case 1: PN_ASSET_SERVICE->uncacheAssetDirectory(current_path, true); break;
                    case 2: PN_ASSET_SERVICE->uncacheAssetDirectory(root_path, true); break;
                    }
                    success_msg->assign("Assets unloaded.");
                    //openPopUp("Success");
                }

                //if (ImGui::Button("Delete All")) openPopUp("Clear Directory");

                ImGui::Spacing();

                // Icon size
                ImGui::Text("Icon Size: ");
                ImGui::PushItemWidth(50.0f);
                ImGui::DragFloat("##IconSizing", &icon_size.x, 1.0f, 32.0f, 256.0f, "%.f", ImGuiSliderFlags_AlwaysClamp);
                ImGui::PopItemWidth();
                icon_size.y = icon_size.x;

                ImGui::Spacing();

                // Filter
                ImGui::Text("Filter: "); ImGui::SameLine();
                ImGui::PushItemWidth(100.0f);
                if (ImGui::InputTextWithHint("##SearchFilter", "Search...", search_filter.data(), search_filter.capacity() + 1))
                    search_filter.resize(strlen(search_filter.c_str()));
                ImGui::PopItemWidth();

                ImGui::Spacing();

                // Refresh
                if (ImGui::Button("Refresh")) {
                    directories = PN_PATH_SERVICE->listDirectories(current_path);
                    files = PN_PATH_SERVICE->listFiles(current_path);
                }

                // Render popups
                //renderPopUps();

                ImGui::EndMenuBar();

                // Render files & folders
                renderAssetsBrowser(current_path);

                ImGui::End();
            }

            unsigned int ResourcePanel::fileIcon(const std::filesystem::path& path)
            {
                if (!PN_ASSET_SERVICE->hasAssets())
                    return 0; // 0 = default box

                auto type = PN_ASSET_SERVICE->getAssetType(path);

                switch (type)
                {
                case Loader::Types::Texture: return 1; // texture box
                case Loader::Types::Sprite:  return 2; // sprite box
                case Loader::Types::Model:   return 3; // model box
                default:                     return 0; // default box
                }
            }



            void ResourcePanel::moveFileAcceptPayload(const std::string& virtual_path) {
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(payload_typestring + "_FILE").c_str())) {
                        std::string asset_id(static_cast<const char*>(payload->Data));
                        std::filesystem::path dest = PN_PATH_SERVICE->resolvePath(virtual_path) / asset_id;
                        std::filesystem::rename(PN_ASSET_SERVICE->getAssetPath(asset_id), dest);
                        files = PN_PATH_SERVICE->listFiles(current_path);
                    }
                    ImGui::EndDragDropTarget();
                }
            }

            void ResourcePanel::pushFileEvent(std::function<void()> callback) {
                std::lock_guard<std::mutex> lock(file_event_mutex);
                file_event_queue.push(std::move(callback));
            }

            void ResourcePanel::renderAssetsBrowser(const std::string& path)
            {
                for (const auto& file : files)
                {
                    unsigned int iconID = fileIcon(file);

                    // Draw a simple colored box as a placeholder
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 size = icon_size;

                    ImU32 color = IM_COL32(100, 100, 100, 255); // default gray
                    if (iconID == 1) color = IM_COL32(200, 100, 100, 255); // texture
                    if (iconID == 2) color = IM_COL32(100, 200, 100, 255); // sprite
                    if (iconID == 3) color = IM_COL32(100, 100, 200, 255); // model

                    ImGui::GetWindowDrawList()->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), color);

                    // Optionally, draw the file name below the box
                    ImGui::Dummy(size); // reserve space
                    ImGui::TextWrapped("%s", file.filename().string().c_str());
                }
            }

            // TODO: Implement popups (deleteAssetPopup, deleteDirectoryPopup, newFolderPopup)
            // Delete Asset Popup
            std::function<void()> ResourcePanel::deleteAssetPopup(const std::string& popup_id) {
                return [this, popup_id]() {
                    if (ImGui::BeginPopup(popup_id.c_str())) {
                        ImGui::Text("Are you sure you want to delete this asset?");
                        ImGui::Separator();

                        if (ImGui::Button("Yes", ImVec2(120, 0))) {
                            // TODO: Implement actual deletion logic
                            files = PN_PATH_SERVICE->listFiles(current_path); // refresh files
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("No", ImVec2(120, 0))) {
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }
                    };
            }

            // Delete Directory Popup
            std::function<void()> ResourcePanel::deleteDirectoryPopup(const std::string& popup_id) {
                return [this, popup_id]() {
                    if (ImGui::BeginPopup(popup_id.c_str())) {
                        ImGui::Text("Are you sure you want to delete this directory?");
                        ImGui::Separator();

                        if (ImGui::Button("Yes", ImVec2(120, 0))) {
                            // TODO: Implement actual directory deletion
                            directories = PN_PATH_SERVICE->listDirectories(current_path); // refresh dirs
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("No", ImVec2(120, 0))) {
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }
                    };
            }

            // New Folder Popup
            std::function<void()> ResourcePanel::newFolderPopup(const std::string& popup_id) {
                return [this, popup_id]() {
                    static char folder_name[64] = "";
                    if (ImGui::BeginPopup(popup_id.c_str())) {
                        ImGui::Text("Enter new folder name:");
                        ImGui::InputText("##NewFolderInput", folder_name, IM_ARRAYSIZE(folder_name));
                        ImGui::Separator();

                        if (ImGui::Button("Create", ImVec2(120, 0))) {
                            if (strlen(folder_name) > 0) {
                                // TODO: actually create folder on disk
                                PN_CORE_INFO("New folder requested: {}", folder_name);
                                directories = PN_PATH_SERVICE->listDirectories(current_path); // refresh dirs
                                folder_name[0] = '\0'; // clear input
                                ImGui::CloseCurrentPopup();
                            }
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                            folder_name[0] = '\0';
                            ImGui::CloseCurrentPopup();
                        }

                        ImGui::EndPopup();
                    }
                    };
            }

            // TODO: Implement file editor (TextCallback, extractCurrentWord, showLuaIntellisense)

            // Text callback for ImGuiInputText
            int ResourcePanel::TextCallback(ImGuiInputTextCallbackData* data) {
                // Currently no special callbacks, just return 0
                return 0;
            }

            // Extract the current word under the cursor
            void ResourcePanel::extractCurrentWord(const std::string& content, size_t cursor_pos, std::string& buffer) {
                buffer.clear();
                if (cursor_pos > content.size()) return;

                // Find the start of the word
                size_t start = cursor_pos;
                while (start > 0 && (isalnum(content[start - 1]) || content[start - 1] == '_')) start--;

                // Find the end of the word
                size_t end = cursor_pos;
                while (end < content.size() && (isalnum(content[end]) || content[end] == '_')) end++;

                buffer = content.substr(start, end - start);
            }

            // Simple Lua-style intellisense placeholder
            void ResourcePanel::showLuaIntellisense(std::string& content, size_t cursor_pos, std::string& buffer) {
                extractCurrentWord(content, cursor_pos, buffer);
                if (!buffer.empty()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Suggestions for: %s", buffer.c_str());
                    ImGui::Text("- print()");
                    ImGui::Text("- math.sqrt()");
                    ImGui::Text("- table.insert()");
                    ImGui::EndTooltip();
                }
            }

            // Render a simple file editor
            void ResourcePanel::renderFileEditor() {
                static std::string file_content = "";
                static ImGuiInputTextFlags flags = ImGuiInputTextFlags_CallbackAlways;

                ImGui::Text("File Editor");
                ImGui::Separator();

                ImGui::InputTextMultiline(
                    "##FileEditor",
                    file_content.data(),
                    file_content.capacity() + 1,
                    ImVec2(-1, 300),
                    flags,
                    TextCallback
                );

                size_t cursor_pos = ImGui::GetCursorPos().x; // optional, can be improved
                std::string current_word;
                showLuaIntellisense(file_content, cursor_pos, current_word);
            }


        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
