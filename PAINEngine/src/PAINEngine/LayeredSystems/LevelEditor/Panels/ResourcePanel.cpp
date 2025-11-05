#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG

#include "pch.h"
#include "ResourcePanel.h"

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include "CoreSystems/Events/GLFW/AssetEvents.h"

namespace PAIN {
    namespace Editor {
        namespace Panel {

            ResourcePanel::ResourcePanel() {

                name = "Resource Panel";

                //Set panel flag
                //flags = ImGuiWindowFlags_MenuBar;

                // Default icon size
                icon_size = { 128.0f, 128.0f };

                // Search filter
                search_filter.resize(32);
                search_filter = "";
            }

            void ResourcePanel::nextWindowSettings() {
                // Default behavior (no special fullscreen/docking hacks)
            }

            void ResourcePanel::onUpdate(AppTiming timing) {

				//Increment timer
				auto_refresh_timer += timing.dt;

				//Render asset browser
				render();
            }

			void ResourcePanel::moveFileAcceptPayload(std::string const& virtual_path) {
#ifdef PN_PLATFORM_WINDOWS
				//Drop target
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(payload_typestring + "_FILE").c_str())) {
						//Get asset ID
						File* file(static_cast<File*>(payload->Data));

						//Rename asset
						asset_service->moveFile(file->path, path_service->resolvePath(virtual_path + "/" + file->file_name));
					}
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(payload_typestring + "DIR").c_str())) {
						//Get asset ID
						Dir* dir(static_cast<Dir*>(payload->Data));

						//Rename asset
						asset_service->moveFile(dir->path, path_service->resolvePath(virtual_path + "/" + dir->file_name));
					}
					ImGui::EndDragDropTarget();
				}
#endif
			}

			void ResourcePanel::onEvent(Event::Event& event) {
#ifdef PN_PLATFORM_WINDOWS
				if (event.getType() == PAIN::Event::Type::FileDrop) {

					//Create event dispatcher
					Event::Dispatcher dispatcher(event);

					//Dispatch window resized event
					dispatcher.Dispatch<Event::FileDropped>([&](Event::FileDropped& e) -> bool {

						//File dropped msg
						std::vector<std::string> msg;
						msg.push_back("Files Dropped:");

						//iterate through files
						for (auto path : e.getPaths()) {

							//Get file path
							std::filesystem::path file_path = path;

							//Target directory ( main game asset folder )
							auto target = path_service->resolvePath(Path::main_assets_alias, file_path.filename().string());

							//Throw asset into the game asset folder
							std::filesystem::copy(file_path, target, std::filesystem::copy_options::overwrite_existing);

							//Sort asset into registry
							asset_service->registerAsset(target);

							msg.push_back(file_path.string());
						}

						//Craft message
						openPopUp("Info", std::make_shared<std::vector<std::string>>(msg));

						//Return false: continue dispatching, true = stop dispatching 
						return true;
						});
				}
#endif
			}

			void ResourcePanel::populateDirs(std::string const& virtual_path) {
				//Retrieve file
				auto fetch_dirs = path_service->listDirectories(virtual_path);

				//Clear file directory
				directories.clear();

				//Iterate through fetched files
				for (auto const& dir : fetch_dirs) {
					Dir temp;

					//Instantiate file system
					temp.path = dir;

					//Get root folder path
					
					auto relative = std::filesystem::relative(temp.path, root);

					//Get display icon
					std::filesystem::path folder_path = "engine\\textures\\folder_icon.png";

					//Folder icon
					if (services->get<Assets::Manager>()->checkAssetRegistered(folder_path)) {
						temp.icon = static_cast<ImTextureID>(services->get<Assets::Manager>()->getAsset<Assets::Texture>(folder_path)->gl_texture);
					}
					else {
						temp.icon = 0;
					}

					//Instantiate name
					temp.file_name = relative.filename().string();

					//Add this to file vector
					directories.push_back(temp);
				}
			}

			void ResourcePanel::populateFiles(std::string const& virtual_path) {

				//Retrieve file
				auto fetch_files = path_service->listFiles(virtual_path);

				//Clear file directory
				files.clear();

				//Iterate through fetched files
				for (auto const& file : fetch_files) {
					File temp;

					//Instantiate file system
					temp.path = file;

					//Get root folder path
					
					auto relative = std::filesystem::relative(temp.path, root);

					//Find asset GUID
					temp.id = asset_service->findGUID(relative);

					//Find asset type
					if (temp.id.IsValid()) {
						temp.type = asset_service->getAssetData(temp.id)->type;
					}

					//Get display icon
					temp.icon = fileIcon(relative);

					//Instantiate name
					temp.file_name = relative.filename().string();

					//Add this to file vector
					files.push_back(temp);
				}
			}

			void ResourcePanel::populateDirectoryCache(const std::string& virtual_dir) {
				if (directoryCache.count(virtual_dir) > 0) return; // Already cached

				std::vector<std::string> children;
				std::filesystem::path dir = path_service->resolvePath(virtual_dir);
				for (const auto& entry : path_service->listDirectories(virtual_dir)) {
					auto relative = std::filesystem::relative(entry, dir);
					children.push_back(virtual_dir + "/" + relative.string());
				}
				directoryCache[virtual_dir] = std::move(children);
			}

			void ResourcePanel::DrawDirectoryTree(std::string const& virtual_dir) {

				//Populat directory cache
				populateDirectoryCache(virtual_dir);
				bool has_children = !directoryCache[virtual_dir].empty();

				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_DefaultOpen;
				if (!has_children)
					node_flags |= ImGuiTreeNodeFlags_Leaf;
				bool is_selected = (virtual_dir == current_path);
				if (is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

				std::filesystem::path dir = path_service->resolvePath(virtual_dir);
				std::string name = dir.filename().string() != "" ? dir.filename().string() : "assets";
				bool open = ImGui::TreeNodeEx(name.c_str(), node_flags);
				moveFileAcceptPayload(virtual_dir);
				if (ImGui::IsItemActivated() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					current_path = virtual_dir;
					populateDirs(current_path);
					populateFiles(current_path);
				}
				if (open) {
					for (const std::string& subdir : directoryCache[virtual_dir]) {
						DrawDirectoryTree(subdir);
					}
					ImGui::TreePop();
				}
			}

			bool ResourcePanel::renderPopUpContext(File const& file) {

				//Boolean break
				bool b_break = false;

				//Push ID
				ImGui::PushID(file.path.string().c_str());

				//Right-click context
				if (ImGui::BeginPopupContextItem("AssetContextMenu##file")) {

					if (ImGui::MenuItem("Open##file")) {
						open_files.push_back(file);
					}
					if (ImGui::MenuItem("Rename##file")) {
						openPopUp("Rename File", std::make_shared<File>(file));
					}
					if (ImGui::MenuItem("Delete##file")) {
						openPopUp("Delete File", std::make_shared<File>(file));
					}
					if (ImGui::MenuItem("New Folder##file")) {
						openPopUp("New Folder");
					}
					if (ImGui::MenuItem("Duplicate##file")) {
						asset_service->duplicateFile(file.path);
					}
					ImGui::EndPopup();
				}

				ImGui::PopID();
				return b_break;
			}

			bool ResourcePanel::renderPopUpContext(Dir const& dir) {

				//Boolean break
				bool b_break = false;

				//Push ID
				ImGui::PushID(dir.path.string().c_str());

				//Right-click context
				if (ImGui::BeginPopupContextItem("AssetContextMenu##dir")) {
					if (ImGui::MenuItem("Open##dir")) {

						//Set current path
						auto relative = std::filesystem::relative(dir.path, root);
						current_path = root_path + relative.string();

						//Update directories & files
						populateDirs(current_path);
						populateFiles(current_path);

						b_break = true;
					}
					if (ImGui::MenuItem("Rename##dir")) {
						openPopUp("Rename Folder", std::make_shared<Dir>(dir));
					}
					if (ImGui::MenuItem("Delete##dir")) {
						openPopUp("Delete Folder", std::make_shared<Dir>(dir));
					}
					if (ImGui::MenuItem("New Folder##dir")) {
						openPopUp("New Folder");
					}
					ImGui::EndPopup();
				}

				ImGui::PopID();
				return b_break;
			}

			unsigned int ResourcePanel::fileIcon(std::filesystem::path const& relative_path) {

				//Icon path
				std::filesystem::path icon_path;

				//Def icon path
				std::string def_icon = "def_icon";
				std::filesystem::path def_icon_path = std::filesystem::path("engine\\textures") / (def_icon + ".png");

				//Discover iconref
				if (Assets::getAssetType(relative_path) == Assets::Type::Texture) {

					auto parent_path = relative_path.parent_path();

					//Get file path
					icon_path = parent_path / (relative_path.filename());
				}
				else {
					//Def icon ref
					std::string icon_ref;

					//Identify extension
					auto ext = relative_path.extension().string();

					if (!ext.empty() && ext.size() > 1) {
						icon_ref = ext.substr(1) + "_icon";
					}

					//Find texture path
					icon_path = std::filesystem::path("engine\\textures") / (icon_ref + ".png");
				}

				//Check if icon path valid
				if (asset_service->checkAssetRegistered(icon_path)) {
					return static_cast<ImTextureID>(asset_service->getAsset<Assets::Texture>(icon_path)->gl_texture);
				}
				else {
					if (asset_service->checkAssetRegistered(def_icon_path)) {
						return static_cast<ImTextureID>(asset_service->getAsset<Assets::Texture>(def_icon_path)->gl_texture);
					}
					else {
						return ImTextureID(0);
					}
				}
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

				//Local shown count
				int shown_count = 0;

				//Display all directories
				for (const auto& dir : directories) {

					//Skip dir not matching searching filter
					if (dir.file_name.find(search_filter) == dir.file_name.npos) {
						continue;
					}

					//Item to exist in the same row
					if (itemIndex % icons_per_row != 0) {
						ImGui::SameLine();
					}

					ImGui::BeginGroup();
					++shown_count;

					//Folder icon
					ImTextureID icon = dir.icon;

					//Display directory icon
					ImVec2 uv0(0.0f, 0.0f);
					ImVec2 uv1(1.0f, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::ImageButton(std::string("##" + dir.file_name).c_str(), icon, ImVec2(icon_size.x, icon_size.y), uv0, uv1);
					if (ImGui::IsItemActivated() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						//Change current path to folder path clicked
						current_path = virtual_path + '/' + dir.file_name;

						//Update directories & files
						populateDirs(current_path);
						populateFiles(current_path);

						//Break from files
						ImGui::PopStyleColor();
						ImGui::EndGroup();
						break;
					}
					ImGui::PopStyleColor();
					moveFileAcceptPayload(virtual_path + '/' + dir.file_name);

					//Render context
					if (renderPopUpContext(dir)) {
						ImGui::EndGroup();
						break;
					}

					//Start drag-and-drop source
#ifdef PN_PLATFORM_WINDOWS
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {

						//Static copy of payload
						static Dir dir_copy;
						dir_copy = dir;

						//Set drag payload with asset name
						ImGui::SetDragDropPayload(std::string("DIR").c_str(), &dir_copy, sizeof(dir_copy) + 1);

						//Render the icon or name at the cursor during dragging
						ImGui::Image(icon, { 64, 64 }, uv0, uv1);
						ImGui::TextWrapped("%s", dir_copy.file_name.c_str());
						ImGui::EndDragDropSource();
					}
#endif

					//Display directory name
					ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + icon_size.x);
					ImGui::TextWrapped("%s", dir.file_name.c_str());
					ImGui::PopTextWrapPos();

					ImGui::EndGroup();

					itemIndex++;
				}

				//Display all files
				for (const auto& file : files) {

					//Skip file not matching searching filter
					if (file.file_name.find(search_filter) == file.file_name.npos) {
						continue;
					}

					//Item to exist in the same row
					if (itemIndex % icons_per_row != 0) {
						ImGui::SameLine();
					}

					//Check path is desc
					if (file.path.extension() == Assets::descriptor_ext) {
						if(!b_show_desc_files)continue;

						//Begin file group
						ImGui::BeginGroup();
						++shown_count;

						//Extension cases
						ImTextureID icon = file.icon;

						//Display file icon
						ImVec2 uv0(0.0f, 0.0f);
						ImVec2 uv1(1.0f, 1.0f);
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
						ImGui::ImageButton(std::string("##" + file.file_name).c_str(), icon, ImVec2(icon_size.x, icon_size.y), uv0, uv1);
						if (ImGui::IsItemActivated() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
							open_files.push_back(file);
						}
						ImGui::PopStyleColor();

						//Display file name
						ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + icon_size.x);
						ImGui::TextWrapped("%s", file.file_name.c_str());
						ImGui::PopTextWrapPos();

						ImGui::EndGroup();

						itemIndex++;
					}

					//Check that asset is registered
					if (!asset_service->checkAssetRegistered(file.id)) {
						continue;
					}

					//Begin file group
					ImGui::BeginGroup();
					++shown_count;

					//Extension cases
					ImTextureID icon = file.icon;

					//Display file icon
					ImVec2 uv0(0.0f, 0.0f);
					ImVec2 uv1(1.0f, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::ImageButton(std::string("##" + file.file_name).c_str(), icon, ImVec2(icon_size.x, icon_size.y), uv0, uv1);
					if (ImGui::IsItemActivated() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						open_files.push_back(file);
					}
					ImGui::PopStyleColor();

					//Render context
					if (renderPopUpContext(file)) {
						ImGui::EndGroup();
						break;
					}

					//Start drag-and-drop source ( Disable drag for desc files )
#ifdef PN_PLATFORM_WINDOWS
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {

						//Get file type string
						auto filetype_string = Assets::assetTypeToString(asset_service->getAssetData(file.id)->type);

						//Static copy of payload
						static File file_copy;
						file_copy = file;

						//Set drag payload with asset name
						ImGui::SetDragDropPayload(std::string(filetype_string + "_FILE").c_str(), &file_copy, sizeof(file_copy) + 1);
						payload_typestring = filetype_string;

						//Render the icon or name at the cursor during dragging
						ImGui::Image(icon, { 64, 64 }, uv0, uv1);
						ImGui::TextWrapped("%s", file.file_name.c_str());
						ImGui::EndDragDropSource();
					}
#endif

					//Display file name
					ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + icon_size.x);
					ImGui::TextWrapped("%s", file.file_name.c_str());
					ImGui::PopTextWrapPos();

					ImGui::EndGroup();

					itemIndex++;
				}

				if (shown_count == 0) {
					ImGui::Text("No results.");
				}
			}

			void ResourcePanel::renderOpenFiles() {

				//Local files to close
				std::vector<File> files_to_close;

				//Iterate through all open files
				for (auto const& file : open_files) {

					//Begin window
					ImGui::Begin(file.file_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

					//Display icon
					{
						ImVec2 icon_size(256, 256);
						ImGui::Image(file.icon, icon_size);
					}

					ImGui::Spacing();

					//File Name
					ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", file.file_name.c_str());
					ImGui::Separator();

					//File Type
					ImGui::Text("Type: %s", Assets::assetTypeToString(file.type).c_str());

					//Path
					ImGui::Text("Path: %s", file.path.string().c_str());

					//File size
					try {
						auto size = std::filesystem::file_size(file.path);
						ImGui::Text("Size: %llu bytes", static_cast<unsigned long long>(size));
					}
					catch (std::filesystem::filesystem_error& e) {
						ImGui::Text("Size: N/A");
					}

					//Last Modified
					try {
						auto ftime = std::filesystem::last_write_time(file.path);

						// Convert to system_clock::time_point
#if defined(_WIN32)
	// On MSVC, file_time_type may not be system_clock; do the conversion
						auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
							ftime - std::filesystem::file_time_type::clock::now()
							+ std::chrono::system_clock::now());
#else
						auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ftime);
#endif
						std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);

						char buf[64];
#if defined(_MSC_VER)
						ctime_s(buf, sizeof(buf), &cftime);
#else
						std::strftime(buf, sizeof(buf), "%c", std::localtime(&cftime));
#endif
						ImGui::Text("Last Modified: %s", buf);
					}
					catch (std::filesystem::filesystem_error& e) {
						ImGui::Text("Last Modified: N/A");
					}

					ImGui::Separator();

					//Operations
					if (ImGui::Button(std::string("Rename##" + file.file_name).c_str())) {
						openPopUp("Rename File", std::make_shared<File>(file));
					}
					ImGui::SameLine();
					if (ImGui::Button(std::string("Delete##" + file.file_name).c_str())) {
						openPopUp("Delete File", std::make_shared<File>(file));
					}
					ImGui::SameLine();
					if (ImGui::Button(std::string("Close##" + file.file_name).c_str())) {
						files_to_close.push_back(file);
					}

					ImGui::End();
				}

				//Close file
				for (auto close_it = files_to_close.begin(); close_it != files_to_close.end(); ++close_it) {
					for (auto it = open_files.begin(); it != open_files.end();) {
						if (it->id == close_it->id) {
							it = open_files.erase(it);
							break;
						}
						else {
							++it;
						}
					}
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

					////Special lua intellisense
					//if (PN_ASSETS_SERVICE->getAssetType(it->first) == Assets::Types::Script) {
					//	//Lua intellisense
					//	static std::string current_word;
					//	current_word.reserve(64);
					//	extractCurrentWord(it->second, editor_state.cursor_pos, current_word);
					//	showLuaIntellisense(it->second, editor_state.cursor_pos, current_word);
					//}

					ImGui::Spacing();

					//Save file
					if (ImGui::Button("Save##Save file") || (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_S))) {
						//std::ofstream file(PN_ASSETS_SERVICE->getAssetPath(it->first));
						//if (file.is_open()) {
						//	file << it->second;
						//	file.close();
						//	PN_CORE_INFO("File saved.");
						//}
						//else {
						//	PN_CORE_WARN("Failed to save file");
						//	file.close();
						//}
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

			std::function<void(std::any const&)> ResourcePanel::deleteFilePopup(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {

					//Warning message
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "This action cannot be undone!");

					//Select a component to add
					ImGui::Text("Are you sure you want to delete file?");

					//Add spacing
					ImGui::Spacing();

					//Display each component as a button
					if (ImGui::Button("Confirm")) {

						//Check for cast validity
						if (data.has_value() && data.type() == typeid(std::shared_ptr<File>)) {

							//Cast to file and remove file
							auto file = std::any_cast<std::shared_ptr<File>>(data);

							//Find open files and remove
							for (auto it = open_files.begin(); it != open_files.end();) {
								if (it->id == file->id) {
									it = open_files.erase(it);
								}
								else {
									++it;
								}
							}

							//Delete asset
							asset_service->removeFile(file->path);

							//Populate files and directories
							populateFiles(current_path);
						}

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

			std::function<void(std::any const&)> ResourcePanel::renameFilePopup(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {

					//Check for cast validity
					if (data.has_value() && data.type() == typeid(std::shared_ptr<File>)) {

						//Cast to file and remove file
						auto file = std::any_cast<std::shared_ptr<File>>(data);

						//Select a component to add
						ImGui::Text("Rename File To: ");

						//New folder name
						static std::string file_name = "";
						static bool name_init = false;
						if (!name_init) {
							file_name = file->path.stem().string();
							name_init = true;
						}
						file_name.resize(32);
						ImGui::InputText("##RenameFileName", file_name.data(), file_name.capacity() + 1);

						//Add spacing
						ImGui::Spacing();

						//Display each component as a button
						if (ImGui::Button("Rename")) {

							//Craft target path
							std::filesystem::path old_path = file->path;
							std::filesystem::path new_name = file_name.c_str();
							new_name.replace_extension(old_path.extension());
							std::filesystem::path target_path = old_path.parent_path() / new_name;

							//Rename file
							asset_service->moveFile(file->path, target_path);

							//Find open files and remove
							for (auto it = open_files.begin(); it != open_files.end();) {
								if (it->id == file->id) {
									it = open_files.erase(it);
								}
								else {
									++it;
								}
							}

							//Reset folder name buffer
							file_name.assign("");

							//Reset name init
							name_init = false;

							//Close popup
							closePopUp(popup_id);
						}

						//Same line
						ImGui::SameLine();

						//Cancel deleting asset
						if (ImGui::Button("Cancel")) {

							//Reset folder name buffer
							file_name.assign("");

							//Reset name init
							name_init = false;

							//Close popup
							closePopUp(popup_id);
						}
					}
					else {
						//Close popup
						closePopUp(popup_id);
					}
					};
			}


			std::function<void(std::any const&)> ResourcePanel::deleteFolderPopup(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {

					//Warning message
					ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "This action cannot be undone!");

					//Select a component to add
					ImGui::Text("Are you sure you want to delete folder? It's contents will be deleted as well.");

					//Add spacing
					ImGui::Spacing();

					//Display each component as a button
					if (ImGui::Button("Confirm")) {

						//Check for cast validity
						if (data.has_value() && data.type() == typeid(std::shared_ptr<Dir>)) {

							//Cast to file and remove file
							auto dir = std::any_cast<std::shared_ptr<Dir>>(data);

							//Delete asset
							asset_service->removeFile(dir->path);

							//Populate files and directories
							populateDirs(current_path);
						}

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

			std::function<void(std::any const&)> ResourcePanel::renameFolderPopup(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {

					//Check for cast validity
					if (data.has_value() && data.type() == typeid(std::shared_ptr<Dir>)) {

						//Cast to dir
						auto dir = std::any_cast<std::shared_ptr<Dir>>(data);

						//Select a component to add
						ImGui::Text("Rename Folder To: ");

						//New folder name
						static std::string file_name = "";
						static bool name_init = false;
						if (!name_init) {
							file_name = dir->path.stem().string();
							name_init = true;
						}
						file_name.resize(32);
						ImGui::InputText("##RenameFolderName", file_name.data(), file_name.capacity() + 1);

						//Add spacing
						ImGui::Spacing();

						//Display each component as a button
						if (ImGui::Button("Rename")) {

							//Craft target path
							std::filesystem::path old_path = dir->path;
							std::filesystem::path new_name = file_name.c_str();
							new_name.replace_extension(old_path.extension());
							std::filesystem::path target_path = old_path.parent_path() / new_name;

							//Rename file
							asset_service->moveFile(dir->path, target_path);

							//Reset folder name buffer
							file_name.assign("");

							//Reset name init
							name_init = false;

							//Close popup
							closePopUp(popup_id);
						}

						//Same line
						ImGui::SameLine();

						//Cancel deleting asset
						if (ImGui::Button("Cancel")) {

							//Reset folder name buffer
							file_name.assign("");

							//Reset name init
							name_init = false;

							//Close popup
							closePopUp(popup_id);
						}
					}
					else {
						//Close popup
						closePopUp(popup_id);
					}
					};
			}

			std::function<void(std::any const&)> ResourcePanel::newFolderPopup(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {

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
						path_service->createDirectory(current_path + "/" + folder_name);

						//Update directories & files
						populateDirs(current_path);

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

			void ResourcePanel::onAttach() {

				//Set up services
				path_service = services->get<Path::Path>();
				asset_service = services->get<Assets::Manager>();

				//Pop UP
				registerPopUp("Delete File", deleteFilePopup("Delete File"));
				registerPopUp("Rename File", renameFilePopup("Rename File"));
				registerPopUp("Delete Folder", deleteFolderPopup("Delete Folder"));
				registerPopUp("Rename Folder", renameFolderPopup("Rename Folder"));
				registerPopUp("New Folder", newFolderPopup("New Folder"));
				registerPopUp("Info", defPopUp("Info"));

				//Initialize root and current path
#ifdef PN_PLATFORM_ANDROID
				root_path = Path::assets_alias + "://";
#else
				root_path = Path::main_assets_alias + "://";
#endif
				root = path_service->resolvePath(root_path);
				current_path = root_path;

				//Update directories & files
				populateDirs(current_path);
				populateFiles(current_path);

				//Search up till 32 characters
				search_filter.resize(32);
				search_filter = "";

				//Default icon size
				icon_size = { 128.0f, 128.0f };

				std::shared_ptr<ResourcePanel> resourcepanel_wrapped(this, [](ResourcePanel*) {});
				std::weak_ptr<ResourcePanel> weak_this = resourcepanel_wrapped;

				//Setup directory watching 
				path_service->watchDirectoryTree(root_path, [this](std::filesystem::path const& file, filewatch::Event event) {

					//Skip directories & invalid paths
					if (std::filesystem::is_directory(file) || !std::filesystem::exists(file)) return;

					//Watch for events
					switch (event) {
					case filewatch::Event::added: {

						PN_CORE_INFO("Add Event for Path: {}", file.string());

						//Push to file event queue
						pushFileEvent([&, file]() {

							});

						break;
					}
					case filewatch::Event::removed: {

						PN_CORE_INFO("Remove Event for Path: {}", file.string());

						//Push to file event queue
						pushFileEvent([&, file]() {

							});

						break;
					}
					case filewatch::Event::modified: {

						PN_CORE_INFO("Modified Event for Path: {}", file.string());

						//Push to file event queue
						pushFileEvent([&, file]() {

							});

						break;
					}
					default: {
						break;
					}
					}
				});
			}

			void ResourcePanel::onDetach() {
				path_service->stopWatchingDirectoryTree(root_path);
			}

			void ResourcePanel::render() {

				////Update resource panel with file change events
				//while (!file_event_queue.empty()) {

				//	try {
				//		//Call callback function if valid
				//		if (file_event_queue.front()) {
				//			//Execute file event callback
				//			file_event_queue.front()();
				//		}

				//		//Pop from queue
				//		file_event_queue.pop();
				//	}
				//	catch (std::exception const&) {
				//		PN_CORE_WARN("Invalid Callback From FileWatcher Handled. Loop Continues.");
				//	}
				//}

				//Get the available width at the start of your layout
				float totalWidth = ImGui::GetContentRegionAvail().x;
				float sidebarWidth = totalWidth * SIDE_BAR_RATIO;
				float mainWidth = totalWidth - sidebarWidth;

				{
					ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), true);

					//Draw directory tree
					DrawDirectoryTree(root_path);

					ImGui::EndChild();
				}

				//Same line separataion between side bar and main content
				ImGui::SameLine();

				{
					ImGui::BeginChild("MainContent", ImVec2(mainWidth, 0), false, ImGuiWindowFlags_MenuBar);

					{
						ImGui::BeginMenuBar();

						//New folder
						{
							//Create new folder popup
							if (ImGui::Button("New Folder")) {
								openPopUp("New Folder");
							}
						}

						ImGui::Spacing();

						{
							//Show desc files trigger
							ImGui::Checkbox("Desc Files", &b_show_desc_files);
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
							if (ImGui::Button("Refresh") || auto_refresh_timer > AUTO_REFRESH_INTERVAL) {

								//Reset timer
								auto_refresh_timer = 0.0f;

								//Update directories & files
								populateDirs(current_path);
								populateFiles(current_path);
							}
						}

						ImGui::EndMenuBar();
					}

					//Render all assets & folders
					renderAssetsBrowser(current_path);

					//Render open files
					renderOpenFiles();

					//Render file editor
					renderFileEditor();

					//End main content child
					ImGui::EndChild();

					//Render popups
					renderPopUps();
				}
			}
        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif
