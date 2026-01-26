#include "pch.h"
#include "ScenesPanel.h"

#ifdef _DEBUG
#include "CoreSystems/Serialization/sSerialization.h"
#include "ECS/Controller.h"
#include "CoreSystems/Scene/Scene.h"
#include <CoreSystems/Scripting/EngineAPIAdapter.h>

#include "LayeredSystems/LevelEditor/Panels/ReflectionUI.h"
#include "CoreSystems/Windows/Window.h"


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

#ifdef PN_PLATFORM_WINDOWS
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
#endif

#ifdef PN_PLATFORM_WINDOWS
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
#endif
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
                        strncpy(nameBuf, layer.name.c_str(), sizeof(nameBuf) - 1);
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
                    ImGui::Spacing();

                    auto cam = scene->GetActiveCamera();
                    float curr_speed = cam->speed;
                    float curr_sens = cam->sensitivity;

                    if (ImGui::DragFloat("Camera Speed", &curr_speed, 0.1f, 0.1f, 100.0f)) {

                        cam->speed = curr_speed;
                    }
                    ImGui::Spacing();

                    if (ImGui::DragFloat("Camera Senstivity", &curr_sens, 0.1f, 0.1f, 100.0f)) {

                        cam->sensitivity = curr_sens;
                    }
                }

            }

            void ScenesPanel::drawLoadingScreenPanel() {
                // Get scene service
                auto scn_service = services->get<Scene::SceneManager>();
                if (!scn_service || !scn_service->loadingScreen) {
                    ImGui::TextDisabled("Loading screen not available");
                    return;
                }
                
                auto& loadingScreen = scn_service->loadingScreen;
                
                ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Loading Screen Configuration");
                ImGui::Separator();
                ImGui::Spacing();
                
                // ============================================================
                // Progress Bar Settings
                // ============================================================
                if (ImGui::CollapsingHeader("Progress Bar", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();
                    
                    // Position
                    glm::vec2 barPos = loadingScreen->getProgressBarPosition();
                    float barPosArray[2] = { barPos.x, barPos.y };
                    
                    ImGui::Text("Position (Screen Space)");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::DragFloat2("##BarPos", barPosArray, 1.0f, 0.0f, 2000.0f, "%.0f px")) {
                        loadingScreen->setProgressBarPosition(barPosArray[0], barPosArray[1]);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Position in screen space (X, Y) from top-left corner");
                    }
                    
                    ImGui::Spacing();
                    
                    // Size
                    glm::vec2 barSize = loadingScreen->getProgressBarSize();
                    float barSizeArray[2] = { barSize.x, barSize.y };
                    
                    ImGui::Text("Size");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::DragFloat2("##BarSize", barSizeArray, 1.0f, 50.0f, 2000.0f, "%.0f px")) {
                        loadingScreen->setProgressBarSize(barSizeArray[0], barSizeArray[1]);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Progress bar dimensions (Width, Height) in pixels");
                    }

                    ImGui::Spacing();

                    // Progress Bar enable/disable toggle
                    bool showProgress = loadingScreen->getShowProgressBar();
                    if (ImGui::Checkbox("Progress Bar Shown", &showProgress)) loadingScreen->setShowProgressBar(showProgress);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Show progress bar on loading screen");
                    }
                    
                    ImGui::Unindent();
                }
                
                ImGui::Spacing();
                
                // ============================================================
                // Status Text Settings
                // ============================================================
                if (ImGui::CollapsingHeader("Status Text", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();
                    
                    // Position
                    glm::vec2 textPos = loadingScreen->getStatusTextPosition();
                    float textPosArray[2] = { textPos.x, textPos.y };
                    
                    ImGui::Text("Position (Screen Space)");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::DragFloat2("##TextPos", textPosArray, 1.0f, 0.0f, 2000.0f, "%.0f px")) {
                        loadingScreen->setStatusTextPosition(textPosArray[0], textPosArray[1]);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Text position (X, Y) from top-left corner");
                    }
                    
                    ImGui::Spacing();
                    
                    // Scale
                    float textScale = loadingScreen->getStatusTextScale();
                    ImGui::Text("Font Scale");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::SliderFloat("##TextScale", &textScale, 0.01f, 0.1f, "%.2f")) {
                        loadingScreen->setStatusTextScale(textScale);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Multiplier for text size (1.0 = default)");
                    }

                    ImGui::Spacing();

                    // Status text enable/disable toggle
                    bool showStatus = loadingScreen->getShowStatusText();
                    if (ImGui::Checkbox("Status Text Shown", &showStatus)) loadingScreen->setShowStatusText(showStatus);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Show Status Text on loading screen");
                    }
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    // Quick presets
                    ImGui::Text("Quick Presets:");
                    
                    auto win = services->get<Window::Window>();
                    if (win) {
                        auto framebuffer = win->getFrameBuffer();
                        float screenWidth = framebuffer.x;
                        float screenHeight = framebuffer.y;
                        glm::vec2 barPos = loadingScreen->getProgressBarPosition();
                        
                        if (ImGui::Button("Below Progress Bar")) {
                            loadingScreen->setStatusTextPosition(screenWidth / 2.0f, barPos.y - 70.0f);
                        }
                        ImGui::SameLine();
                        
                        if (ImGui::Button("Above Progress Bar")) {
                            loadingScreen->setStatusTextPosition(screenWidth / 2.0f, barPos.y + 70.0f);
                        }
                    }
                    
                    ImGui::Unindent();
                }
                
                ImGui::Spacing();
                
                // ============================================================
                // Style & Color Settings  
                // ============================================================
                if (ImGui::CollapsingHeader("Style & Colors")) {
                    ImGui::Indent();

                    auto bgColor = loadingScreen->getBackgroundColor();
                    float bgfillColorArray[3] = { bgColor.r, bgColor.g, bgColor.b };

                    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Background Appearance");
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Fill Color
                    ImGui::Text("Background Fill Color:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::ColorEdit3("##BGFillColor", bgfillColorArray, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                        loadingScreen->setBackgroundColor(glm::vec3(bgfillColorArray[0], bgfillColorArray[1], bgfillColorArray[2]));
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Color of the background");
                    }

                    ImGui::Spacing();
                    
                    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Progress Bar Appearance");
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    // Get current style
                    auto [fillColor, glowColor, glowIntensity] = loadingScreen->getProgressBarStyle();
                    float fillColorArray[3] = { fillColor.r, fillColor.g, fillColor.b };
                    float glowColorArray[3] = { glowColor.r, glowColor.g, glowColor.b };
                    float intensity = glowIntensity;
                    
                    // Fill Color
                    ImGui::Text("Fill Color:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::ColorEdit3("##FillColor", fillColorArray, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                        loadingScreen->setProgressBarFillColor(glm::vec3(fillColorArray[0], fillColorArray[1], fillColorArray[2]));
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Color of the filled progress portion");
                    }
                    
                    ImGui::Spacing();
                    
                    // Glow Color
                    ImGui::Text("Glow Color:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::ColorEdit3("##GlowColor", glowColorArray, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                        loadingScreen->setProgressBarGlowColor(glm::vec3(glowColorArray[0], glowColorArray[1], glowColorArray[2]));
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Color of the animated glow effect");
                    }
                    
                    ImGui::Spacing();
                    
                    // Glow Intensity
                    ImGui::Text("Glow Intensity:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::SliderFloat("##GlowIntensity", &intensity, 0.0f, 2.0f, "%.2f")) {
                        loadingScreen->setProgressBarGlowIntensity(intensity);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Intensity of the glow effect (0 = off, 2 = maximum)");
                    }
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    // Quick color presets
                    ImGui::Text("Quick Presets:");
                    
                    if (ImGui::Button("Cyan/Blue (Default)")) {
                        loadingScreen->setProgressBarFillColor(glm::vec3(0.2f, 0.8f, 0.9f));
                        loadingScreen->setProgressBarGlowColor(glm::vec3(0.3f, 0.6f, 1.0f));
                        loadingScreen->setProgressBarGlowIntensity(0.8f);
                    }
                    ImGui::SameLine();
                    
                    if (ImGui::Button("Green/Yellow")) {
                        loadingScreen->setProgressBarFillColor(glm::vec3(0.3f, 0.9f, 0.3f));
                        loadingScreen->setProgressBarGlowColor(glm::vec3(1.0f, 1.0f, 0.3f));
                        loadingScreen->setProgressBarGlowIntensity(0.6f);
                    }
                    ImGui::SameLine();
                    
                    if (ImGui::Button("Purple/Pink")) {
                        loadingScreen->setProgressBarFillColor(glm::vec3(0.8f, 0.3f, 0.9f));
                        loadingScreen->setProgressBarGlowColor(glm::vec3(1.0f, 0.4f, 0.8f));
                        loadingScreen->setProgressBarGlowIntensity(1.0f);
                    }
                    
                    ImGui::Unindent();
                }
                
                ImGui::Spacing();
                
                // ============================================================
                // Background Settings
                // ============================================================
                if (ImGui::CollapsingHeader("Background")) {
                    ImGui::Indent();
                    
                    ImGui::TextWrapped("Set a background texture and optionally configure spritesheet animation.");
                    ImGui::Spacing();
                    
                    // Background texture selector
                    Assets::GUID currentBg = scn_service->loadingScreen->getBackgroundTexture();
                    if (DrawAssetSelectorField("Background Texture",
                        currentBg,
                        PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                        services, false)) {
                        
                        if (currentBg.IsValid()) {
                            loadingScreen->setBackgroundTexture(currentBg);
                        }
                    }

                    ImGui::Spacing();

                    // Scale
                    float bgScale = loadingScreen->getBGScale();
                    ImGui::Text("Background Texture Scale");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::SliderFloat("##BGScale", &bgScale, 0.1f, 10.0f, "%.2f")) {
                        loadingScreen->setBGScale(bgScale);
                    }
                    
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    
                    // Spritesheet Animation
                    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Spritesheet Animation");
                    ImGui::TextWrapped("Configure spritesheet-based animation for the background texture.");
                    ImGui::Spacing();
                    
                    // Get current settings
                    auto [frameCount, framesPerRow, frameTime, enabled] = loadingScreen->getSpritesheetSettings();
                    
                    // Frame count
                    int tempFrameCount = frameCount;
                    ImGui::Text("Frame Count:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::InputInt("##FrameCount", &tempFrameCount, 1, 10)) {
                        if (tempFrameCount >= 1 && tempFrameCount <= 1000) {
                            loadingScreen->setSpritesheetAnimation(tempFrameCount, framesPerRow, frameTime);
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Total number of frames in the spritesheet");
                    }
                    
                    // Frames per row
                    int tempFramesPerRow = framesPerRow;
                    ImGui::Text("Frames Per Row:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::InputInt("##FramesPerRow", &tempFramesPerRow, 1, 10)) {
                        if (tempFramesPerRow >= 1 && tempFramesPerRow <= 100) {
                            loadingScreen->setSpritesheetAnimation(frameCount, tempFramesPerRow, frameTime);
                        }
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Number of frames per row in the spritesheet layout");
                    }
                    
                    // Frame time
                    float tempFrameTime = frameTime;
                    ImGui::Text("Frame Duration:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::SliderFloat("##FrameTime", &tempFrameTime, 0.01f, 1.0f, "%.2f sec")) {
                        loadingScreen->setSpritesheetAnimation(frameCount, framesPerRow, tempFrameTime);
                    }
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Time to display each frame");
                    }
                    
                    // Enable/disable toggle
                    bool tempEnabled = enabled;
                    if (ImGui::Checkbox("Enable Animation", &tempEnabled)) {
                        loadingScreen->setAnimationEnabled(tempEnabled);
                    }

                    ImGui::Spacing();

                    // Background enable/disable toggle
                    bool showBG = loadingScreen->getShowBG();
                    if (ImGui::Checkbox("Texture Background Shown", &showBG)) loadingScreen->setShowBG(showBG);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Show Texture As Background on loading screen");
                    }

                    ImGui::Spacing();

                    // Overlay enable/disable toggle
                    bool showOverlay = loadingScreen->getShowOverlay();
                    if (ImGui::Checkbox("Overlay Shown", &showOverlay)) loadingScreen->setShowOverlay(showOverlay);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Show Overlay on loading screen");
                    }
                    
                    ImGui::Unindent();
                }
                
                ImGui::Spacing();
                
                // ============================================================
                // Live Preview Section
                // ============================================================
                if (ImGui::CollapsingHeader("Live Preview")) {
                    ImGui::Indent();
                    
                    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "Loading Screen Preview");
                    ImGui::TextWrapped("Preview the loading screen in real-time without reloading the scene.");
                    
                    ImGui::Spacing();
                    
                    // Preview controls
                    static float previewProgress = 0.5f;
                    static char previewStatus[256] = "Loading assets...";
                    static bool autoAnimate = false;
                    
                    ImGui::Text("Progress:");
                    ImGui::SetNextItemWidth(200);
                    ImGui::SliderFloat("##PreviewProgress", &previewProgress, 0.0f, 1.0f, "%.2f");
                    
                    ImGui::Spacing();
                    
                    ImGui::Text("Status Text:");
                    ImGui::SetNextItemWidth(300);
                    ImGui::InputText("##PreviewStatus", previewStatus, IM_ARRAYSIZE(previewStatus));
                    
                    ImGui::Spacing();
                    
                    ImGui::Checkbox("Auto-animate progress", &autoAnimate);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Automatically cycle progress from 0% to 100%");
                    }
                    
                    ImGui::Spacing();
                    
                    // Preview enable/disable toggle
                    static bool enablePreview = false;
                    ImGui::Checkbox("Enable Live Preview", &enablePreview);
                    if (ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Toggle real-time preview rendering (may impact performance)");
                    }
                    
                    // Auto-animate logic
                    if (autoAnimate) {
                        previewProgress += 0.01f * ImGui::GetIO().DeltaTime;
                        if (previewProgress > 1.0f) previewProgress = 0.0f;
                    }

                    // Only render preview if enabled
                    if (enablePreview) {
                        loadingScreen->renderPreview(previewProgress, std::string(previewStatus));
                    }
                    
                    ImGui::Unindent();
                }
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Default Configuration##LoadingScreen")) loadingScreen->defaultSetup();

                ImGui::Spacing();
                
                // Info footer
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("All settings are applied in real-time. Changes will be visible the next time the loading screen is displayed.");
                ImGui::PopStyleColor();
            }

            void ScenesPanel::onAttach()
            {
#ifdef PN_PLATFORM_WINDOWS
                registerPopUp("CreateScene", createScenePopup("CreateScene"));
                registerPopUp("SaveSceneAs", saveSceneAsPopup("SaveSceneAs"));
                registerPopUp("DeleteScene", deleteScenePopup("DeleteScene"));
#endif
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

#ifdef PN_PLATFORM_WINDOWS
                // Create New Scene
                if (ImGui::Button("Create New Scene")) {
                    openPopUp("CreateScene");
                }
#endif

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
#ifdef PN_PLATFORM_WINDOWS
                        if (ImGui::Button("Save")) {
                            if (scn_service->isPlaying()) {
                                openPopUp("Info", std::make_shared<std::string>("Cannot save scene while Game is Playing! Please Stop first."));
                            }
                            else {
                                scn_service->saveActiveScene(selected);
                                openPopUp("Info", std::make_shared<std::string>("Scene Saved!"));
                            }

                        }
                        ImGui::SameLine();
#endif
                    }

#ifdef PN_PLATFORM_WINDOWS
                    //Delete scene option
                    if (ImGui::Button("Delete")) {
                        openPopUp("DeleteScene");
                    }

                    ImGui::SameLine();
#endif
                }

#ifdef PN_PLATFORM_WINDOWS
                //Save scene as
                if (ImGui::Button("Save As")) {
                    openPopUp("SaveSceneAs");
                }
#endif

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
                if (ImGui::CollapsingHeader("Camera Settings")) {
                    drawActiveCamPanel();
                }

                //Render loading screen
                if (ImGui::CollapsingHeader("Loading Screen")) {
                    drawLoadingScreenPanel();
                }

                // Modals last
#ifdef PN_PLATFORM_WINDOWS
                drawCreateModal();
                drawDeleteModal();
                drawSaveAsModal();
#endif
                drawEditMaskModal();

                renderPopUps();
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif