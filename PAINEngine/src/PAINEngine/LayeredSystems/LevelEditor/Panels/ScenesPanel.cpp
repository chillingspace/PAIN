#include "pch.h"
#include "ScenesPanel.h"

#ifdef _DEBUG
#include "CoreSystems/Serialization/sSerialization.h"

namespace PAIN {
    namespace Editor {
        namespace Panel {

            static inline std::string makeSceneId(const std::string& base) {
                // visible id stored with extension for now
                return base.empty() ? std::string{} : (base + ".scn");
            }

            static inline std::string stripExt(const std::string& s) {
                const auto p = s.rfind('.');
                return (p == std::string::npos) ? s : s.substr(0, p);
            }

            ScenesPanel::ScenesPanel(ScenesHooks hooks)
                : hooks_(std::move(hooks)) {

                name = "Scene Manager";                 // visible window title
                flags = ImGuiWindowFlags_None;    // normal tool window

                // seed with a default layer so UI is usable immediately
                layers_.push_back(Layer{ 0, true });
                rebuildMaskSize(layers_.size());
            }

            void ScenesPanel::nextWindowSettings() {
                // default window; leave docking/frameless tricks to ToolsPanel only
            }

            std::function<void(std::any const&)> ScenesPanel::createScenePopup(std::string const& popup_id)
            {
                return [this, popup_id](std::any const&) {
                    ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Create New Scene");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Scene Name:");
                    ImGui::SameLine();
                    ImGui::InputText("##SceneName", nameBuf_, IM_ARRAYSIZE(nameBuf_));

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    float button_width = 120.0f;
                    float spacing = ImGui::GetStyle().ItemSpacing.x;
                    float total_width = (button_width * 2) + spacing;
                    float offset = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
                    if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                    bool create_clicked = ImGui::Button("Create", ImVec2(button_width, 0));
                    ImGui::SameLine();
                    bool cancel_clicked = ImGui::Button("Cancel", ImVec2(button_width, 0));

                    if (create_clicked) {
                        if (hooks_.onCreate) hooks_.onCreate(std::string{ nameBuf_ });
                        currSceneId_ = std::string{ nameBuf_ } + ".scn";
                        auto ser = services->get<Serialization::Service>();
                        ser->setGrid(0);
                        const auto& doc = ser->doc();
                        if (doc.layers.empty()) ser->addLayer();
                        // F
                        services->get<ECS::Controller>()->destroyAllEntities();
                        closePopUp(popup_id);
                    }

                    if (cancel_clicked) {
                        closePopUp(popup_id);
                    }
                };
            }

            std::function<void(std::any const&)> ScenesPanel::saveSceneAsPopup(std::string const& popup_id)
            {
                return [this, popup_id](std::any const&) {
                    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Save Scene As");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("New Scene Name:");
                    ImGui::SameLine();
                    ImGui::InputText("##SaveSceneName", nameBuf_, IM_ARRAYSIZE(nameBuf_));

                    ImGui::Spacing();
                    ImGui::TextWrapped("This will create a new scene file with the specified name.");
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    float button_width = 120.0f;
                    float spacing = ImGui::GetStyle().ItemSpacing.x;
                    float total_width = (button_width * 2) + spacing;
                    float offset = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
                    if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                    bool save_clicked = ImGui::Button("Save As", ImVec2(button_width, 0));
                    ImGui::SameLine();
                    bool cancel_clicked = ImGui::Button("Cancel", ImVec2(button_width, 0));

                    if (save_clicked) {
                        if (hooks_.onSaveAs) hooks_.onSaveAs(std::string{ nameBuf_ });
                        currSceneId_ = std::string{ nameBuf_ } + ".scn";
                        closePopUp(popup_id);
                    }
                    if (cancel_clicked) {
                        closePopUp(popup_id);
                    }
                    };
            }

            std::function<void(std::any const&)> ScenesPanel::deleteScenePopup(std::string const& popup_id)
            {
                return [this, popup_id](std::any const&) {
                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f), "Delete Scene");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::TextWrapped("Are you sure you want to delete this scene?");
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "This action cannot be undone.");
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    float button_width = 120.0f;
                    float spacing = ImGui::GetStyle().ItemSpacing.x;
                    float total_width = (button_width * 2) + spacing;
                    float offset = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
                    if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                    bool delete_clicked = ImGui::Button("Delete", ImVec2(button_width, 0));
                    ImGui::SameLine();
                    bool cancel_clicked = ImGui::Button("Cancel", ImVec2(button_width, 0));

                    if (delete_clicked) {
                        if (hooks_.onDelete) hooks_.onDelete(currSceneId_);
                        currSceneId_.clear();
                        closePopUp(popup_id);
                    }
                    if (cancel_clicked) {
                        closePopUp(popup_id);
                    }
                    };
            }

            void ScenesPanel::ensureAtLeastOneLayer() {
                if (layers_.empty()) {
                    layers_.push_back(Layer{ 0, true });
                    rebuildMaskSize(1);
                    selectedLayerIdx_ = 0;
                }
            }

            void ScenesPanel::rebuildMaskSize(std::size_t n) {
                mask_.assign(n, std::vector<bool>(n, false));
                for (std::size_t i = 0; i < n; ++i) mask_[i][i] = false; 
            }

            std::string ScenesPanel::baseNameFromId(const std::string& sceneId) {
                return stripExt(sceneId);
            }

            // Modals
            void ScenesPanel::drawCreateModal() {
                if (!showCreate_) return;
                ImGui::OpenPopup("Create Scene");
                if (ImGui::BeginPopupModal("Create Scene", &showCreate_, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::TextUnformatted("Enter a name for the scene (without .scn):");
                    static char buf[256];
                    ImGui::InputText("##SaveAsName", buf, IM_ARRAYSIZE(buf));
                    tmpNameBuf_ = buf; // copy back into std::string if you want to keep it

                    ImGui::Spacing();

                    const bool valid = !tmpNameBuf_.empty() && (tmpNameBuf_.find(".scn") == std::string::npos);
                    if (ImGui::Button("Ok") && valid) {
                        // reset "scene" in our temporary model
                        currSceneId_ = makeSceneId(tmpNameBuf_);

                        // optional external hook
                        if (hooks_.onCreate) hooks_.onCreate(tmpNameBuf_);
                        if (hooks_.onChange) hooks_.onChange(currSceneId_);

                        tmpNameBuf_.clear();
                        showCreate_ = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        tmpNameBuf_.clear();
                        showCreate_ = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }

            void ScenesPanel::drawDeleteModal() {
                if (!showDelete_) return;
                ImGui::OpenPopup("Delete Scene");
                if (ImGui::BeginPopupModal("Delete Scene", &showDelete_, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "This action cannot be undone!");
                    ImGui::TextUnformatted("Are you sure you want to delete this scene?");
                    ImGui::Spacing();

                    if (ImGui::Button("Confirm")) {
                        if (hooks_.onDelete) hooks_.onDelete(currSceneId_);
                        currSceneId_.clear();
                        showDelete_ = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        showDelete_ = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }

            void ScenesPanel::drawSaveAsModal() {
                if (!showSaveAs_) return;
                ImGui::OpenPopup("Save Scene As");
                if (ImGui::BeginPopupModal("Save Scene As", &showSaveAs_, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::TextUnformatted("Enter a new name (without .scn):");
                    static char buf[256];
                    ImGui::InputText("##SaveAsName", buf, IM_ARRAYSIZE(buf));
                    tmpNameBuf_ = buf; // copy back into std::string if you want to keep it

                    ImGui::Spacing();

                    const bool valid = !tmpNameBuf_.empty() && (tmpNameBuf_.find(".scn") == std::string::npos);
                    if (ImGui::Button("Ok") && valid) {
                        if (hooks_.onSaveAs) hooks_.onSaveAs(tmpNameBuf_);
                        currSceneId_ = makeSceneId(tmpNameBuf_);
                        if (hooks_.onChange) hooks_.onChange(currSceneId_);
                        tmpNameBuf_.clear();
                        showSaveAs_ = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        tmpNameBuf_.clear();
                        showSaveAs_ = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }

            void ScenesPanel::drawEditMaskModal() {
                if (!showEditMask_) return;

                auto ser = services->get<Serialization::Service>();
                if (!ser) { showEditMask_ = false; return; }
                const auto& doc = ser->doc(); // read-only view

                ImGui::OpenPopup("Edit Layer Bit Mask");
                if (!ImGui::BeginPopupModal("Edit Layer Bit Mask", &showEditMask_, ImGuiWindowFlags_AlwaysAutoResize))
                    return;

                const unsigned n = static_cast<unsigned>(doc.layers.size());
                // make sure the service has an NxN mask (diagonal forced false)
                ser->ensureMaskSize();

                ImGui::PushID("MaskGrid");
                ImGui::TextUnformatted("Bitmask Grid:");

                if (n > 0 && ImGui::BeginTable("##BitmaskGrid", n + 1,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame)) {

                    // Header
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("Layer\\Mask");
                    for (unsigned col = 0; col < n; ++col) {
                        ImGui::TableNextColumn();
                        ImGui::Text("L%u", doc.layers[col].id);
                    }

                    // Rows
                    for (unsigned i = 0; i < n; ++i) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("L%u", doc.layers[i].id);

                        for (unsigned j = 0; j < n; ++j) {
                            ImGui::TableNextColumn();

                            if (i == j) { ImGui::TextUnformatted("X"); continue; }

                            // get current bit safely
                            bool bit = (i < doc.mask_matrix.size() &&
                                j < doc.mask_matrix[i].size()) ? doc.mask_matrix[i][j] : false;

                            // unique ID per cell
                            const std::string id = "##m_" + std::to_string(i) + "_" + std::to_string(j);
                            if (ImGui::Checkbox(id.c_str(), &bit)) {
                                // write back via service (keeps symmetry & marks dirty)
                                ser->setMask(i, j, bit);
                                if (hooks_.onDirty) hooks_.onDirty();
                                if (hooks_.onMaskChanged) hooks_.onMaskChanged(i, j, bit);
                            }
                        }
                    }

                    ImGui::EndTable();
                }

                ImGui::Spacing();
                if (ImGui::Button("Done##MaskGrid")) {
                    showEditMask_ = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::PopID();
                ImGui::EndPopup();
            }

            void ScenesPanel::drawSkyboxSettingsPanel()
            {
             
            }

            void ScenesPanel::onAttach()
            {
                registerPopUp("CreateScene", createScenePopup("CreateScene"));
                registerPopUp("SaveSceneAs", saveSceneAsPopup("SaveSceneAs"));
                registerPopUp("DeleteScene", deleteScenePopup("DeleteScene"));
            }

            // ---------- Main draw ----------
            void ScenesPanel::onUpdate(AppTiming timing) {
                auto ser = services->get<Serialization::Service>();
                const auto& doc = ser->doc(); // read for drawing

                // Title & dock are handled by IPanel
                if (ser->consumeSceneChanged()) {
                    currSceneId_ = ser->getCurrSceneId();
                    ser->markSceneChanged();
                }

                ImGui::Text("Scene ID: %s", currSceneId_.empty() ? "(none)" : currSceneId_.c_str());

                // Create New Scene
                if (ImGui::Button("Create New Scene")) {
                    openPopUp("CreateScene");
                }


                std::shared_ptr<Assets::Manager> asset_service = nullptr;

                // Prevent null stuff to happen
                if(services->get<Assets::Manager>()) {
                    asset_service = services->get<Assets::Manager>();
                }


                auto scenes = asset_service->getAllAssetDataOfType(Assets::Type::Scenes);

                // Show dropdown of available scenes
                if (!scenes.empty()) {
                    std::vector<const char*> scene_names;
                    for (const auto& scene : scenes) {
                        if (scene) {
                            scene_names.push_back(scene.get()->name.c_str()); 
                        }

                    }

                    // Combo box for scene selection
                    if (ImGui::Combo("Select Scene", &selected_scene_index, scene_names.data(), scene_names.size())) {
                        // Optional: handle selection change if needed
                        // When selection changes, update path text box
                        if (selected_scene_index >= 0 && selected_scene_index < scene_names.size()) {
                            selected = asset_service->findGUID(scene_names[selected_scene_index]);
                        }
                    }

                    // Get selected scene ID (or name)
                    auto& selected_scene = scenes[selected_scene_index]; 
                    std::string id = selected_scene->name; // Use id or name as required

                    // Button to load the selected scene
                    if (ImGui::Button("Load Scene")) {
                        bool success = hooks_.onChange ? hooks_.onChange(id) : false; 
                        if (success) {
                            currSceneId_ = id; // update panel label
                        }
                        else {
                            showSceneLoadError_ = true;
                            loadSceneErrorMsg_ = "Failed to load scene (file may not exist or be invalid):\n" + id;
                        }
                    }
                }
                else {
                    ImGui::TextDisabled("No scenes available!");
                }

                

                if (!currSceneId_.empty()) {
                    ImGui::SameLine();
                    if (ImGui::Button("Delete Scene")) {
                        openPopUp("DeleteScene");
                    }
                    if (ImGui::Button("Save Scene As")) {
                        openPopUp("SaveSceneAs");
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Save Curr Scene")) {
                        if (hooks_.onSaveCurrent) hooks_.onSaveCurrent(currSceneId_);
                    }
                }

                ImGui::Separator();

                // Graphics Settings
                drawSkyboxSettingsPanel();

                // Layers
                //ImGui::Text("Total Layers: %u", (unsigned)doc.layers.size());

                //if (ImGui::Button("Create Layer")) {
                //    ser->addLayer();
                //}
                //ImGui::SameLine();
                //if (ImGui::Button("Remove Layer")) {
                //    // pick a selected index from your panel state; here assume 0 for sample
                //    unsigned sel = std::min<unsigned>(selectedLayerIdx_, (unsigned)doc.layers.size() - 1);
                //    ser->removeLayer(sel);
                //    selectedLayerIdx_ = (unsigned)std::min<size_t>(selectedLayerIdx_, doc.layers.size() ? doc.layers.size() - 1 : 0);
                //}

                //ImGui::TextUnformatted("Layer List:");
                //ImGui::BeginChild("##LayerList", ImVec2(0, 200), true);
                //for (unsigned i = 0; i < doc.layers.size(); ++i) {
                //    bool vis = doc.layers[i].enabled;
                //    if (ImGui::Checkbox((std::string("##vis_") + std::to_string(i)).c_str(), &vis)) {
                //        ser->setLayerVisible(i, vis);        // <- write to service
                //        if (hooks_.onLayerVisibleChanged) hooks_.onLayerVisibleChanged(i, vis);
                //        if (hooks_.onDirty)               hooks_.onDirty();
                //    }
                //    ImGui::SameLine();
                //    std::string label = "Layer " + std::to_string(doc.layers[i].id);
                //    if (ImGui::Selectable(label.c_str(), selectedLayerIdx_ == i)) {
                //        selectedLayerIdx_ = i;
                //    }
                //}
                //ImGui::EndChild();

                //if (ImGui::Button("Edit Layer Bit Mask")) { showEditMask_ = true; }

                // Modals last
                drawCreateModal();
                drawDeleteModal();
                drawSaveAsModal();
                drawEditMaskModal();

                // Show the error popup when load scene fails
                if (showSceneLoadError_) {
                    ImGui::OpenPopup("Scene Load Error");
                    showSceneLoadError_ = false;
                }

                // Render the modal if it's open
                if (ImGui::BeginPopupModal("Scene Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                    ImGui::TextWrapped("%s", loadSceneErrorMsg_.c_str());
                    ImGui::Spacing();
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                renderPopUps();
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif