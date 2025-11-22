#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG

#include "pch.h"
#include "ResourcePanel.h"
#include "EntityPanel.h"
#include "ComponentsPanel.h"
#include "../Editor.h"

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include "CoreSystems/Events/GLFW/AssetEvents.h"

std::shared_ptr<PAIN::Assets::Model> PAIN::Editor::Panel::ResourcePanel::MaterialPreview::sphere_model = nullptr;
std::shared_ptr<PAIN::Assets::Shader> PAIN::Editor::Panel::ResourcePanel::MaterialPreview::shader = nullptr;
unsigned int PAIN::Editor::Panel::ResourcePanel::MaterialPreview::sphere_vao = 0;
unsigned int PAIN::Editor::Panel::ResourcePanel::MaterialPreview::sphere_vbo = 0;
unsigned int PAIN::Editor::Panel::ResourcePanel::MaterialPreview::sphere_ebo = 0;

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
				//Drop target
				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(payload_typestring + "_FILE").c_str())) {
						//Get asset ID
						File* file(static_cast<File*>(payload->Data));

						//Rename asset
						asset_service->moveFile(file->path, path_service->resolvePath(virtual_path + "/" + file->file_name));

						//Repopulate Files
						populateFiles(current_path);
					}
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string(payload_typestring + "DIR").c_str())) {
						//Get asset ID
						Dir* dir(static_cast<Dir*>(payload->Data));

						//Rename asset
						asset_service->moveFile(dir->path, path_service->resolvePath(virtual_path + "/" + dir->file_name));

						//Repopulate Dir
						populateDirs(current_path);
					}
					ImGui::EndDragDropTarget();
				}
			}

			void ResourcePanel::onEvent(Event::Event& event) {
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

							//SKip
							if (file_path == target) continue;

							//Throw asset into the game asset folder
							std::filesystem::copy(file_path, target, std::filesystem::copy_options::overwrite_existing);

							//Push msg
							msg.push_back(file_path.string());
						}

						//Craft message
						openPopUp("Info", std::make_shared<std::vector<std::string>>(msg));

						//Return false: continue dispatching, true = stop dispatching 
						return true;
						});
				}
			}

			void ResourcePanel::populateDirs(std::string const& virtual_path) {

				//Safety check
				std::string path;
				if (!path_service->pathExists(virtual_path)) {
					path = root_path;
				}
				else {
					path = virtual_path;
				}

				//Retrieve file
				auto fetch_dirs = path_service->listDirectories(path);

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

					//Get Folder icon
					auto icon_opt = services->get<Assets::Manager>()->getAsset<Assets::Texture>(folder_path);
					temp.icon = icon_opt.has_value() ? static_cast<ImTextureID>(icon_opt.value()->gl_texture) : 0;

					//Instantiate name
					temp.file_name = relative.filename().string();

					//Add this to file vector
					directories.push_back(temp);
				}
			}

			void ResourcePanel::populateFiles(std::string const& virtual_path) {

				//Safety check
				std::string path;
				if (!path_service->pathExists(virtual_path)) {
					path = root_path;
				}
				else {
					path = virtual_path;
				}

				//Retrieve file
				auto fetch_files = path_service->listFiles(path);

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

			void ResourcePanel::populateDirectoryCache(std::filesystem::path const& path) {
				if (directoryCache.count(path) > 0) return; // Already cached

				//Virtual path
				std::string virtual_path = root_path + "/" + std::filesystem::relative(path, root).string();

				std::vector<std::filesystem::path> children;
				std::filesystem::path dir = path;
				for (const auto& entry : path_service->listDirectories(virtual_path)) {
					children.push_back(entry);
				}
				directoryCache[path] = std::move(children);
			}

			void ResourcePanel::DrawDirectoryTree(std::filesystem::path const& path) {

				//Virtual path
				std::string virtual_path = root_path + "/" + std::filesystem::relative(path, root).string();

				//Populat directory cache
				populateDirectoryCache(path);
				bool has_children = !directoryCache[path].empty();

				ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_DefaultOpen;
				if (!has_children)
					node_flags |= ImGuiTreeNodeFlags_Leaf;
				bool is_selected = (path == path_service->resolvePath(current_path));
				if (is_selected) node_flags |= ImGuiTreeNodeFlags_Selected;

				std::filesystem::path dir = path;
				std::string name = dir.filename().string() != "" ? dir.filename().string() : "assets";
				bool open = ImGui::TreeNodeEx(name.c_str(), node_flags);

				//Check for activation
				if (ImGui::IsItemActivated() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
					current_path = virtual_path;
					populateDirs(current_path);
					populateFiles(current_path);
				}

				//Accept payload
				moveFileAcceptPayload(virtual_path);

				//Render context
				renderPopUpContext({dir,0, dir.filename().string() });

				//Render if open
				if (open) {
					for (const std::filesystem::path& subdir : directoryCache[path]) {
						DrawDirectoryTree(subdir);
					}
					ImGui::TreePop();
				}
				
				//Start drag-and-drop source
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {

					//Static copy of payload
					static Dir dir_copy;
					dir_copy.path = dir;
					dir_copy.file_name = dir.filename().string();

					//Get display icon
					static std::filesystem::path folder_path = "engine\\textures\\folder_icon.png";

					//Folder icon
					auto icon_opt = services->get<Assets::Manager>()->getAsset<Assets::Texture>(folder_path);
					dir_copy.icon = icon_opt.has_value() ? static_cast<ImTextureID>(icon_opt.value()->gl_texture) : 0;

					//Set drag payload with asset name
					ImGui::SetDragDropPayload(std::string("DIR").c_str(), &dir_copy, sizeof(dir_copy) + 1);

					//Display directory icon
					ImVec2 uv0(0.0f, 0.0f);
					ImVec2 uv1(1.0f, 1.0f);

					//Render the icon or name at the cursor during dragging
					ImGui::Image(dir_copy.icon, { 64, 64 }, uv0, uv1);
					ImGui::TextWrapped("%s", dir_copy.file_name.c_str());
					ImGui::EndDragDropSource();
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
						if (open_files.find(file) == open_files.end())open_files.insert(file);
					}
					if (ImGui::MenuItem("Rename##file")) {
						openPopUp("Rename File", std::make_shared<File>(file));
					}
					if (file.type == Assets::Type::Material && ImGui::MenuItem("New Material")) {

						//Create new default material
						openPopUp("New Material");
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

			bool ResourcePanel::renderPopUpContext(std::string const& virtual_path) {

				//Boolean break
				bool b_break = false;

				//Push ID
				ImGui::PushID(virtual_path.c_str());

				//Right-click context
				if (ImGui::BeginPopupContextWindow("AssetContextMenu##VirtualDirectory")) {
					//Decide the directory it is in
					auto relative = std::filesystem::relative(path_service->resolvePath(virtual_path), root);
					auto engine_material = Assets::getAllEngineFolders()[Assets::Type::Material];
					auto game_material = Assets::getAllGameFolders()[Assets::Type::Material];
					if ((relative == engine_material || relative == game_material) && ImGui::MenuItem("New Material")) {

						//Create new default material
						openPopUp("New Material");
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

					//Get texture
					auto texture = asset_service->getAsset<Assets::Texture>(icon_path);

					//Folder icon
					auto texture_opt = services->get<Assets::Manager>()->getAsset<Assets::Texture>(icon_path);

					//Check and ensure texture is not a cubemap
					if (texture_opt.has_value() && !texture.value()->is_cube_map) {
						return static_cast<ImTextureID>(texture.value()->gl_texture);
					}
				}

				//Folder icon
				auto def_icon_opt = services->get<Assets::Manager>()->getAsset<Assets::Texture>(def_icon_path);
				return def_icon_opt.has_value() ? static_cast<ImTextureID>(def_icon_opt.value()->gl_texture) : ImTextureID(0);
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

				//General context
				renderPopUpContext(virtual_path);

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
					ImTextureID icon = dir.icon ? dir.icon : 0;

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
							if (open_files.find(file) == open_files.end())open_files.insert(file);
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
					ImTextureID icon = file.icon ? file.icon : 0;

					//Display file icon
					ImVec2 uv0(0.0f, 0.0f);
					ImVec2 uv1(1.0f, 1.0f);
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
					ImGui::ImageButton(std::string("##" + file.file_name).c_str(), icon, ImVec2(icon_size.x, icon_size.y), uv0, uv1);
					if (ImGui::IsItemActivated() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						if(open_files.find(file) == open_files.end())open_files.insert(file);
					}
					
					if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && file.type == Assets::Type::Script){
						setSelectedFilePath(file.path.string());

						auto editor = services->get<PAIN::Editor::Editor>();

						auto entity_panel = entities_panel.lock();
						if (!entity_panel) {
							// Recover weak_ptr if it expired (happens on panel reload/scene change)
							if (editor) {
								auto ep = editor->getPanel<Panel::EntityPanel>();
								if (ep) {
									entities_panel = ep;
									entity_panel = ep;
								}
							}
						}

						auto component_panel = components_panel.lock();
						if (!component_panel) {
							// Recover weak_ptr if it expired (happens on panel reload/scene change)
							if (editor) {
								auto cp = editor->getPanel<Panel::ComponentsPanel>();
								if (cp) {
									components_panel = cp;
									component_panel = cp;
								}
							}
						}

						entity_panel->unselectEntity();
						component_panel->setScriptChanged(false);

					}
					else if (ImGui::IsItemActivated() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
						setSelectedFilePath("");
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

				//Display nothing
				if (shown_count == 0) {
					ImGui::Text("No results.");
				}
			}

			ResourcePanel::MaterialPreview::MaterialPreview() {
				glGenFramebuffers(1, &preview_fbo);
				glBindFramebuffer(GL_FRAMEBUFFER, preview_fbo);

				glGenTextures(1, &preview_texture);
				glBindTexture(GL_TEXTURE_2D, preview_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, preview_size.x, preview_size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, preview_texture, 0);

				glGenRenderbuffers(1, &preview_depth_rbo);
				glBindRenderbuffer(GL_RENDERBUFFER, preview_depth_rbo);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, preview_size.x, preview_size.y);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, preview_depth_rbo);

				assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
				glBindFramebuffer(GL_FRAMEBUFFER, 0);

				auto err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL error after Binding frame buffer: {}", err);
				}

			}

			void ResourcePanel::MaterialPreview::init() {
				// Generate and bind VAO
				glGenVertexArrays(1, &sphere_vao);
				glGenBuffers(1, &sphere_vbo);
				glGenBuffers(1, &sphere_ebo);
				glBindVertexArray(sphere_vao);

				glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
				glBufferData(GL_ARRAY_BUFFER,
					sphere_model->vertices.size() * sizeof(Assets::Vertex),
					sphere_model->vertices.data(), GL_STATIC_DRAW);

				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					sphere_model->indices.size() * sizeof(unsigned int),
					sphere_model->indices.data(), GL_STATIC_DRAW);

				// Position attribute, layout(location = 0)
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, pos));
				glEnableVertexAttribArray(0);

				// Normal attribute, layout(location = 1)
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, normal));
				glEnableVertexAttribArray(1);

				// texcoords attribute, layout(location = 2)
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, uv));
				glEnableVertexAttribArray(2);

				// Unbind VAO
				glBindVertexArray(0);

				auto err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL error after initializing sphere vao/vbo/ebo: {}", err);
				}
			}

			void ResourcePanel::MaterialPreview::render(std::shared_ptr<const Assets::Material> material) {
				// Prepare preview FBO
				glViewport(0, 0, preview_size.x, preview_size.y);
				glBindFramebuffer(GL_FRAMEBUFFER, preview_fbo);
				glEnable(GL_DEPTH_TEST);
				glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				// Setup camera (static position, orbital, whatever looks good)
				glm::mat4 proj = glm::perspective(glm::radians(45.0f), float(preview_size.x) / float(preview_size.y), 0.1f, 10.0f);
				glm::vec3 cam_pos = glm::vec3(0.0f, 0.0f, 2.5f);
				glm::mat4 view = glm::lookAt(cam_pos, glm::vec3(0.0f), glm::vec3(0, 1, 0));
				glm::mat4 model = glm::mat4(1.0f);

				//Bind shader
				shader->Bind();

				// Matrices
				shader->SetUniform("u_M", model);
				shader->SetUniform("u_V", view);
				shader->SetUniform("u_P", proj);

				// Camera
				shader->SetUniform("u_CamPos", cam_pos);

				// Light
				shader->SetUniform("u_LightPos", glm::vec3(3.0f, 2.0f, 3.0f));
				shader->SetUniform("u_LightColor", glm::vec3(20.0f));
				shader->SetUniform("u_AmbientLight", glm::vec3(0.03f));

				// Material properties (NOW THESE EXIST IN THE SHADER!)
				shader->SetUniform("u_BaseColor", material->baseColor);
				shader->SetUniform("u_Metallic", material->metallic);
				shader->SetUniform("u_Roughness", material->roughness);

				//Bind vertex array
				glBindVertexArray(sphere_vao);

				//Draw elements
				for (size_t i = 0; i < sphere_model->submeshes.size(); ++i) {
					// TEMPORARY: Only render first submesh
					const auto& submesh = sphere_model->submeshes[i];

					//// Material properties
					//shader->SetUniform("material.rough", material->roughness);
					//shader->SetUniform("material.metal", material->metallic);
					//shader->SetUniform("material.color", material->baseColor);

					//// Bind textures from MaterialInstance
					//bool hasTexture = albedoTexture != 0;
					//shader->SetUniform("material.useTex", hasTexture ? 1.0f : 0.0f);
					//shader->SetUniform("material.alwaysLit", emissiveTexture ? 1.f : 0.f);

					//if (hasTexture) {
					//	glActiveTexture(GL_TEXTURE6);
					//	glBindTexture(GL_TEXTURE_2D, albedoTexture);
					//	shader->SetUniform("material.tex", 6);

					//	if (aoTexture != 0) {
					//		glActiveTexture(GL_TEXTURE7);
					//		glBindTexture(GL_TEXTURE_2D, aoTexture);
					//		shader->SetUniform("material.ao_map", 7);
					//		shader->SetUniform("material.use_ao", 1.0f);
					//	}
					//	else {
					//		shader->SetUniform("material.use_ao", 0.0f);
					//	}
					//}

					// Draw this submesh
					glDrawElements(
						GL_TRIANGLES,
						submesh.indexCount,
						GL_UNSIGNED_INT,
						(void*)(submesh.firstIndex * sizeof(unsigned int))
					);
				}

				//Unbind shader
				//shader->UnBind();

				//Unbind frame buffer
				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			void ResourcePanel::renderOpenFiles() {

				//Local files to close
				std::vector<File> files_to_close;

				//Iterate through all open files
				for (auto const& file : open_files) {

					//Begin window
					ImGui::Begin(file.file_name.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize);

					//Render asset type
					switch (file.type) {
					case Assets::Type::Material: {
						auto mat_opt = asset_service->getAsset<Assets::Material>(file.id);
						if (mat_opt.has_value()) {
							mat_preview.render(mat_opt.value());
							ImVec2 icon_size(256, 256);
							ImGui::Image(static_cast<ImTextureID>(mat_preview.getPreviewTexture()), icon_size);
						}
						else {
							//Display icon
							ImVec2 icon_size(256, 256);
							ImGui::Image(file.icon, icon_size);
						}
						break;
					}
					default: {
						//Display icon
						ImVec2 icon_size(256, 256);
						ImGui::Image(file.icon, icon_size);
					}
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

			std::string ResourcePanel::getSelectedFilePath() {
				return selected_filepath;
			}

			void ResourcePanel::setSelectedFilePath(std::string filepath) {
				selected_filepath = filepath;
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

							//Repopulate files
							populateFiles(current_path);

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

							//Disable deletion of root path
							if (dir->path != root) {

								//Delete asset
								asset_service->removeFile(dir->path);

								//Populate files and directories
								populateDirs(current_path);

								//Reset directory cache
								directoryCache.clear();

								//Check if current path is present within deleted folder
								if (path_service->resolvePath(current_path) == dir->path) {
									current_path = root_path;
								}
							}
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

							//Reset directory cache
							directoryCache.clear();

							//Check if current path is in the previous path
							if (path_service->resolvePath(current_path) == old_path) {
								current_path = root_path + "/" + std::filesystem::relative(target_path, root).string();
							}

							//Repopulate dir
							populateDirs(current_path);

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

						//Reset directory cache
						directoryCache[current_path].clear();

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

			std::function<void(std::any const&)> ResourcePanel::newMaterialPopup(std::string const& popup_id) {
				return [this, popup_id](std::any const& data) {

					//Select a component to add
					ImGui::Text("New material name without extension: ");

					//New folder name
					static std::string mat_name = "";
					mat_name.resize(32);
					ImGui::InputText("##Newmat_name", mat_name.data(), mat_name.capacity() + 1);

					//Add spacing
					ImGui::Spacing();

					//Display each component as a button
					if (ImGui::Button("Create")) {

						//Craft outpath
						std::filesystem::path out_path = path_service->resolvePath(current_path);
						out_path /= mat_name.c_str();
						out_path.replace_extension(*Assets::getAllExtensions()[Assets::Type::Material].begin());

						//Create material
						Assets::Material def_material;
						asset_service->createNewMaterial(def_material, out_path);

						//Update directories & files
						populateFiles(current_path);

						//Reset folder name buffer
						mat_name.assign("");

						//Close popup
						closePopUp(popup_id);
					}

					//Same line
					ImGui::SameLine();

					//Cancel deleting asset
					if (ImGui::Button("Cancel")) {

						//Reset folder name buffer
						mat_name.assign("");

						//Close popup
						closePopUp(popup_id);
					}
					};
			}

			void ResourcePanel::pushFileEvent(std::filesystem::path const& file, filewatch::Event const& event, std::function<void()>&& callback) {

				std::lock_guard<std::mutex> lock(file_event_mutex);
				event_functions[file].callback[event] = std::move(callback);
				event_functions[file].last_updated = std::chrono::steady_clock::now();
			}

			void ResourcePanel::onAttach() {

				//Set up services
				path_service = services->get<Path::Path>();
				asset_service = services->get<Assets::Manager>();

				//Init material preview
				auto sphere_opt = asset_service->getAsset<Assets::Model>("engine\\models\\sphere.obj");
				mat_preview.sphere_model = sphere_opt.has_value() ? sphere_opt.value() : nullptr;
				auto shader_opt = asset_service->getAsset<Assets::Shader>("engine\\shaders\\pbr_preview.vert");
				mat_preview.shader = shader_opt.has_value() ? shader_opt.value() : nullptr;
				mat_preview.init();

				//Pop UP
				registerPopUp("Delete File", deleteFilePopup("Delete File"));
				registerPopUp("Rename File", renameFilePopup("Rename File"));
				registerPopUp("Delete Folder", deleteFolderPopup("Delete Folder"));
				registerPopUp("Rename Folder", renameFolderPopup("Rename Folder"));
				registerPopUp("New Folder", newFolderPopup("New Folder"));
				registerPopUp("New Material", newMaterialPopup("New Material"));
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

				//Setup directory watching 
				path_service->watchDirectoryTree(root_path, [this](std::filesystem::path const& file, filewatch::Event event) {

					//Skip desc paths
					if (file.extension() == Assets::descriptor_ext) return;

					//Check for removed directories or renamed directories
					if (event == filewatch::Event::removed || event == filewatch::Event::renamed_old) {

						//Lock events and update current path
						std::lock_guard<std::mutex> lock(file_event_mutex);

						//Check for invalidated current path
						if (file == path_service->resolvePath(current_path)) current_path = root_path;

						//Check if path is in directory cache
						if (directoryCache.find(file) != directoryCache.end()) {
							directoryCache[file].clear();
						}
					}

					//Watch for events
					switch (event) {
					case filewatch::Event::added: {

						//Push to file event queue
						pushFileEvent(file, event, [&, file]() {

							//Log added event
							PN_CORE_INFO("Add Event for Path: {}", file.string());

							//Register asset
							if (std::filesystem::is_directory(file)) {

								//Recursive register all assets in directory
								for (auto const& entry : std::filesystem::recursive_directory_iterator(file)) {
									if (entry.path().extension() == Assets::descriptor_ext) continue;
									auto relative = std::filesystem::relative(entry.path(), root);
									asset_service->registerAsset(relative);
								}
							}
							else {
								auto relative = std::filesystem::relative(file, root);
								asset_service->registerAsset(relative);
							}

							});

						break;
					}
					case filewatch::Event::removed: {

						//Push to file event queue
						pushFileEvent(file, event, [&, file]() {

							//Log removed event
							PN_CORE_INFO("Remove Event for Path: {}", file.string());

							//Unregister asset
							auto relative = std::filesystem::relative(file, root);
							asset_service->unregisterAsset(relative);

							});

						break;
					}
					case filewatch::Event::modified: {

						//Push to file event queue
						pushFileEvent(file, event, [&, file]() {

							//Log modified event
							PN_CORE_INFO("Modified Event for Path: {}", file.string());

							//Check modified event for directory
							if (std::filesystem::is_directory(file)) return;

							//Get relative path
							auto relative = std::filesystem::relative(file, root);

							//Check asset been registered
							if (!asset_service->checkAssetRegistered(relative)) {
								//Register asset
								if (std::filesystem::is_directory(file)) {

									//Recursive register all assets in directory
									for (auto const& entry : std::filesystem::recursive_directory_iterator(file)) {
										if (entry.path().extension() == Assets::descriptor_ext) continue;
										auto relative = std::filesystem::relative(entry.path(), root);
										asset_service->registerAsset(relative);
									}
								}
								else {
									auto relative = std::filesystem::relative(file, root);
									asset_service->registerAsset(relative);
								}
							}
							else {
								//Get GUID
								auto id = asset_service->findGUID(relative);

								//Check if cached
								if (asset_service->checkAssetCached(id)) {
									asset_service->recacheAsset(id);
								}
							}

							});

						break;
					}
					case filewatch::Event::renamed_new: {

						//Push to file event queue
						pushFileEvent(file, event, [&, file]() {

							//Log modified event
							PN_CORE_INFO("Renamed New Path Event for Path: {}", file.string());

							//Register asset
							if (std::filesystem::is_directory(file)) {

								//Recursive register all assets in directory
								for (auto const& entry : std::filesystem::recursive_directory_iterator(file)) {
									auto relative = std::filesystem::relative(file, root);
									asset_service->registerAsset(relative);
								}
							}
							else {
								auto relative = std::filesystem::relative(file, root);
								asset_service->registerAsset(relative);
							}

							});

						break;
					}
					case filewatch::Event::renamed_old: {

						//Push to file event queue
						pushFileEvent(file, event, [&, file]() {

							//Log modified event
							PN_CORE_INFO("Renamed Old Path Event for Path: {}", file.string());

							//Unregister old
							auto relative = std::filesystem::relative(file, root);
							asset_service->unregisterAsset(relative);

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

				//Update resource panel with file change events
				{
					std::lock_guard<std::mutex> lock(file_event_mutex);

					auto now = std::chrono::steady_clock::now();
					for (auto it = event_functions.begin(); it != event_functions.end(); ) {
						if (now - it->second.last_updated > debounce_time) {

							//Final callback
							std::function<void()> callback;

							//Get all callbacks
							auto callbacks = it->second.callback;

							// Priority logic
							if (callbacks[filewatch::Event::renamed_new]) {
								callback = callbacks[filewatch::Event::renamed_new];
							}
							else if (callbacks[filewatch::Event::renamed_old]) {
								callback = callbacks[filewatch::Event::renamed_old];
							}
							else if (callbacks[filewatch::Event::removed] && callbacks[filewatch::Event::added] && callbacks[filewatch::Event::modified]) {
								callback = callbacks[filewatch::Event::modified];
							}
							else if (callbacks[filewatch::Event::removed] && callbacks[filewatch::Event::added]) {
								callback = nullptr;
							}
							else if (callbacks[filewatch::Event::removed]) {
								callback = callbacks[filewatch::Event::removed];
							}
							else if (callbacks[filewatch::Event::added]) {
								callback = callbacks[filewatch::Event::added];
							}
							else if (callbacks[filewatch::Event::modified]) {
								callback = callbacks[filewatch::Event::modified];
							}
							else {
								callback = nullptr;
							}

							//Decide which event to push
							file_event_queue.push(std::move(callback));

							//Erase event
							it = event_functions.erase(it);
						}
						else {
							++it;
						}
					}

					while (!file_event_queue.empty()) {

						try {

							//Ensure function
							if (file_event_queue.front()) {
								//Execute file event callback
								file_event_queue.front()();
							}

							//Pop from queue
							file_event_queue.pop();
						}
						catch (std::exception const&) {
							PN_CORE_WARN("Invalid Callback From FileWatcher Handled. Loop Continues.");

							//Pop from queue
							file_event_queue.pop();
						}
					}
				}

				//Get the available width at the start of your layout
				float totalWidth = ImGui::GetContentRegionAvail().x;
				float sidebarWidth = totalWidth * SIDE_BAR_RATIO;
				float mainWidth = totalWidth - sidebarWidth;

				{
					ImGui::BeginChild("Sidebar", ImVec2(sidebarWidth, 0), true);

					//Draw directory tree
					DrawDirectoryTree(root);

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

								//Reset directory cache
								directoryCache.clear();
								populateDirectoryCache(current_path);

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
