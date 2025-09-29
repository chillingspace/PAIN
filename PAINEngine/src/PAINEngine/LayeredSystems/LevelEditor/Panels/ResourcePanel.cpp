#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG

#include "pch.h"
#include "ResourcePanel.h"

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include "CoreSystems/Assets/sPath.h"
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Assets/sLoader.h"

#define PN_PATH_SERVICE  services->get<Path::Service>()
#define PN_LOADER_SERVICE  services->get<Loader::Service>()
#define PN_ASSETS_SERVICE  services->get<Assets::Service>()

namespace PAIN {
    namespace Editor {
        namespace Panel {

            ResourcePanel::ResourcePanel() {
                name = "Resource Panel";

                //Set panel flag
                flags = ImGuiWindowFlags_MenuBar;

                // Default icon size
                icon_size = { 128.0f, 128.0f };

                // Initialize root and current path
                root_path = "Game_Assets:/";
                current_path = root_path;

                // Search filter
                search_filter.resize(32);
                search_filter = "";
            }

            void ResourcePanel::nextWindowSettings() {
                // Default behavior (no special fullscreen/docking hacks)
            }

            void ResourcePanel::onUpdate(AppTiming timing) {
                //// Currently empty, could be used for background updates
                static bool initialized = false;
                if (!initialized) {
                    init();
                    initialized = true;
                }

				render();
            }

			void ResourcePanel::moveFileAcceptPayload(std::string const& virtual_path) {
				//Drop target
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(payload_typestring + "_FILE").c_str())) {
						//Get asset ID
						std::string asset_id(static_cast<const char*>(payload->Data));

						//Craft the destination path
						std::filesystem::path dest_path = PN_PATH_SERVICE->resolvePath(virtual_path) / asset_id;

						//Copy file
						std::filesystem::rename(PN_ASSETS_SERVICE->getAssetPath(asset_id), dest_path);

						//Update files
						files = PN_PATH_SERVICE->listFiles(current_path);
					}
					ImGui::EndDragDropTarget();
				}
			}

			// TO Do: File Drop Event
			/*void ResourcePanel::onEvent(std::shared_ptr<Assets::FileDropEvent> event) {

				if (NIKE_LVLEDITOR_SERVICE->getEditorState() && !checkPopUpShowing()) {
					int file_count = event->count;
					const char** file_paths = event->paths;

					//Initialize message
					std::string message = "Files Added: " + std::to_string(file_count) + " \n";

					for (int i = 0; i < file_count; ++i) {
						std::filesystem::path src_file_path{ file_paths[i] };

						//Check if path is valid
						if (PN_ASSETS_SERVICE->isPathValid(src_file_path.string(), false)) {

							//Get asset id
							auto asset_id = PN_ASSETS_SERVICE->getIDFromPath(src_file_path.string(), false);

							//Check if asset has already been registered
							if (PN_ASSETS_SERVICE->isAssetRegistered(asset_id)) {
								//Delete assets old registration
								std::filesystem::remove(PN_ASSETS_SERVICE->getAssetPath(asset_id));
							}

							//Copy file
							std::filesystem::copy(src_file_path, PN_PATH_SERVICE->resolvePath(current_path), std::filesystem::copy_options::overwrite_existing);

							//Log success
							NIKEE_CORE_INFO("File " + src_file_path.string() + " successfully copied into" + PN_PATH_SERVICE->resolvePath(current_path).string());
							message += std::string(file_paths[i]) + "\n";
						}
						else {
							NIKEE_CORE_ERROR("Error Unsupported File Type: {}", file_paths[i]);
							message = "Error Unsupported File Type: " + src_file_path.filename().extension().string();
						}
					}

					//Update directories & files
					directories = PN_PATH_SERVICE->listDirectories(current_path);
					files = PN_PATH_SERVICE->listFiles(current_path);

					//Show success popup
					success_msg->assign(message);
					b_file_dropped = true;
				}

				event->setEventProcessed(true);
			}*/

			unsigned int ResourcePanel::fileIcon(std::filesystem::path const& path) {
				//Get assets icons
				//if (PN_ASSETS_SERVICE->getAssetType(path) == Assets::Types::Texture && PN_ASSETS_SERVICE->isAssetCached(path)) {

				//	//Check if asset has been loaded
				//	std::string icon_ref = PN_ASSETS_SERVICE->getIDFromPath(path.string(), false);
				//	return PN_ASSETS_SERVICE->getAsset<Assets::Texture>(icon_ref)->gl_data;
				//}
				//else {
				//	std::string icon_ref = path.extension().string().substr(1) + "_icon.png";
				//	if (auto texture = PN_ASSETS_SERVICE->getAsset<Assets::Texture>(icon_ref)) {
				//		return texture->gl_data;
				//	}
				//	else {
				//		//Load default file icon
				//		return PN_ASSETS_SERVICE->getAsset<Assets::Texture>("def_icon.png")->gl_data;
				//	}
				//}

				return 0;
			}

			void ResourcePanel::renderAssetsBrowser(std::string const& virtual_path) {

				//Check if both files and directories are empty
				if (directories.empty() && files.empty()) {
					ImGui::Text("No results.");
					return;
				}

				//Calculate the number of icons per row based on window size & icon size
				int icons_per_row = static_cast<int>((ImGui::GetContentRegionAvail().x + (ImGui::GetStyle().ItemSpacing.x * 2)) / (icon_size.x + (ImGui::GetStyle().ItemSpacing.x * 2)));
				icons_per_row = std::clamp(icons_per_row, 1, (int)UINT16_MAX);

				//Track item index
				int itemIndex = 0;

				//Display all directories
				for (const auto& dir : directories) {

					//Skip dir not matching searching filter
					if (dir == PN_PATH_SERVICE->resolvePath("Engine_Assets:/") || dir.string().find(search_filter) == dir.string().npos) {
						continue;
					}

					//Item to exist in the same row
					if (itemIndex % icons_per_row != 0) {
						ImGui::SameLine();
					}

					ImGui::BeginGroup();

					//Folder icon
					/*ImTextureID icon = static_cast<ImTextureID>(PN_ASSETS_SERVICE->getAsset<Assets::Texture>("folder_icon.png")->gl_data);

					//Display directory icon
					ImVec2 uv0(0.0f, 1.0f);
					ImVec2 uv1(1.0f, 0.0f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

					if (ImGui::ImageButton(std::string("##" + dir.filename().string()).c_str(), icon, ImVec2(icon_size.x, icon_size.y), uv0, uv1)) {
						//Change current path to folder path clicked
						current_path = virtual_path + '/' + dir.filename().string();

						//Update directories & files
						directories = PN_PATH_SERVICE->listDirectories(current_path);
						files = PN_PATH_SERVICE->listFiles(current_path);

						//Break from files
						ImGui::PopStyleColor();
						ImGui::EndGroup();
						break;
					}
					ImGui::PopStyleColor();
					moveFileAcceptPayload(virtual_path + '/' + dir.filename().string());*/


					// ----------- Placeholder as Icon PNG not in yet ---------------------
					// Determine type for label (assuming 'dir' is a std::filesystem::path object)
					std::string label;
					auto type = PN_ASSETS_SERVICE->getAssetType(dir);

					switch (type) {
					case Assets::Types::Model:   label = "Model"; break;
					case Assets::Types::Music:   label = "Music"; break;
					case Assets::Types::Scene:   label = "Scene"; break;
					case Assets::Types::Prefab:  label = "Prefab"; break;
					case Assets::Types::Grid:    label = "Grid"; break;
					case Assets::Types::Script:  label = "Script"; break;
					case Assets::Types::Font:    label = "Font"; break;
					case Assets::Types::Video:   label = "Video"; break;
					case Assets::Types::Texture: label = "Texture"; break;
					default:
						if (std::filesystem::is_directory(dir))
							label = "Folder"; // Use "Folder" for directories
						else
							label = "File";
						break;
					}

					// Display directory icon - using a styled button with *only* the label
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 10.0f));

					// The button text is now ONLY the label, but we must still add a unique ID (##...)
					std::string button_id = label + "##" + dir.filename().string();

					// Use a regular ImGui::Button with the label as the visible text
					if (ImGui::Button(button_id.c_str(), ImVec2(icon_size.x, icon_size.y))) {
						// Change current path to folder path clicked
						current_path = virtual_path + '/' + dir.filename().string();

						// Update directories & files
						directories = PN_PATH_SERVICE->listDirectories(current_path);
						files = PN_PATH_SERVICE->listFiles(current_path);

						// Pop styles and break
						ImGui::PopStyleVar(2);
						ImGui::PopStyleColor();
						ImGui::EndGroup();
						break;
					}

					// Pop styles if the button wasn't clicked
					ImGui::PopStyleVar(2);
					ImGui::PopStyleColor();

					// Retain the move file functionality outside the button's 'if' block
					moveFileAcceptPayload(virtual_path + '/' + dir.filename().string());

					// ----------- Placeholder as Icon PNG not in yet ---------------------


					//Display directory name
					ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + icon_size.x);
					ImGui::TextWrapped(dir.filename().string().c_str());
					ImGui::PopTextWrapPos();

					ImGui::EndGroup();

					itemIndex++;
				}

				//Display all files
				for (const auto& file : files) {

					//Skip file not matching searching filter
					if (file.string().find(search_filter) == file.string().npos) {
						continue;
					}

					//Item to exist in the same row
					if (itemIndex % icons_per_row != 0) {
						ImGui::SameLine();
					}

					//Begin file group
					ImGui::BeginGroup();

					//Extension cases
					ImTextureID icon = static_cast<ImTextureID>(fileIcon(file));

					//Display file icon
					ImVec2 uv0(0.0f, 1.0f);
					ImVec2 uv1(1.0f, 0.0f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					if (ImGui::ImageButton(std::string("##" + file.filename().string()).c_str(), icon, ImVec2(icon_size.x, icon_size.y), uv0, uv1)) {
						selected_asset_id = file.filename().string();
					}
					ImGui::PopStyleColor();

					//Start drag-and-drop source
					if (PN_ASSETS_SERVICE->isAssetRegistered(file.filename().string()) && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
						auto filetype_string = PN_ASSETS_SERVICE->getAssetTypeString(file.filename().string());
						//Set drag payload with asset name
						ImGui::SetDragDropPayload(std::string(filetype_string + "_FILE").c_str(), file.filename().string().c_str(), file.filename().string().size() + 1);
						payload_typestring = filetype_string;

						//Render the icon or name at the cursor during dragging
						ImGui::Image(icon, { 64, 64 }, uv0, uv1);
						ImGui::TextWrapped(file.filename().string().c_str());
						ImGui::EndDragDropSource();
					}

					//Display file name
					ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + icon_size.x);
					ImGui::TextWrapped(file.filename().string().c_str());
					ImGui::PopTextWrapPos();

					ImGui::EndGroup();

					itemIndex++;
				}
			}

			int ResourcePanel::TextCallback(ImGuiInputTextCallbackData* data) {
				EditorState* editor_state = static_cast<EditorState*>(data->UserData);
				editor_state->cursor_pos = data->CursorPos;
				return 0;
			}

			void ResourcePanel::extractCurrentWord(std::string const& content, size_t cursor_pos, std::string& buffer) {
				size_t word_start = content.find_last_of(" \n\t", cursor_pos - 1);
				word_start = (word_start == std::string::npos) ? 0 : word_start + 1;
				size_t word_end = content.find_first_of(" \n\t", cursor_pos);
				word_end = (word_end == std::string::npos) ? content.size() : word_end;
				buffer = content.substr(word_start, word_end - word_start);
			}

			void ResourcePanel::showLuaIntellisense(std::string& content, size_t cursor_pos, std::string& buffer) {

				//Search for matching functions (TO DO: Script)
				/*if (!buffer.empty()) {
					for (const auto& func : PN_LUA_SERVICE->getGlobalLuaFunctions()) {
						if (func.find(buffer.c_str()) != std::string::npos) {
							if (ImGui::Selectable(func.c_str())) {
								auto pos = cursor_pos - buffer.size();
								content.erase(pos, buffer.size());
								content.insert(pos, func.c_str());
							}
						}
					}
				}*/
			}

			void ResourcePanel::renderFileEditor() {
				for (decltype(file_editing_map)::iterator it = file_editing_map.begin(); it != file_editing_map.end(); ++it) {
					ImGui::Begin(it->first.c_str());

					//Editing file
					static EditorState editor_state;
					ImGui::Text("Editing: %s", it->first.c_str());
					if (ImGui::InputTextMultiline("##editor", &it->second[0], it->second.capacity(),
						ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y * 0.95f),
						ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CallbackAlways, TextCallback, &editor_state)) {
						it->second.resize(strlen(it->second.c_str()));
					}

					//Special lua intellisense
					if (PN_ASSETS_SERVICE->getAssetType(it->first) == Assets::Types::Script) {
						//Lua intellisense
						static std::string current_word;
						current_word.reserve(64);
						extractCurrentWord(it->second, editor_state.cursor_pos, current_word);
						showLuaIntellisense(it->second, editor_state.cursor_pos, current_word);
					}

					ImGui::Spacing();

					//Save file
					if (ImGui::Button("Save##Save file") || (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S))) {
						std::ofstream file(PN_ASSETS_SERVICE->getAssetPath(it->first));
						if (file.is_open()) {
							file << it->second;
							file.close();
							PN_CORE_INFO("File saved.");
						}
						else {
							PN_CORE_WARN("Failed to save file");
							file.close();
						}
					}

					ImGui::SameLine();

					//Close file
					if (ImGui::Button("Close##CloseFile")) {
						it = file_editing_map.erase(it);
						ImGui::End();
						break;
					}

					ImGui::End();
				}
			}

			std::function<void()> ResourcePanel::deleteAssetPopup(std::string const& popup_id) {
				return [this, popup_id]() {

					//Warning message
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "This action cannot be undone!");

					//Select a component to add
					ImGui::Text("Are you sure you want to delete this asset?");

					//Add spacing
					ImGui::Spacing();

					//Display each component as a button
					if (ImGui::Button("Confirm")) {

						//Get selected asset path
						auto path = PN_ASSETS_SERVICE->getAssetPath(selected_asset_id);

						//Remove path and clear selected asset text buffer
						std::filesystem::remove(path);
						selected_asset_id.clear();
						files = PN_PATH_SERVICE->listFiles(current_path);

						//Close popup
						closePopUp(popup_id);
					}

					//Same line
					ImGui::SameLine();

					//Cancel deleting asset
					if (ImGui::Button("Cancel")) {

						//Close popup
						closePopUp(popup_id);
					}
				};
			}

			std::function<void()> ResourcePanel::deleteDirectoryPopup(std::string const& popup_id) {
				return [this, popup_id]() {

					//Warning message
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "This action cannot be undone!");

					//Select a component to add
					ImGui::Text("Are you sure you want to delete everything in selected directory?");

					//Add spacing
					ImGui::Spacing();

					//Display each component as a button
					if (ImGui::Button("Confirm")) {

						//Check for directory mode
						switch (directory_mode) {
						case 0: {

							//Remove all files in current directory
							for (auto const& file : files) {
								std::filesystem::remove(file);
							}
							break;
						}
						case 1: {

							//Remove all files & folders in current directory
							for (auto const& file : files) {
								std::filesystem::remove(file);
							}
							for (auto const& dir : directories) {
								std::filesystem::remove_all(dir);
							}
							break;
						}
						case 2: {

							//Remove all files & folders in root directory
							for (auto const& file : PN_PATH_SERVICE->listFiles(root_path)) {
								std::filesystem::remove(file);
							}
							for (auto const& dir : PN_PATH_SERVICE->listDirectories(root_path)) {
								std::filesystem::remove_all(dir);
							}
							current_path = root_path;
							break;
						}
						default: {
							break;
						}
						}


						//Update directories & files
						directories = PN_PATH_SERVICE->listDirectories(current_path);
						files = PN_PATH_SERVICE->listFiles(current_path);

						//Close popup
						closePopUp(popup_id);
					}

					//Same line
					ImGui::SameLine();

					//Cancel deleting asset
					if (ImGui::Button("Cancel")) {

						//Close popup
						closePopUp(popup_id);
					}
				};
			}

			std::function<void()> ResourcePanel::newFolderPopup(std::string const& popup_id) {
				return [this, popup_id]() {

					//Select a component to add
					ImGui::Text("New folder name: ");

					//New folder name
					static std::string folder_name = "";
					folder_name.resize(32);
					ImGui::InputText("##NewFolderName", folder_name.data(), folder_name.capacity() + 1);

					//Add spacing
					ImGui::Spacing();

					//Display each component as a button
					if (ImGui::Button("Create")) {

						//Create a new directory
						std::filesystem::create_directory(PN_PATH_SERVICE->resolvePath(current_path) / folder_name);

						//Update directories & files
						directories = PN_PATH_SERVICE->listDirectories(current_path);

						//Reset folder name buffer
						folder_name.assign("");

						//Close popup
						closePopUp(popup_id);
					}

					//Same line
					ImGui::SameLine();

					//Cancel deleting asset
					if (ImGui::Button("Cancel")) {

						//Reset folder name buffer
						folder_name.assign("");

						//Close popup
						closePopUp(popup_id);
					}
				};
			}

			void ResourcePanel::pushFileEvent(std::function<void()> callback) {
				std::lock_guard<std::mutex> lock(file_event_mutex);
				file_event_queue.push(std::move(callback));
			}

			void ResourcePanel::init() {

				//Setup events listening
				/*std::shared_ptr<LevelEditor::ResourcePanel> resourcepanel_wrapped(this, [](LevelEditor::ResourcePanel*) {});
				NIKE_EVENTS_SERVICE->addEventListeners<Assets::FileDropEvent>(resourcepanel_wrapped);

				entities_panel = std::dynamic_pointer_cast<EntitiesPanel>(NIKE_LVLEDITOR_SERVICE->getPanel(EntitiesPanel::getStaticName()));*/

				//Register popups
				error_msg = std::make_shared<std::string>("Error");
				success_msg = std::make_shared<std::string>("Success");

				// Pop UP
				registerPopUp("Error", defPopUp("Error", error_msg));
				registerPopUp("Success", defPopUp("Success", success_msg));
				registerPopUp("Delete Asset", deleteAssetPopup("Delete Asset"));
				registerPopUp("Clear Directory", deleteDirectoryPopup("Clear Directory"));
				registerPopUp("New Folder", newFolderPopup("New Folder"));

				//Initialize root
				root_path = "Game_Assets:/";
				current_path = root_path;

				//Search up till 32 characters
				search_filter.resize(32);
				search_filter = "";

				//Default icon size
				icon_size = { 128.0f, 128.0f };

				//Register all engine icons
				//PN_ASSETS_SERVICE->scanAssetDirectory("Engine_Assets:/Icons");

				//Init all directories & files
				directories = PN_PATH_SERVICE->listDirectories(current_path);
				files = PN_PATH_SERVICE->listFiles(current_path);

				// Create a weak_ptr for safe capturing in filewatch callbacks
				//std::weak_ptr<LevelEditor::ResourcePanel> weak_this = resourcepanel_wrapped;

				//Setup directory watching 
				/*PN_PATH_SERVICE->watchDirectoryTree("Game_Assets:/", [weak_this](std::filesystem::path const& file, filewatch::Event event) {
					if (auto shared_this = weak_this.lock()) { // Check if the object is still alive

						//Engine engine assets path
						static auto engine_assets = PN_PATH_SERVICE->resolvePath("Engine_Assets:/");

						//Skip directories & invalid paths
						if (std::filesystem::is_directory(file) ||
							!PN_ASSETS_SERVICE->isPathValid(file.string(), false) ||
							file.string().find(engine_assets.string()) != std::string::npos) {
							return;
						}

						//Watch for events
						switch (event) {
						case filewatch::Event::added: {

							cout << "ADD EVENT FOR PATH: " << file.string() << " " << file.extension().string() << endl;

							//Push to file event queue
							shared_this->pushFileEvent([&, file]() {

								//Register asset if needed
								auto asset_id = PN_ASSETS_SERVICE->getIDFromPath(file.string(), false);
								if (!PN_ASSETS_SERVICE->isAssetRegistered(asset_id)) {
									PN_ASSETS_SERVICE->registerAsset(file.string(), false);
								}
								});

							break;
						}
						case filewatch::Event::removed: {

							cout << "REMOVE EVENT FOR PATH: " << file.string() << " " << file.extension().string() << endl;

							//Push to file event queue
							shared_this->pushFileEvent([&, file]() {

								//Unregister asset if needed
								PN_ASSETS_SERVICE->unregisterAsset(PN_ASSETS_SERVICE->getIDFromPath(file.string(), false));
								});

							break;
						}
						case filewatch::Event::modified: {

							cout << "MODIFIED EVENT FOR PATH: " << file.string() << " " << file.extension().string() << endl;

							//Push to file event queue
							shared_this->pushFileEvent([&, file]() {

								//Only recache assets that are already cached
								auto asset_id = PN_ASSETS_SERVICE->getIDFromPath(file.string(), false);
								if (PN_ASSETS_SERVICE->isAssetCached(asset_id)) {

									//Recache asset
									PN_ASSETS_SERVICE->recacheAsset(asset_id);
								}
								});

							break;
						}
						default: {
							break;
						}
						}
					}
				});*/
			}

			void ResourcePanel::render() {

				//Update resource panel with file change events
				while (!file_event_queue.empty()) {

					try {
						//Call callback function if valid
						if (file_event_queue.front()) {
							//Execute file event callback
							file_event_queue.front()();
						}

						//Pop from queue
						file_event_queue.pop();
					}
					catch (std::exception const&) {
						PN_CORE_WARN("Invalid Callback From FileWatcher Handled. Loop Continues.");
					}
				}

				ImGui::BeginMenuBar();

				//Parent path navigation
				{
					//Back button
					if (!current_path.empty() && ImGui::Button("< Back")) {

						//Stop searching for parent at root directory
						if (current_path != root_path) {
							current_path = PN_PATH_SERVICE->getVirtualParentPath(current_path);

							//Update directories & files
							directories = PN_PATH_SERVICE->listDirectories(current_path);
							files = PN_PATH_SERVICE->listFiles(current_path);
						}
					}
					moveFileAcceptPayload(PN_PATH_SERVICE->getVirtualParentPath(current_path));
				}

				ImGui::Spacing();

				//New folder
				{
					//Create new folder popup
					if (ImGui::Button("New Folder")) {
						openPopUp("New Folder");
					}
				}

				ImGui::Spacing();

				//Directory level actions
				{
					//Array of load directories
					const char* load_directory[] = { "Current", "Current *", "Root *" };

					//Render the dropdown
					ImGui::PushItemWidth(100.0f);
					ImGui::Combo("##Directory", &directory_mode, load_directory, IM_ARRAYSIZE(load_directory));
					ImGui::PopItemWidth();

					//Load all from directory
					if (ImGui::Button("Load All")) {

						//Check for directory mode
						switch (directory_mode) {
							case 0: {
								PN_ASSETS_SERVICE->cacheAssetDirectory(current_path);
								success_msg->assign("All assets in: \"" + current_path + "\" loaded.");
								openPopUp("Success");
								break;
							}
							case 1: {
								PN_ASSETS_SERVICE->cacheAssetDirectory(current_path, true);
								success_msg->assign("All assets in: \"" + current_path + "*\" loaded.");
								openPopUp("Success");
								break;
							}
							case 2: {
								PN_ASSETS_SERVICE->cacheAssetDirectory(root_path, true);
								success_msg->assign("All assets in: \"" + root_path + "*\" loaded.");
								openPopUp("Success");
								break;
							}
							default: {
								break;
							}
						}
					}

					//Unload all from directory
					if (ImGui::Button("Unload All")) {

						//Check for directory mode
						switch (directory_mode) {
						case 0: {
							PN_ASSETS_SERVICE->uncacheAssetDirectory(current_path);
							success_msg->assign("All assets in: \"" + current_path + "*\" unloaded.");
							openPopUp("Success");
							break;
						}
						case 1: {
							PN_ASSETS_SERVICE->uncacheAssetDirectory(current_path, true);
							success_msg->assign("All assets in: \"" + current_path + "*\" unloaded.");
							openPopUp("Success");
							break;
						}
						case 2: {
							PN_ASSETS_SERVICE->uncacheAssetDirectory(root_path, true);
							success_msg->assign("All assets in: \"" + root_path + "*\" unloaded.");
							openPopUp("Success");
							break;
						}
						default: {
							break;
						}
						}
					}

					//Delete all from directory
					if (ImGui::Button("Delete All")) {
						openPopUp("Clear Directory");
					}
				}

				ImGui::Spacing();

				//Customize icon size
				{
					ImGui::Text("Icon Size: ");
					ImGui::PushItemWidth(50.0f);
					ImGui::DragFloat("##IconSizing", &icon_size.x, 1.0f, 32.0f, 256.0f, "%.f", ImGuiSliderFlags_AlwaysClamp);
					ImGui::PopItemWidth();
					icon_size.y = icon_size.x;
				}

				ImGui::Spacing();

				//Search filter
				{
					//Input filter
					ImGui::Text("Filter: ");
					ImGui::SameLine();
					ImGui::PushItemWidth(100.0f);
					if (ImGui::InputTextWithHint("##SearchFilter", "Search...", search_filter.data(), search_filter.capacity() + 1)) {
						search_filter.resize(strlen(search_filter.c_str()));
					}
					ImGui::PopItemWidth();
				}

				ImGui::Spacing();

				//Refresh directory
				{
					if (ImGui::Button("Refresh")) {
						//Update directories & files
						directories = PN_PATH_SERVICE->listDirectories(current_path);
						files = PN_PATH_SERVICE->listFiles(current_path);
					}
				}

				//Render popups
				renderPopUps();

				ImGui::EndMenuBar();

				//Render all assets & folders
				renderAssetsBrowser(current_path);

				//Set window dock id
				dock_id = ImGui::GetWindowDockID();

				//Render selected asset options
				if (!selected_asset_id.empty() && PN_ASSETS_SERVICE->isAssetRegistered(selected_asset_id)) {

					// Center the panel
					ImGui::Begin("Selected Asset", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings);

					//Get selected asset path
					auto path = PN_ASSETS_SERVICE->getAssetPath(selected_asset_id);

					//Asset metadata
					ImGui::Text("Asset: %s", selected_asset_id.c_str());
					ImGui::Text("Type: %s", PN_ASSETS_SERVICE->getAssetTypeString(selected_asset_id).c_str());

					//Get selected asset texture display
					ImTextureID display = static_cast<ImTextureID>(fileIcon(path));

					//Display image
					ImVec2 uv0(0.0f, 1.0f);
					ImVec2 uv1(1.0f, 0.0f);
					ImGui::Image(display, { 256, 256 }, uv0, uv1);

					//Loadable type actions
					if (PN_ASSETS_SERVICE->isAssetLoadable(selected_asset_id)) {

						//Show audio length if asset is loaded & an audio file
						//if (PN_ASSETS_SERVICE->isAssetCached(selected_asset_id) && (PN_ASSETS_SERVICE->getAssetType(selected_asset_id) == Assets::Types::Sound ||
						//	PN_ASSETS_SERVICE->getAssetType(selected_asset_id) == Assets::Types::Music)) {

						//	//Show audio length
						//	auto length = PN_ASSETS_SERVICE->getAsset<Audio::IAudio>(selected_asset_id)->getLength(NIKE_AUDIO_TIMEUNIT_MS);
						//	ImGui::Text("Length:");
						//	ImGui::Text("%d ms", length);
						//	ImGui::Text("%.2f s", length / 1000.0f);
						//	ImGui::Text("%.2f mins", (length / 1000.0f) / 60.0f);
						//}

						//Asset loading or unloading
						if (PN_ASSETS_SERVICE->isAssetCached(selected_asset_id)) {
							//Unload action
							if (ImGui::Button("Unload")) {

								//Unload asset
								PN_ASSETS_SERVICE->uncacheAsset(selected_asset_id);
								success_msg->assign("Asset: \"" + selected_asset_id + "\" unloaded.");
								openPopUp("Success");
							}

							//Audio asset preview
							if (PN_ASSETS_SERVICE->getAssetType(selected_asset_id) == Assets::Types::Sound ||
								PN_ASSETS_SERVICE->getAssetType(selected_asset_id) == Assets::Types::Music) {

								//Same line
								ImGui::SameLine();

								//Play button
								if (ImGui::Button("Play")) {
									//Check if channel group has been created
									//if (!PN_AUDIO_SERVICE->checkChannelGroupExist("Audio Preview")) {
									//	PN_AUDIO_SERVICE->createChannelGroup("Audio Preview");
									//}

									////Get audio group
									//auto group = PN_AUDIO_SERVICE->getChannelGroup("Audio Preview");

									////Toggle audio state
									//if (group->getPaused()) {
									//	group->setPaused(false);
									//}
									//else {
									//	//Play music
									//	bool is_music = PN_ASSETS_SERVICE->getAssetType(selected_asset_id) == Assets::Types::Music ? true : false;
									//	PN_AUDIO_SERVICE->playAudio(selected_asset_id, "", "Audio Preview", 0.5f, 0.5f, false, is_music);
									//}
								}

								//Manage preview audio group
								//if (PN_AUDIO_SERVICE->checkChannelGroupExist("Audio Preview")) {
								//	auto group = PN_AUDIO_SERVICE->getChannelGroup("Audio Preview");

								//	if (group->isPlaying()) {
								//		//Same line
								//		ImGui::SameLine();

								//		//Pause audio preview
								//		if (ImGui::Button("Pause")) {
								//			group->setPaused(true);
								//		}

								//		//Same line
								//		ImGui::SameLine();

								//		//Pause audio preview
								//		if (ImGui::Button("Stop")) {
								//			group->stop();
								//		}
								//	}
								//	else if (!group->isPlaying() && !group->getPaused()) {
								//		PN_AUDIO_SERVICE->unloadChannelGroup("Audio Preview");
								//	}
								//}
							}


						}
						else {
							//Load action
							if (ImGui::Button("Load")) {

								//Load asset
								PN_ASSETS_SERVICE->cacheAsset(selected_asset_id);
								success_msg->assign("Asset: \"" + selected_asset_id + "\" loaded.");
								openPopUp("Success");
							}
						}

						//Same line
						ImGui::SameLine();
					}

					//Editable type actions
					if (PN_ASSETS_SERVICE->isAssetEditable(selected_asset_id)) {
						if (ImGui::Button("Edit##EditableAsset")) {

							//Read file into string
							std::ifstream file(PN_ASSETS_SERVICE->getAssetPath(selected_asset_id));
							if (file.is_open()) {

								//Check if file is already open
								if (file_editing_map.find(selected_asset_id) == file_editing_map.end()) {
									file_editing_map[selected_asset_id].reserve(1024 * 1024); // 1mb storage for file editing
									// Read file content
									file_editing_map[selected_asset_id].assign((std::istreambuf_iterator<char>(file)),
										std::istreambuf_iterator<char>());
									file.close();
								}
							}
							else {
								PN_CORE_WARN("Failed to open file");
								file.close();
							}
						}

						//Same line
						ImGui::SameLine();
					}

					//Delete asset
					if (ImGui::Button("Delete##DeleteAsset")) {
						openPopUp("Delete Asset");
					}

					//Same line
					ImGui::SameLine();

					//Unload action
					if (ImGui::Button("Close##CloseAsset")) {

						//Reset selected asset id
						selected_asset_id.clear();
					}

					//Render popups
					renderPopUps();
				}

				//Render file editor
				renderFileEditor();

				//File dropped popup
				/*if (b_file_dropped && !checkPopUpShowing()) {
					openPopUp("Success");
					b_file_dropped = false;
				}*/

				//Render popups
				renderPopUps();

			}


        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif
