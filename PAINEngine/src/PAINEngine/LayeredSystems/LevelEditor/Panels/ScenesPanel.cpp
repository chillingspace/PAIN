#include "pch.h"
#include "ScenesPanel.h"

#ifdef _DEBUG
#include "CoreSystems/Serialization/sSerialization.h"
#include "ECS/Controller.h"
#include "CoreSystems/Scene/Scene.h"

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

            ScenesPanel::ScenesPanel() {

                name = "Scene Manager";                 // visible window title
                flags = ImGuiWindowFlags_None;    // normal tool window
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
                        services->get<Scene::SceneManager>()->createScene(std::string(nameBuf_));
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
                        services->get<Scene::SceneManager>()->saveActiveScene(selected, std::string{ nameBuf_ });
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
                        services->get<Scene::SceneManager>()->deleteScene(selected);
                        closePopUp(popup_id);
                    }
                    if (cancel_clicked) {
                        closePopUp(popup_id);
                    }
                    };
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
                        services->get<Scene::SceneManager>()->createScene(std::string(nameBuf_));

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
                        services->get<Scene::SceneManager>()->deleteScene(selected);
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
                        services->get<Scene::SceneManager>()->saveActiveScene(selected, std::string(nameBuf_));
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

                //Get scene service
                auto scn_service = services->get<Scene::SceneManager>();

                //Get layers and matrix
                auto& layers = scn_service->getLayers();
                auto& maskMatrix = scn_service->getMaskMatrix();

                // Ensure mask matrix is correct size
                const unsigned n = static_cast<unsigned>(layers.size());
                if (maskMatrix.size() != n) {
                    maskMatrix.resize(n);
                    for (auto& row : maskMatrix) {
                        row.resize(n, false);
                    }
                }

                ImGui::OpenPopup("Edit Layer Collision Matrix");
                if (!ImGui::BeginPopupModal("Edit Layer Collision Matrix", &showEditMask_,
                    ImGuiWindowFlags_AlwaysAutoResize)) {
                    return;
                }

                ImGui::TextWrapped("This matrix defines which layers can interact with each other.");
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                    "Checked = layers can collide/interact");
                ImGui::Separator();
                ImGui::Spacing();

                if (n > 0 && ImGui::BeginTable("##CollisionMatrix", n + 1,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg)) {

                    // Header row
                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Layer \\ Layer");

                    for (unsigned col = 0; col < n; ++col) {
                        ImGui::TableNextColumn();
                        ImGui::Text("L%d", layers[col].id);
                    }

                    // Data rows
                    for (unsigned i = 0; i < n; ++i) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("L%d", layers[i].id);

                        for (unsigned j = 0; j < n; ++j) {
                            ImGui::TableNextColumn();

                            // Diagonal is always disabled (layer can't interact with itself)
                            if (i == j) {
                                ImGui::TextDisabled("X");
                                continue;
                            }

                            // Get current bit
                            bool canInteract = maskMatrix[i][j];

                            const std::string id = "##mask_" + std::to_string(i) + "_" + std::to_string(j);
                            if (ImGui::Checkbox(id.c_str(), &canInteract)) {
                                // Update matrix symmetrically
                                maskMatrix[i][j] = canInteract;
                                maskMatrix[j][i] = canInteract;
                            }
                        }
                    }

                    ImGui::EndTable();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Quick actions
                if (ImGui::Button("Enable All")) {
                    for (unsigned i = 0; i < n; ++i) {
                        for (unsigned j = 0; j < n; ++j) {
                            if (i != j) {
                                maskMatrix[i][j] = true;
                            }
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Disable All")) {
                    for (auto& row : maskMatrix) {
                        std::fill(row.begin(), row.end(), false);
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Done")) {
                    showEditMask_ = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            void ScenesPanel::drawLayerManagementPanel() {
                //Get scene service
                auto scn_service = services->get<Scene::SceneManager>();

                //Get layers and matrix
                auto& layers = scn_service->getLayers();
                auto& maskMatrix = scn_service->getMaskMatrix();

                ImGui::Indent();

                ImGui::Text("Total Layers: %u", static_cast<unsigned>(layers.size()));

                // Add/Remove layer buttons
                if (ImGui::Button("Add Layer")) {
                    // Create new layer with next available ID
                    int newId = 0;
                    for (const auto& layer : layers) {
                        if (layer.id >= newId) newId = layer.id + 1;
                    }

                    Scene::Layer newLayer;
                    newLayer.id = newId;
                    newLayer.mask = 1 << newId; // Unique bit mask
                    newLayer.enabled = true;
                    layers.push_back(newLayer);

                    // Resize mask matrix
                    size_t n = layers.size();
                    maskMatrix.resize(n);
                    for (auto& row : maskMatrix) {
                        row.resize(n, false);
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Remove Selected") && !layers.empty()) {
                    if (selectedLayerIdx_ < layers.size()) {
                        layers.erase(layers.begin() + selectedLayerIdx_);

                        // Resize mask matrix
                        size_t n = layers.size();
                        maskMatrix.resize(n);
                        for (auto& row : maskMatrix) {
                            row.resize(n, false);
                        }

                        // Adjust selection
                        if (selectedLayerIdx_ >= layers.size() && !layers.empty()) {
                            selectedLayerIdx_ = static_cast<unsigned>(layers.size() - 1);
                        }
                    }
                }

                ImGui::SameLine();

                if (ImGui::Button("Edit Mask Matrix")) {
                    showEditMask_ = true;
                }

                ImGui::Separator();

                // Layer list
                ImGui::BeginChild("##LayerList", ImVec2(0, 200), true);

                for (unsigned i = 0; i < layers.size(); ++i) {
                    ImGui::PushID(i);

                    // Visibility checkbox
                    const std::string checkboxId = "##vis_" + std::to_string(i);
                    if (ImGui::Checkbox(checkboxId.c_str(), &layers[i].enabled)) {
                        // Layer visibility changed
                    }

                    ImGui::SameLine();

                    // Selectable layer item
                    const std::string label = "Layer " + std::to_string(layers[i].id) +
                        " (Mask: 0x" + std::to_string(layers[i].mask) + ")";

                    if (ImGui::Selectable(label.c_str(), selectedLayerIdx_ == i)) {
                        selectedLayerIdx_ = i;
                    }

                    // Context menu for layer
                    if (ImGui::BeginPopupContextItem()) {
                        if (ImGui::MenuItem("Delete Layer")) {
                            layers.erase(layers.begin() + i);
                            // Resize mask matrix
                            size_t n = layers.size();
                            maskMatrix.resize(n);
                            for (auto& row : maskMatrix) {
                                row.resize(n, false);
                            }
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break;
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::PopID();
                }

                ImGui::EndChild();

                // Selected layer properties
                if (selectedLayerIdx_ < layers.size()) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Selected Layer Properties");

                    auto& selectedLayer = layers[selectedLayerIdx_];

                    ImGui::Text("Index: %u", selectedLayerIdx_);
                    ImGui::InputInt("Layer ID", &selectedLayer.id);
                    ImGui::InputInt("Bit Mask", &selectedLayer.mask);
                    ImGui::Checkbox("Enabled", &selectedLayer.enabled);
                }

                ImGui::Unindent();
            }

            void ScenesPanel::drawGraphicsSettingsPanel() {

                //Get scene service
                auto scn_service = services->get<Scene::SceneManager>();

                //Get current skybox
                Assets::GUID curr_skybox = scn_service->getCurrSkyBoxTextureID();

                // Model Asset Selection
                if (DrawAssetSelectorField("Select A Skybox",
                    curr_skybox,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    services, false, { ".hdr" })) {

                    //Simply skybox settings
                    if (curr_skybox != scn_service->getCurrSkyBoxTextureID()) scn_service->setCurrSkyBoxTexture(curr_skybox);
                }

                //World Light intensity
                if (scn_service->getWorldLight() && ImGui::ColorEdit3("World Light Intensity", glm::value_ptr(scn_service->getWorldLight()->L_intensity))) {
                }

                //camera Light intensity
                if (scn_service->getCameraLight() && ImGui::ColorEdit3("Camera Light Intensity", glm::value_ptr(scn_service->getCameraLight()->L_intensity))) {
                }

                //Set using day
                bool using_day = scn_service->getUsingDayTime();
                if (ImGui::Checkbox("Using Day Time", &using_day)) {
                    scn_service->setUsingDayTime(using_day);
                }
            }

            void ScenesPanel::onAttach()
            {
                registerPopUp("CreateScene", createScenePopup("CreateScene"));
                registerPopUp("SaveSceneAs", saveSceneAsPopup("SaveSceneAs"));
                registerPopUp("DeleteScene", deleteScenePopup("DeleteScene"));
                registerPopUp("Info", defPopUp("Info"));
            }

            // ---------- Main draw ----------
            void ScenesPanel::onUpdate(AppTiming timing) {

                //Get services
                auto scn_service = services->get<Scene::SceneManager>();
                auto asset_service = services->get<Assets::Manager>();

                //Get scene manager name
                auto scn_id = scn_service->getCurrScnID();

                //Render current scene ID
                ImGui::Text("Scene ID: %s", !asset_service->checkAssetRegistered(scn_id) ? "(none)" : asset_service->getAssetData(scn_id)->name.c_str());

                // Create New Scene
                if (ImGui::Button("Create New Scene")) {
                    openPopUp("CreateScene");
                }

                // Show dropdown of available scenes
                if (!asset_service->getAllAssetDataOfType(Assets::Type::Scenes).empty()) {
                    // Model Asset Selection
                    if (DrawAssetSelectorField("Select A Scene",
                        selected,
                        PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Scenes),
                        services)) {

                        //Update with new name
                        selected_scn_name = asset_service->getAssetData(selected)->name;
                    }
                }
                else {
                    ImGui::TextDisabled("No scenes available!");
                }

                //Check if valid scene has been selected
                if (selected.IsValid() && !selected_scn_name.empty()) {
                    
                    //Load scene
                    if (scn_service->getCurrScnID() != selected) {
                        if (ImGui::Button("Load")) {
                            scn_service->loadScene(selected);
                        }

                        ImGui::SameLine();
                    }
                    //Save curr scene
                    else {
                        if (ImGui::Button("Save")) {
                            scn_service->saveActiveScene(selected);
                            openPopUp("Info", std::make_shared<std::string>("Scene Saved!"));
                        }
                        ImGui::SameLine();
                    }

                    //Delete scene option
                    if (ImGui::Button("Delete")) {
                        openPopUp("DeleteScene");
                    }

                    ImGui::SameLine();
                }

                //Save scene as
                if (ImGui::Button("Save As")) {
                    openPopUp("SaveSceneAs");
                }

                ImGui::Separator();

                //Render graphics settings
                if (ImGui::CollapsingHeader("Graphics Settings")) {
                    drawGraphicsSettingsPanel();
                }

                //Render Layer settings
                if (ImGui::CollapsingHeader("Layer Settings")) {
                    drawLayerManagementPanel();
                }

                //// Scene configuration panels
                //drawSkyboxSettingsPanel();
                //drawGraphicsSettingsPanel();
                //drawEnvironmentSettingsPanel();
                //drawCameraSettingsPanel();
                //drawLayerManagementPanel();

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

                //// Show the error popup when load scene fails
                //if (showSceneLoadError_) {
                //    ImGui::OpenPopup("Scene Load Error");
                //    showSceneLoadError_ = false;
                //}

                //// Render the modal if it's open
                //if (ImGui::BeginPopupModal("Scene Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
                //    ImGui::TextWrapped("%s", loadSceneErrorMsg_.c_str());
                //    ImGui::Spacing();
                //    if (ImGui::Button("OK", ImVec2(120, 0))) {
                //        ImGui::CloseCurrentPopup();
                //    }
                //    ImGui::EndPopup();
                //}

                renderPopUps();
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif