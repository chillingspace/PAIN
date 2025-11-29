#include "pch.h"
#include "ScenesPanel.h"

#ifdef _DEBUG
#include "CoreSystems/Serialization/sSerialization.h"
#include "ECS/Controller.h"
#include "CoreSystems/Scene/Scene.h"
#include <CoreSystems/Scripting/EngineAPIAdapter.h>

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
                selected_cam_index = -1;
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
                auto scnService = services->get<Scene::SceneManager>();
                if (!scnService) return;

                auto& layers = scnService->getLayers();
                auto& maskMatrix = scnService->getMaskMatrix();

                // At the START of drawLayerManagementPanel(), before the layer loop:
                std::unordered_map<int, int> layerEntityCounts;
                auto controller = services->get<ECS::Controller>();
                if (controller) {
                    auto& registry = controller->getRegistry();
                    auto view = registry.view<Entity::Layer>();

                    for (auto entity : view) {
                        const auto& entityLayer = view.get<Entity::Layer>(entity);
                        layerEntityCounts[entityLayer.layer_id]++;
                    }
                }

                // Statistics bar
                {
                    ImGui::BeginGroup();
                    ImGui::Text("Layers: %u/32", static_cast<unsigned>(layers.size()));
                    ImGui::SameLine(120);

                    int enabledCount = 0;
                    for (const auto& layer : layers) {
                        if (layer.enabled) enabledCount++;
                    }
                    ImGui::Text("Enabled: %d", enabledCount);
                    ImGui::SameLine(240);

                    if (selectedLayerIdx_ < layers.size()) {
                        ImGui::TextColored(
                            ImVec4(layers[selectedLayerIdx_].color.x,
                                layers[selectedLayerIdx_].color.y,
                                layers[selectedLayerIdx_].color.z, 1.0f),
                            "%s Selected", layers[selectedLayerIdx_].name.c_str());
                    }
                    ImGui::EndGroup();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Toolbar
                {
                    // Add Layer button
                    if (ImGui::Button("Add Layer", ImVec2(100, 0))) {
                        if (layers.size() < 32) {
                            int newId = 0;
                            for (const auto& layer : layers) {
                                if (layer.id >= newId) newId = layer.id + 1;
                            }

                            Scene::Layer newLayer;
                            newLayer.id = newId;
                            newLayer.mask = 1 << newId;
                            newLayer.enabled = true;
                            newLayer.name = "Layer " + std::to_string(newId);

                            // Assign a nice default color
                            static const glm::vec3 defaultColors[] = {
                                {0.9f, 0.3f, 0.3f},  // Red
                                {0.3f, 0.9f, 0.3f},  // Green
                                {0.3f, 0.3f, 0.9f},  // Blue
                                {0.9f, 0.9f, 0.3f},  // Yellow
                                {0.9f, 0.3f, 0.9f},  // Magenta
                                {0.3f, 0.9f, 0.9f},  // Cyan
                            };
                            newLayer.color = defaultColors[newId % 6];

                            layers.push_back(newLayer);

                            // Resize and initialize mask matrix
                            size_t n = layers.size();
                            maskMatrix.resize(n);
                            for (auto& row : maskMatrix) {
                                row.resize(n, false);
                            }

                            // Auto-select new layer
                            selectedLayerIdx_ = static_cast<unsigned>(layers.size() - 1);

                            PN_CORE_INFO("[LayerPanel] Created layer '{}'", newLayer.name);
                        }
                        else {
                            PN_CORE_WARN("[LayerPanel] Maximum 32 layers reached!");
                        }
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Add a new layer (Max 32)");
                    }

                    ImGui::SameLine();

                    // Remove button
                    ImGui::BeginDisabled(layers.empty() || selectedLayerIdx_ >= layers.size());
                    if (ImGui::Button("Remove", ImVec2(100, 0))) {
                        if (selectedLayerIdx_ < layers.size()) {
                            std::string removedName = layers[selectedLayerIdx_].name;
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

                            PN_CORE_INFO("[LayerPanel] Removed layer '{}'", removedName);
                        }
                    }
                    ImGui::EndDisabled();

                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        ImGui::SetTooltip("Remove selected layer");
                    }

                    ImGui::SameLine();

                    // Collision Matrix button
                    if (ImGui::Button("Collision Matrix", ImVec2(130, 0))) {
                        showEditMask_ = true;
                    }

                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Edit layer collision matrix");
                    }

                    ImGui::SameLine();

                    // Quick actions dropdown
                    if (ImGui::Button("Actions", ImVec2(100, 0))) {
                        ImGui::OpenPopup("LayerActions");
                    }

                    if (ImGui::BeginPopup("LayerActions")) {
                        if (ImGui::MenuItem("Enable All Layers")) {
                            for (auto& layer : layers) layer.enabled = true;
                        }
                        if (ImGui::MenuItem("Disable All Layers")) {
                            for (auto& layer : layers) layer.enabled = false;
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Reset Layer Colors")) {
                            static const glm::vec3 colors[] = {
                                {0.9f, 0.3f, 0.3f}, {0.3f, 0.9f, 0.3f}, {0.3f, 0.3f, 0.9f},
                                {0.9f, 0.9f, 0.3f}, {0.9f, 0.3f, 0.9f}, {0.3f, 0.9f, 0.9f},
                            };
                            for (size_t i = 0; i < layers.size(); ++i) {
                                layers[i].color = colors[i % 6];
                            }
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Enable All Collisions")) {
                            for (size_t i = 0; i < maskMatrix.size(); ++i) {
                                for (size_t j = 0; j < maskMatrix[i].size(); ++j) {
                                    if (i != j) maskMatrix[i][j] = true;
                                }
                            }
                        }
                        if (ImGui::MenuItem("Disable All Collisions")) {
                            for (auto& row : maskMatrix) {
                                std::fill(row.begin(), row.end(), false);
                            }
                        }
                        ImGui::EndPopup();
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Layer list with cards
                ImGui::BeginChild("##LayerCardList", ImVec2(0, 350), true, ImGuiWindowFlags_HorizontalScrollbar);

                if (layers.empty()) {
                    // Empty state
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 100);
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    float textWidth = ImGui::CalcTextSize("No layers created yet").x;
                    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - textWidth) * 0.5f);
                    ImGui::Text("No layers created yet");
                    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Click 'Add Layer' to create one").x) * 0.5f);
                    ImGui::Text("Click 'Add Layer' to create one");
                    ImGui::PopStyleColor();
                }

                for (unsigned i = 0; i < layers.size(); ++i) {
                    ImGui::PushID(i);

                    auto& layer = layers[i];
                    bool isSelected = (selectedLayerIdx_ == i);

                    // Card background
                    ImVec2 cardPos = ImGui::GetCursorScreenPos();
                    ImVec2 cardSize = ImVec2(ImGui::GetContentRegionAvail().x, 80);

                    ImU32 cardColor = isSelected
                        ? IM_COL32(60, 80, 100, 255)   // Selected
                        : IM_COL32(40, 45, 50, 255);    // Normal

                    ImGui::GetWindowDrawList()->AddRectFilled(
                        cardPos,
                        ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y),
                        cardColor,
                        4.0f  // Rounded corners
                    );

                    // Color indicator bar on left
                    ImU32 colorIndicator = IM_COL32(
                        static_cast<int>(layer.color.x * 255),
                        static_cast<int>(layer.color.y * 255),
                        static_cast<int>(layer.color.z * 255),
                        255
                    );
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        cardPos,
                        ImVec2(cardPos.x + 6, cardPos.y + cardSize.y),
                        colorIndicator,
                        4.0f, ImDrawFlags_RoundCornersLeft
                    );

                    // Card content
                    ImGui::SetCursorScreenPos(ImVec2(cardPos.x + 12, cardPos.y + 8));

                    ImGui::BeginGroup();
                    {
                        // Row 1: Visibility + Name + Actions
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);

                        // Visibility toggle with nice icon
                        bool wasEnabled = layer.enabled;
                        if (ImGui::Checkbox(layer.enabled ? "Visible" : "Hidden", &layer.enabled)) {
                            if (wasEnabled != layer.enabled) {
                                PN_CORE_INFO("[LayerPanel] Layer '{}' {}",
                                    layer.name,
                                    layer.enabled ? "enabled" : "disabled");
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip(layer.enabled ? "Layer Visible" : "Layer Hidden");
                        }

                        ImGui::SameLine();

                        // Layer name (editable)
                        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.25f, 0.3f, 0.8f));
                        ImGui::SetNextItemWidth(250);
                        char nameBuf[64];
                        strncpy_s(nameBuf, layer.name.c_str(), sizeof(nameBuf) - 1);
                        if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf))) {
                            layer.name = nameBuf;
                        }
                        ImGui::PopStyleColor();

                        ImGui::SameLine();

                        // Color picker (compact)
                        ImGui::SetNextItemWidth(80);
                        if (ImGui::ColorEdit3("##Color", &layer.color.x,
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                            // Color changed
                        }

                        ImGui::SameLine();

                        // Quick duplicate button
                        if (ImGui::SmallButton("Duplicate")) {
                            if (layers.size() < 32) {
                                Scene::Layer duplicate = layer;
                                duplicate.id = static_cast<int>(layers.size());
                                duplicate.mask = 1 << duplicate.id;
                                duplicate.name = layer.name + " Copy";
                                layers.push_back(duplicate);

                                // Resize mask matrix
                                size_t n = layers.size();
                                maskMatrix.resize(n);
                                for (auto& row : maskMatrix) {
                                    row.resize(n, false);
                                }
                            }
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Duplicate layer");
                        }

                        // Row 2: Layer info
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                        ImGui::Text("ID");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(65);
                        if (ImGui::InputInt("##LayerID", &layer.id, 1, 100, ImGuiInputTextFlags_ElideLeft)) {
                            // Validate ID range
                            if (layer.id < 0) layer.id = 0;
                            if (layer.id > 31) layer.id = 31;
                            layer.mask = 1 << layer.id;
                        }
                        ImGui::SameLine(125);
                        ImGui::Text("Mask: 0x%08X", layer.mask);
                        ImGui::SameLine(250);

                        // Show collision count
                        int collisionCount = 0;
                        if (i < maskMatrix.size()) {
                            for (bool canCollide : maskMatrix[i]) {
                                if (canCollide) collisionCount++;
                            }
                        }
                        ImGui::Text("Collides with: %d layers", collisionCount);
                        ImGui::PopStyleColor();

                        // Row 3: Entity count (if entities have layer component)
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);

                        int entityCount = 0;
                        auto it = layerEntityCounts.find(layer.id);
                        if (it != layerEntityCounts.end()) {
                            entityCount = it->second;
                        }

                        // Color code based on count
                        ImVec4 countColor = entityCount > 0
                            ? ImVec4(0.7f, 0.9f, 0.7f, 1.0f)  // Green if has entities
                            : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);  // Gray if empty

                        ImGui::PushStyleColor(ImGuiCol_Text, countColor);
                        ImGui::Text("Entities: %d", entityCount);
                        ImGui::PopStyleColor();
                    }
                    ImGui::EndGroup();

                    // Card content
                    ImGui::SetCursorScreenPos(ImVec2(cardPos.x, cardPos.y));

                    // Make card clickable
                    ImGui::InvisibleButton("##CardButton", cardSize);
                    if (ImGui::IsItemClicked()) {
                        selectedLayerIdx_ = i;
                    }

                    // Context menu
                    if (ImGui::BeginPopupContextItem(("LayerContext" + std::to_string(i)).c_str())) {
                        ImGui::TextColored(ImVec4(layer.color.x, layer.color.y, layer.color.z, 1.0f),
                            "Layer: %s", layer.name.c_str());
                        ImGui::Separator();

                        if (ImGui::MenuItem("Duplicate")) {
                            if (layers.size() < 32) {
                                Scene::Layer duplicate = layer;
                                duplicate.id = static_cast<int>(layers.size());
                                duplicate.mask = 1 << duplicate.id;
                                duplicate.name = layer.name + " Copy";
                                layers.push_back(duplicate);

                                size_t n = layers.size();
                                maskMatrix.resize(n);
                                for (auto& row : maskMatrix) {
                                    row.resize(n, false);
                                }
                            }
                        }

                        if (ImGui::MenuItem(layer.enabled ? "Disable" : "Enable")) {
                            layer.enabled = !layer.enabled;
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("Move Up", nullptr, false, i > 0)) {
                            std::swap(layers[i], layers[i - 1]);
                            selectedLayerIdx_ = i - 1;
                        }

                        if (ImGui::MenuItem("Move Down", nullptr, false, i < layers.size() - 1)) {
                            std::swap(layers[i], layers[i + 1]);
                            selectedLayerIdx_ = i + 1;
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("Delete", "Del")) {
                            layers.erase(layers.begin() + i);
                            size_t n = layers.size();
                            maskMatrix.resize(n);
                            for (auto& row : maskMatrix) {
                                row.resize(n, false);
                            }
                            if (selectedLayerIdx_ >= layers.size() && !layers.empty()) {
                                selectedLayerIdx_ = static_cast<unsigned>(layers.size() - 1);
                            }
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break;
                        }

                        ImGui::EndPopup();
                    }

                    ImGui::Dummy(ImVec2(0, 3)); // Space for next card
                    ImGui::PopID();
                }

                ImGui::EndChild();
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

                //Get graphics settings
                auto& gs = GraphicsSettings::get();

                //Set using world light
                bool using_wlight = gs.world_light;
                if (ImGui::Checkbox("Using World Light", &using_wlight)) {
                    gs.world_light = using_wlight;
                }

                //Set using IBL
                bool using_ibl = gs.ibl;
                if (ImGui::Checkbox("Using IBL", &using_ibl)) {
                    gs.ibl = using_ibl;
                }

                //Set using diffuse map
                bool using_diffuse = gs.DEBUG_USE_DIFFUSE_MAP;
                if (ImGui::Checkbox("Using Diffuse Map", &using_diffuse)) {
                    gs.DEBUG_USE_DIFFUSE_MAP = using_diffuse;
                }

                //Set using ao map
                bool using_ao = gs.DEBUG_USE_AO_MAP;
                if (ImGui::Checkbox("Using AO Map", &using_ao)) {
                    gs.DEBUG_USE_AO_MAP = using_ao;
                }

                //Set using normal map
                bool using_normal = gs.DEBUG_USE_NORMAL_MAP;
                if (ImGui::Checkbox("Using Normal Map", &using_normal)) {
                    gs.DEBUG_USE_NORMAL_MAP = using_normal;
                }

                //Set using rm map
                bool using_rm = gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP;
                if (ImGui::Checkbox("Using Roughness Metallic Map", &using_rm)) {
                    gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP = using_rm;
                }

                //Set using rm map
                bool using_emissive = gs.DEBUG_USE_EMISSION_MAP;
                if (ImGui::Checkbox("Using Emissive Map", &using_emissive)) {
                    gs.DEBUG_USE_EMISSION_MAP = using_emissive;
                }

                //PBR Map Index
                static int selected_pbr_index = 0;

                //Find current active pbr
                selected_pbr_index = gs.DEBUG_PBR_MAP_TYPE;

                //Dropdown to select PBR Map
                if (ImGui::Combo("PBR Map Types", &selected_pbr_index, gs.DEBUG_PBR_MAP_STRING.data(), gs.DEBUG_PBR_MAP_STRING.size())) {
                    // When selection changes, update path text box
                    if (selected_pbr_index >= 0 && selected_pbr_index < gs.DEBUG_PBR_MAP_STRING.size()) {
                        gs.DEBUG_PBR_MAP_TYPE = static_cast<GraphicsSettings::DEBUG_PBR_MAP_TYPES>(selected_pbr_index);
                    }
                }
            }

            void ScenesPanel::drawActiveCamPanel()
            {
                auto scene = services->get<Scene::SceneManager>();
                if (scene) {

                    const auto& cameras = scene->GetAllGameCamera(); // Get by const reference!
                    std::string active_game_cam = scene->GetActiveGameCamera();

                    int current_active_index = -1;
                    int loop_index = 0;

                    cached_camera_names_ptr.clear();
                    for (const auto& [name, camPtr] : cameras) {
                        cached_camera_names_ptr.push_back(name.c_str());

                        // SYNC LOGIC: Check if this is our active camera
                        if (name == active_game_cam) {
                            current_active_index = loop_index;
                        }
                        loop_index++;
                    }

                    if (current_active_index != -1) {
                        selected_cam_index = current_active_index;
                    }


                    // Combo box for active cam selection
                    if (ImGui::Combo("Select Active Camera", &selected_cam_index, cached_camera_names_ptr.data(), cached_camera_names_ptr.size())) {
                        
                        // Input sanitization
                        if (selected_cam_index >= 0 && selected_cam_index < cached_camera_names_ptr.size()) {

                            const char* selectedName = cached_camera_names_ptr[selected_cam_index];

                            auto it = cameras.find(selectedName);
                            if (it != cameras.end()) {
  
                                scene->ChangeGameCamera(it->first);
                            }
                        }
                    }
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
                ImGui::Spacing();

                //Render graphics settings
                if (ImGui::CollapsingHeader("Graphics Settings")) {
                    drawGraphicsSettingsPanel();
                }

                //Render Layer settings
                if (ImGui::CollapsingHeader("Layer Settings")) {
                    drawLayerManagementPanel();
                }

                //Render Active Cam
                ImGui::Spacing();
                drawActiveCamPanel();

              
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