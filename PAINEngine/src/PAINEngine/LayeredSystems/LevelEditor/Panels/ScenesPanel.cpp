#include "pch.h"
#include "ScenesPanel.h"

#ifdef _DEBUG
#include "CoreSystems/Serialization/sSerialization.h"
#include "ECS/Controller.h"
#include "CoreSystems/Scene/Scene.h"
#include <CoreSystems/Scripting/EngineAPIAdapter.h>

#include "LayeredSystems/LevelEditor/Panels/ReflectionUI.h"
#include "CoreSystems/Windows/Window.h"

#include "Systems/Physics/sysPhysics.h"


namespace PAIN {
    namespace Editor {
        namespace Panel {

            static inline std::string makeSceneId(const std::string& base) {
                return base.empty() ? std::string{} : (base + ".scn");
            }

            static inline std::string stripExt(const std::string& s) {
                const auto p = s.rfind('.');
                return (p == std::string::npos) ? s : s.substr(0, p);
            }

            ScenesPanel::ScenesPanel() {
                name = "Scene Manager";
                flags = ImGuiWindowFlags_None;
                selected_cam_index = -1;
            }

            void ScenesPanel::nextWindowSettings() {
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
                    tmpNameBuf_ = buf;

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
                    tmpNameBuf_ = buf;

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

            // ----------------------------------------------------------------
            // Edit Mask Modal
            // ----------------------------------------------------------------
            void ScenesPanel::drawEditMaskModal() {
                if (!showEditMask_) return;

                auto scn_service = services->get<Scene::SceneManager>();
                auto& layers = scn_service->getLayers();
                auto& maskMatrix = scn_service->getMaskMatrix();

                const unsigned n = static_cast<unsigned>(layers.size());
                if (maskMatrix.size() != n) {
                    maskMatrix.resize(n);
                    for (auto& row : maskMatrix) row.resize(n, false);
                }

                ImGui::OpenPopup("Edit Layer Collision Matrix");
                if (!ImGui::BeginPopupModal("Edit Layer Collision Matrix", &showEditMask_,
                    ImGuiWindowFlags_AlwaysAutoResize)) {
                    return;
                }

                ImGui::TextWrapped("This matrix defines which layers can interact with each other.");
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Checked = layers can collide/interact");
                ImGui::Separator();
                ImGui::Spacing();

                if (n > 0 && ImGui::BeginTable("##CollisionMatrix", n + 1,
                    ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg)) {

                    ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted("Layer \\ Layer");

                    for (unsigned col = 0; col < n; ++col) {
                        ImGui::TableNextColumn();
                        ImGui::Text("L%d", layers[col].id);
                    }

                    for (unsigned i = 0; i < n; ++i) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        ImGui::Text("L%d", layers[i].id);

                        for (unsigned j = 0; j < n; ++j) {
                            ImGui::TableNextColumn();
                            if (i == j) { ImGui::TextDisabled("X"); continue; }

                            bool canInteract = maskMatrix[i][j];
                            const std::string id = "##mask_" + std::to_string(i) + "_" + std::to_string(j);
                            if (ImGui::Checkbox(id.c_str(), &canInteract)) {
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

                if (ImGui::Button("Enable All")) {
                    for (unsigned i = 0; i < n; ++i)
                        for (unsigned j = 0; j < n; ++j)
                            if (i != j) maskMatrix[i][j] = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Disable All")) {
                    for (auto& row : maskMatrix) std::fill(row.begin(), row.end(), false);
                }
                ImGui::SameLine();
                if (ImGui::Button("Done")) {
                    showEditMask_ = false;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // ----------------------------------------------------------------
            // Layer Management Panel
            // ----------------------------------------------------------------
            void ScenesPanel::drawLayerManagementPanel() {
                auto scnService = services->get<Scene::SceneManager>();
                if (!scnService) return;

                auto& layers = scnService->getLayers();
                auto& maskMatrix = scnService->getMaskMatrix();

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
                    for (const auto& layer : layers) { if (layer.enabled) enabledCount++; }
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
                    if (ImGui::Button("Add Layer", ImVec2(100, 0))) {
                        if (layers.size() < 32) {
                            int newId = 0;
                            for (const auto& layer : layers)
                                if (layer.id >= newId) newId = layer.id + 1;

                            Scene::Layer newLayer;
                            newLayer.id = newId;
                            newLayer.mask = 1 << newId;
                            newLayer.enabled = true;
                            newLayer.name = "Layer " + std::to_string(newId);
                            newLayer.pickable = true;

                            static const glm::vec3 defaultColors[] = {
                                {0.9f,0.3f,0.3f},{0.3f,0.9f,0.3f},{0.3f,0.3f,0.9f},
                                {0.9f,0.9f,0.3f},{0.9f,0.3f,0.9f},{0.3f,0.9f,0.9f},
                            };
                            newLayer.color = defaultColors[newId % 6];
                            layers.push_back(newLayer);

                            size_t n = layers.size();
                            maskMatrix.resize(n);
                            for (auto& row : maskMatrix) row.resize(n, false);

                            selectedLayerIdx_ = static_cast<unsigned>(layers.size() - 1);
                            PN_CORE_INFO("[LayerPanel] Created layer '{}'", newLayer.name);
                        }
                        else {
                            PN_CORE_WARN("[LayerPanel] Maximum 32 layers reached!");
                        }
                    }
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add a new layer (Max 32)");
                    ImGui::SameLine();

                    ImGui::BeginDisabled(layers.empty() || selectedLayerIdx_ >= layers.size());
                    if (ImGui::Button("Remove", ImVec2(100, 0))) {
                        if (selectedLayerIdx_ < layers.size()) {
                            std::string removedName = layers[selectedLayerIdx_].name;
                            layers.erase(layers.begin() + selectedLayerIdx_);

                            size_t n = layers.size();
                            maskMatrix.resize(n);
                            for (auto& row : maskMatrix) row.resize(n, false);

                            if (selectedLayerIdx_ >= layers.size() && !layers.empty())
                                selectedLayerIdx_ = static_cast<unsigned>(layers.size() - 1);

                            PN_CORE_INFO("[LayerPanel] Removed layer '{}'", removedName);
                        }
                    }
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("Remove selected layer");
                    ImGui::SameLine();

                    if (ImGui::Button("Collision Matrix", ImVec2(130, 0))) showEditMask_ = true;
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Edit layer collision matrix");
                    ImGui::SameLine();

                    if (ImGui::Button("Actions", ImVec2(100, 0))) ImGui::OpenPopup("LayerActions");
                    if (ImGui::BeginPopup("LayerActions")) {
                        if (ImGui::MenuItem("Enable All Layers"))  for (auto& l : layers) l.enabled = true;
                        if (ImGui::MenuItem("Disable All Layers")) for (auto& l : layers) l.enabled = false;
                        ImGui::Separator();
                        if (ImGui::MenuItem("Reset Layer Colors")) {
                            static const glm::vec3 colors[] = {
                                {0.9f,0.3f,0.3f},{0.3f,0.9f,0.3f},{0.3f,0.3f,0.9f},
                                {0.9f,0.9f,0.3f},{0.9f,0.3f,0.9f},{0.3f,0.9f,0.9f},
                            };
                            for (size_t i = 0; i < layers.size(); ++i) layers[i].color = colors[i % 6];
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem("Enable All Collisions")) {
                            for (size_t i = 0; i < maskMatrix.size(); ++i)
                                for (size_t j = 0; j < maskMatrix[i].size(); ++j)
                                    if (i != j) maskMatrix[i][j] = true;
                        }
                        if (ImGui::MenuItem("Disable All Collisions")) {
                            for (auto& row : maskMatrix) std::fill(row.begin(), row.end(), false);
                        }
                        ImGui::EndPopup();
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::BeginChild("##LayerCardList", ImVec2(0, 350), true, ImGuiWindowFlags_HorizontalScrollbar);

                if (layers.empty()) {
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

                    ImVec2 cardPos = ImGui::GetCursorScreenPos();
                    ImVec2 cardSize = ImVec2(ImGui::GetContentRegionAvail().x, 100);

                    ImU32 cardColor = isSelected ? IM_COL32(60, 80, 100, 255) : IM_COL32(40, 45, 50, 255);
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        cardPos, ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), cardColor, 4.0f);

                    ImU32 colorIndicator = IM_COL32(
                        static_cast<int>(layer.color.x * 255),
                        static_cast<int>(layer.color.y * 255),
                        static_cast<int>(layer.color.z * 255), 255);
                    ImGui::GetWindowDrawList()->AddRectFilled(
                        cardPos, ImVec2(cardPos.x + 6, cardPos.y + cardSize.y),
                        colorIndicator, 4.0f, ImDrawFlags_RoundCornersLeft);

                    ImGui::SetCursorScreenPos(ImVec2(cardPos.x + 12, cardPos.y + 8));
                    ImGui::BeginGroup();
                    {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);

                        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.2f, 0.25f, 0.3f, 0.8f));
                        ImGui::SetNextItemWidth(250);
                        char nameBuf[64];
                        strncpy(nameBuf, layer.name.c_str(), sizeof(nameBuf) - 1);
                        if (ImGui::InputText("##Name", nameBuf, sizeof(nameBuf)))
                            layer.name = nameBuf;
                        ImGui::PopStyleColor();

                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(80);
                        if (ImGui::ColorEdit3("##Color", &layer.color.x,
                            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Layer Color");

                        ImGui::SameLine();
                        if (ImGui::SmallButton("Duplicate")) {
                            if (layers.size() < 32) {
                                Scene::Layer dup = layer;
                                dup.id = static_cast<int>(layers.size());
                                dup.mask = 1 << dup.id;
                                dup.name = layer.name + " Copy";
                                layers.push_back(dup);

                                size_t n = layers.size();
                                maskMatrix.resize(n);
                                for (auto& row : maskMatrix) row.resize(n, false);
                            }
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duplicate layer");

                        bool wasEnabled = layer.enabled;
                        if (ImGui::Checkbox(layer.enabled ? "Visible" : "Hidden", &layer.enabled)) {
                            if (wasEnabled != layer.enabled)
                                PN_CORE_INFO("[LayerPanel] Layer '{}' {}", layer.name, layer.enabled ? "enabled" : "disabled");
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip(layer.enabled ? "Layer Visible" : "Layer Hidden");
                        ImGui::SameLine();

                        bool wasPickable = layer.pickable;
                        if (ImGui::Checkbox(layer.pickable ? "Pickable" : "Not Pickable", &layer.pickable)) {
                            if (wasPickable != layer.pickable)
                                PN_CORE_INFO("[LayerPanel] Layer '{}' {}", layer.name, layer.pickable ? "pickable" : "locked");
                        }
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip(layer.pickable ? "Can be selected by mouse" : "Cannot be selected by mouse");

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
                        ImGui::Text("ID");
                        ImGui::SameLine();
                        ImGui::SetNextItemWidth(65);
                        if (ImGui::InputInt("##LayerID", &layer.id, 1, 100, ImGuiInputTextFlags_ElideLeft)) {
                            if (layer.id < 0)  layer.id = 0;
                            if (layer.id > 31) layer.id = 31;
                            layer.mask = 1 << layer.id;
                        }
                        ImGui::SameLine(125);
                        ImGui::Text("Mask: 0x%08X", layer.mask);
                        ImGui::SameLine(250);

                        int collisionCount = 0;
                        if (i < maskMatrix.size())
                            for (bool canCollide : maskMatrix[i]) if (canCollide) collisionCount++;
                        ImGui::Text("Collides with: %d layers", collisionCount);
                        ImGui::PopStyleColor();

                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4);
                        int entityCount = 0;
                        auto it = layerEntityCounts.find(layer.id);
                        if (it != layerEntityCounts.end()) entityCount = it->second;

                        ImVec4 countColor = entityCount > 0
                            ? ImVec4(0.7f, 0.9f, 0.7f, 1.0f)
                            : ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
                        ImGui::PushStyleColor(ImGuiCol_Text, countColor);
                        ImGui::Text("Entities: %d", entityCount);
                        ImGui::PopStyleColor();
                    }
                    ImGui::EndGroup();

                    ImGui::SetCursorScreenPos(ImVec2(cardPos.x, cardPos.y));
                    ImGui::InvisibleButton("##CardButton", cardSize);
                    if (ImGui::IsItemClicked()) selectedLayerIdx_ = i;

                    if (ImGui::BeginPopupContextItem(("LayerContext" + std::to_string(i)).c_str())) {
                        ImGui::TextColored(ImVec4(layer.color.x, layer.color.y, layer.color.z, 1.0f),
                            "Layer: %s", layer.name.c_str());
                        ImGui::Separator();

                        if (ImGui::MenuItem("Duplicate")) {
                            if (layers.size() < 32) {
                                Scene::Layer dup = layer;
                                dup.id = static_cast<int>(layers.size());
                                dup.mask = 1 << dup.id;
                                dup.name = layer.name + " Copy";
                                layers.push_back(dup);
                                size_t n = layers.size();
                                maskMatrix.resize(n);
                                for (auto& row : maskMatrix) row.resize(n, false);
                            }
                        }
                        if (ImGui::MenuItem(layer.enabled ? "Disable" : "Enable"))
                            layer.enabled = !layer.enabled;

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
                            for (auto& row : maskMatrix) row.resize(n, false);
                            if (selectedLayerIdx_ >= layers.size() && !layers.empty())
                                selectedLayerIdx_ = static_cast<unsigned>(layers.size() - 1);
                            ImGui::EndPopup();
                            ImGui::PopID();
                            break;
                        }
                        ImGui::EndPopup();
                    }

                    ImGui::Dummy(ImVec2(0, 3));
                    ImGui::PopID();
                }

                ImGui::EndChild();
            }

            // ----------------------------------------------------------------
            // Graphics Settings Panel
            // ----------------------------------------------------------------
            void ScenesPanel::drawGraphicsSettingsPanel() {
                auto scn_service = services->get<Scene::SceneManager>();
                Assets::GUID curr_skybox = scn_service->getCurrSkyBoxTextureID();

                if (DrawAssetSelectorField("Select A Skybox",
                    curr_skybox,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    services, false, { ".hdr" })) {
                    if (curr_skybox != scn_service->getCurrSkyBoxTextureID())
                        scn_service->setCurrSkyBoxTexture(curr_skybox);
                }

                if (scn_service->getWorldLight())
                    ImGui::ColorEdit3("World Light Intensity", glm::value_ptr(scn_service->getWorldLight()->L_intensity));
                if (scn_service->getCameraLight())
                    ImGui::ColorEdit3("Camera Light Intensity", glm::value_ptr(scn_service->getCameraLight()->L_intensity));

                auto& gs = GraphicsSettings::get();

                bool using_wlight = gs.world_light;
                if (ImGui::Checkbox("Using World Light", &using_wlight))
                    gs.world_light = using_wlight;

                bool using_ibl = gs.ibl;
                if (ImGui::Checkbox("Using IBL", &using_ibl))
                    gs.ibl = using_ibl;

                bool using_diffuse = gs.DEBUG_USE_DIFFUSE_MAP;
                if (ImGui::Checkbox("Using Diffuse Map", &using_diffuse))
                    gs.DEBUG_USE_DIFFUSE_MAP = using_diffuse;

                bool using_ao = gs.DEBUG_USE_AO_MAP;
                if (ImGui::Checkbox("Using AO Map", &using_ao))
                    gs.DEBUG_USE_AO_MAP = using_ao;

                bool using_normal = gs.DEBUG_USE_NORMAL_MAP;
                if (ImGui::Checkbox("Using Normal Map", &using_normal))
                    gs.DEBUG_USE_NORMAL_MAP = using_normal;

                bool using_rm = gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP;
                if (ImGui::Checkbox("Using Roughness Metallic Map", &using_rm))
                    gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP = using_rm;

                bool using_emissive = gs.DEBUG_USE_EMISSION_MAP;
                if (ImGui::Checkbox("Using Emissive Map", &using_emissive))
                    gs.DEBUG_USE_EMISSION_MAP = using_emissive;

                static int selected_pbr_index = 0;
                selected_pbr_index = gs.DEBUG_PBR_MAP_TYPE;
                if (ImGui::Combo("PBR Map Types", &selected_pbr_index,
                    gs.DEBUG_PBR_MAP_STRING.data(), gs.DEBUG_PBR_MAP_STRING.size())) {
                    if (selected_pbr_index >= 0 && selected_pbr_index < (int)gs.DEBUG_PBR_MAP_STRING.size())
                        gs.DEBUG_PBR_MAP_TYPE = static_cast<GraphicsSettings::DEBUG_PBR_MAP_TYPES>(selected_pbr_index);
                }
            }

            // ----------------------------------------------------------------
            // Minimap Settings Panel
            // ----------------------------------------------------------------
            void ScenesPanel::drawMinimapSettingsPanel() {
                auto& gs = GraphicsSettings::get();

                bool minimapEnabled = gs.minimap_enabled;
                if (ImGui::Checkbox("Enable Minimap", &minimapEnabled))
                    gs.minimap_enabled = minimapEnabled;

                ImGui::BeginDisabled(!gs.minimap_enabled);

                ImGui::DragFloat("Minimap Radius", &gs.minimap_radius, 0.25f, 2.0f, 100.0f, "%.1f");
                ImGui::DragFloat("Minimap Camera Height", &gs.minimap_camera_height, 0.25f, 2.0f, 200.0f, "%.1f");
                ImGui::DragFloat2("Minimap Size (px)", glm::value_ptr(gs.minimap_size_px), 1.0f, 64.0f, 1024.0f, "%.0f");

                static const char* recommendedPositions[] = {
                    "Top Left","Top Right","Bottom Left","Bottom Right","Top Middle","Bottom Middle"
                };
                int recommendedPos = static_cast<int>(gs.minimap_recommended_position);
                if (ImGui::Combo("Recommended Position", &recommendedPos, recommendedPositions, IM_ARRAYSIZE(recommendedPositions)))
                    gs.minimap_recommended_position = static_cast<GraphicsSettings::MINIMAP_RECOMMENDED_POSITION>(recommendedPos);

                bool overridePos = gs.minimap_override_position;
                if (ImGui::Checkbox("Override Position", &overridePos))
                    gs.minimap_override_position = overridePos;

                ImGui::BeginDisabled(!gs.minimap_override_position);
                float maxPosX = 8192.0f, maxPosY = 8192.0f;
                if (auto window = services->get<Window::Window>()) {
                    const glm::vec2 fb = window->getFrameBuffer();
                    maxPosX = glm::max(0.0f, fb.x - gs.minimap_size_px.x);
                    maxPosY = glm::max(0.0f, fb.y - gs.minimap_size_px.y);
                }
                ImGui::DragFloat("Minimap Pos X (px)", &gs.minimap_pos_px.x, 1.0f, 0.0f, maxPosX, "%.0f");
                ImGui::DragFloat("Minimap Pos Y (px)", &gs.minimap_pos_px.y, 1.0f, 0.0f, maxPosY, "%.0f");
                ImGui::EndDisabled();

                ImGui::TextDisabled("Recommended position respects minimap size.");

                bool rotateWithCamera = gs.minimap_rotate_with_player;
                if (ImGui::Checkbox("Rotate With Camera", &rotateWithCamera))
                    gs.minimap_rotate_with_player = rotateWithCamera;

                static const char* routeModes[] = {
                    "Nearest Target Line","Breadcrumb Dots","Edge Arrow","Line + Edge Arrow"
                };
                int routeMode = static_cast<int>(gs.minimap_route_mode);
                if (ImGui::Combo("Route Mode", &routeMode, routeModes, IM_ARRAYSIZE(routeModes)))
                    gs.minimap_route_mode = static_cast<GraphicsSettings::MINIMAP_ROUTE_MODE>(routeMode);

                ImGui::Separator();
                ImGui::TextUnformatted("Category Visibility");
                ImGui::Checkbox("Show Player", &gs.minimap_show_player);
                ImGui::Checkbox("Show Danger", &gs.minimap_show_danger);
                ImGui::Checkbox("Show Items", &gs.minimap_show_items);
                ImGui::Checkbox("Show Objective", &gs.minimap_show_objective);
                ImGui::Checkbox("Show Walls", &gs.minimap_show_walls);
                ImGui::Checkbox("Show Route", &gs.minimap_show_route);

                ImGui::Separator();
                ImGui::TextUnformatted("Icons");
                ImGui::Checkbox("Use Icon Textures", &gs.minimap_use_icon_textures);
                ImGui::DragFloat("Icon Scale", &gs.minimap_icon_scale, 0.05f, 0.2f, 4.0f, "%.2f");

                auto drawPathField = [](const char* label, std::string& value) {
                    char buffer[256]{};
                    strncpy(buffer, value.c_str(), sizeof(buffer) - 1);
                    if (ImGui::InputText(label, buffer, IM_ARRAYSIZE(buffer))) value = buffer;
                    };

                ImGui::BeginDisabled(!gs.minimap_use_icon_textures);
                drawPathField("Player Icon Path", gs.minimap_icon_player_path);
                drawPathField("Danger Icon Path", gs.minimap_icon_danger_path);
                drawPathField("Item Icon Path", gs.minimap_icon_item_path);
                drawPathField("Objective Icon Path", gs.minimap_icon_objective_path);
                drawPathField("Wall Icon Path", gs.minimap_icon_wall_path);
                ImGui::EndDisabled();

                ImGui::Checkbox("Show Legend", &gs.minimap_show_legend);
                ImGui::SliderFloat("Minimap Background Alpha", &gs.minimap_background_alpha, 0.0f, 1.0f, "%.2f");
                ImGui::DragFloat("Minimap Border Thickness", &gs.minimap_border_thickness, 0.1f, 0.0f, 10.0f, "%.1f");
                ImGui::ColorEdit4("Minimap Border Color", glm::value_ptr(gs.minimap_border_color));

                ImGui::EndDisabled();
            }

            // ----------------------------------------------------------------
            // Floor Settings Panel
            // ----------------------------------------------------------------
            void ScenesPanel::drawFloorSettingsPanel() {
                auto scene = services->get<Scene::SceneManager>();
                if (!scene) return;

                bool floorEnabled = scene->isFloorEnabled();
                if (ImGui::Checkbox("Enable Floor Collision", &floorEnabled)) {
                    scene->setFloorEnabled(floorEnabled);
                    auto physics = services->get<ECS::Controller>()->getSystem<Physics::System>();
                    if (physics) physics->set_floor_enabled(floorEnabled);
                }
                ImGui::Spacing();

                ImGui::BeginDisabled(!floorEnabled);

                glm::vec3 floorPos = scene->getFloorPosition();
                float floorPosArray[3] = { floorPos.x, floorPos.y, floorPos.z };
                if (ImGui::DragFloat3("Floor Position", floorPosArray, 0.1f)) {
                    scene->setFloorPosition(glm::vec3(floorPosArray[0], floorPosArray[1], floorPosArray[2]));
                    auto physics = services->get<ECS::Controller>()->getSystem<Physics::System>();
                    if (physics) physics->create_floor();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Center position of the floor collider");
                ImGui::Spacing();

                glm::vec3 floorExtents = scene->getFloorExtents();
                float floorExtentsArray[3] = { floorExtents.x, floorExtents.y, floorExtents.z };
                if (ImGui::DragFloat3("Floor Half-Extents", floorExtentsArray, 0.1f, 0.01f, 1000.0f)) {
                    scene->setFloorExtents(glm::vec3(floorExtentsArray[0], floorExtentsArray[1], floorExtentsArray[2]));
                    auto physics = services->get<ECS::Controller>()->getSystem<Physics::System>();
                    if (physics) physics->create_floor();
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Half-extents of the floor box (total size = 2x half-extents)");

                ImGui::EndDisabled();

                if (!floorEnabled) {
                    ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "Floor collision is DISABLED");
                    ImGui::TextWrapped("Camera will not collide with the floor. Useful for flying cameras.");
                }
                else {
                    ImVec4 infoColor(0.7f, 0.7f, 0.7f, 1.0f);
                    glm::vec3 pos = scene->getFloorPosition();
                    glm::vec3 ext = scene->getFloorExtents();
                    ImGui::TextColored(infoColor, "Floor top surface: Y=%.2f", pos.y + ext.y);
                    ImGui::TextColored(infoColor, "Floor bottom: Y=%.2f", pos.y - ext.y);
                }
            }

            // ----------------------------------------------------------------
            // Active Camera Panel
            // ----------------------------------------------------------------
            void ScenesPanel::drawActiveCamPanel() {
                auto scene = services->get<Scene::SceneManager>();
                if (!scene) return;

                const auto& cameras = scene->GetAllGameCamera();
                std::string active_game_cam = scene->GetActiveGameCamera();

                int current_active_index = -1;
                int loop_index = 0;

                cached_camera_names_ptr.clear();
                for (const auto& [name, camPtr] : cameras) {
                    cached_camera_names_ptr.push_back(name.c_str());
                    if (name == active_game_cam) current_active_index = loop_index;
                    loop_index++;
                }

                if (current_active_index != -1) selected_cam_index = current_active_index;

                if (ImGui::Combo("Select Active Camera", &selected_cam_index,
                    cached_camera_names_ptr.data(), cached_camera_names_ptr.size())) {
                    if (selected_cam_index >= 0 && selected_cam_index < (int)cached_camera_names_ptr.size()) {
                        const char* selectedName = cached_camera_names_ptr[selected_cam_index];
                        auto it = cameras.find(selectedName);
                        if (it != cameras.end()) scene->ChangeGameCamera(it->first);
                    }
                }
                ImGui::Spacing();

                auto cam = scene->GetActiveCamera();
                float curr_speed = cam->speed;
                float curr_sens = cam->sensitivity;

                if (ImGui::DragFloat("Camera Speed", &curr_speed, 0.1f, 0.1f, 100.0f)) cam->speed = curr_speed;
                ImGui::Spacing();
                if (ImGui::DragFloat("Camera Sensitivity", &curr_sens, 0.1f, 0.1f, 100.0f)) cam->sensitivity = curr_sens;
                ImGui::Spacing();
            }

            // ----------------------------------------------------------------
            // Loading Screen Panel
            // ----------------------------------------------------------------
            void ScenesPanel::drawLoadingScreenPanel() {
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
                // Progress Bar
                // ============================================================
                if (ImGui::CollapsingHeader("Progress Bar", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    glm::vec2 barPos = loadingScreen->getProgressBarPosition();
                    float barPosArray[2] = { barPos.x, barPos.y };
                    ImGui::Text("Position (Screen Space)");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::DragFloat2("##BarPos", barPosArray, 1.0f, 0.0f, 2000.0f, "%.0f px"))
                        loadingScreen->setProgressBarPosition(barPosArray[0], barPosArray[1]);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Position in screen space (X, Y) from top-left corner");

                    ImGui::Spacing();

                    glm::vec2 barSize = loadingScreen->getProgressBarSize();
                    float barSizeArray[2] = { barSize.x, barSize.y };
                    ImGui::Text("Size");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::DragFloat2("##BarSize", barSizeArray, 1.0f, 50.0f, 2000.0f, "%.0f px"))
                        loadingScreen->setProgressBarSize(barSizeArray[0], barSizeArray[1]);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Progress bar dimensions (Width, Height) in pixels");

                    ImGui::Spacing();

                    bool showProgress = loadingScreen->getShowProgressBar();
                    if (ImGui::Checkbox("Progress Bar Shown", &showProgress))
                        loadingScreen->setShowProgressBar(showProgress);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show progress bar on loading screen");

                    // -- Position presets (resolution-aware) --
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::Text("Position Presets:");

                    if (auto win = services->get<Window::Window>()) {
                        auto fb = win->getFrameBuffer();
                        float sw = fb.x;
                        float sh = fb.y;

                        if (ImGui::Button("Center Bottom")) {
                            loadingScreen->setProgressBarPosition(sw * 0.5f, sh * 0.85f);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Center Middle")) {
                            loadingScreen->setProgressBarPosition(sw * 0.5f, sh * 0.5f);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Center Top")) {
                            loadingScreen->setProgressBarPosition(sw * 0.5f, sh * 0.15f);
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
                        ImGui::TextWrapped("Resolution: %.0f x %.0f", sw, sh);
                        ImGui::PopStyleColor();
                    }

                    ImGui::Unindent();
                }

                ImGui::Spacing();

                // ============================================================
                // Status Text
                // ============================================================
                if (ImGui::CollapsingHeader("Status Text", ImGuiTreeNodeFlags_DefaultOpen)) {
                    ImGui::Indent();

                    glm::vec2 textPos = loadingScreen->getStatusTextPosition();
                    float textPosArray[2] = { textPos.x, textPos.y };
                    ImGui::Text("Position (Screen Space)");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::DragFloat2("##TextPos", textPosArray, 1.0f, 0.0f, 2000.0f, "%.0f px"))
                        loadingScreen->setStatusTextPosition(textPosArray[0], textPosArray[1]);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Text position (X, Y) from top-left corner");

                    ImGui::Spacing();

                    float textScale = loadingScreen->getStatusTextScale();
                    ImGui::Text("Font Scale");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::SliderFloat("##TextScale", &textScale, 0.01f, 0.1f, "%.2f"))
                        loadingScreen->setStatusTextScale(textScale);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Multiplier for text size");

                    ImGui::Spacing();

                    bool showStatus = loadingScreen->getShowStatusText();
                    if (ImGui::Checkbox("Status Text Shown", &showStatus))
                        loadingScreen->setShowStatusText(showStatus);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Status Text on loading screen");

                    // -- Position presets (resolution-aware) --
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::Text("Quick Presets:");

                    if (auto win = services->get<Window::Window>()) {
                        auto fb = win->getFrameBuffer();
                        float sw = fb.x;
                        float sh = fb.y;
                        glm::vec2 bp = loadingScreen->getProgressBarPosition();

                        if (ImGui::Button("Below Bar")) {
                            loadingScreen->setStatusTextPosition(bp.x, bp.y + 50.0f);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Above Bar")) {
                            loadingScreen->setStatusTextPosition(bp.x, bp.y - 50.0f);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Screen Center")) {
                            loadingScreen->setStatusTextPosition(sw * 0.5f, sh * 0.5f);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button("Bottom")) {
                            loadingScreen->setStatusTextPosition(sw * 0.5f, sh * 0.92f);
                        }

                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
                        ImGui::TextWrapped("Bar at (%.0f, %.0f)  —  Resolution: %.0f x %.0f",
                            bp.x, bp.y, sw, sh);
                        ImGui::PopStyleColor();
                    }

                    ImGui::Unindent();
                }

                ImGui::Spacing();

                // ============================================================
                // Style & Colors
                // ============================================================
                if (ImGui::CollapsingHeader("Style & Colors")) {
                    ImGui::Indent();

                    auto bgColor = loadingScreen->getBackgroundColor();
                    float bgfillColorArray[3] = { bgColor.r, bgColor.g, bgColor.b };
                    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Background Appearance");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Background Fill Color:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::ColorEdit3("##BGFillColor", bgfillColorArray,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
                        loadingScreen->setBackgroundColor(glm::vec3(bgfillColorArray[0], bgfillColorArray[1], bgfillColorArray[2]));
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color of the background");

                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.9f, 0.9f, 1.0f, 1.0f), "Progress Bar Appearance");
                    ImGui::Separator();
                    ImGui::Spacing();

                    auto [fillColor, glowColor, glowIntensity] = loadingScreen->getProgressBarStyle();
                    float fillColorArray[3] = { fillColor.r, fillColor.g, fillColor.b };
                    float glowColorArray[3] = { glowColor.r, glowColor.g, glowColor.b };
                    float intensity = glowIntensity;

                    ImGui::Text("Fill Color:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::ColorEdit3("##FillColor", fillColorArray,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
                        loadingScreen->setProgressBarFillColor(glm::vec3(fillColorArray[0], fillColorArray[1], fillColorArray[2]));
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color of the filled progress portion");

                    ImGui::Spacing();
                    ImGui::Text("Glow Color:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::ColorEdit3("##GlowColor", glowColorArray,
                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel))
                        loadingScreen->setProgressBarGlowColor(glm::vec3(glowColorArray[0], glowColorArray[1], glowColorArray[2]));
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Color of the animated glow effect");

                    ImGui::Spacing();
                    ImGui::Text("Glow Intensity:");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::SliderFloat("##GlowIntensity", &intensity, 0.0f, 2.0f, "%.2f"))
                        loadingScreen->setProgressBarGlowIntensity(intensity);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Intensity of the glow effect (0 = off, 2 = maximum)");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
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
                // Background
                // ============================================================
                if (ImGui::CollapsingHeader("Background")) {
                    ImGui::Indent();

                    ImGui::TextWrapped("Set a background texture and optionally configure spritesheet animation.");
                    ImGui::Spacing();

                    Assets::GUID currentBg = scn_service->loadingScreen->getBackgroundTexture();
                    if (DrawAssetSelectorField("Background Texture",
                        currentBg,
                        PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                        services, false)) {
                        if (currentBg.IsValid()) loadingScreen->setBackgroundTexture(currentBg);
                    }

                    ImGui::Spacing();

                    float bgScale = loadingScreen->getBGScale();
                    ImGui::Text("Background Texture Scale");
                    ImGui::SetNextItemWidth(200);
                    if (ImGui::SliderFloat("##BGScale", &bgScale, 0.1f, 10.0f, "%.2f"))
                        loadingScreen->setBGScale(bgScale);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.5f, 1.0f), "Spritesheet Animation");
                    ImGui::TextWrapped("Configure spritesheet-based animation for the background texture.");
                    ImGui::Spacing();

                    auto [frameCount, framesPerRow, frameTime, enabled] = loadingScreen->getSpritesheetSettings();

                    int tempFrameCount = frameCount;
                    ImGui::Text("Frame Count:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::InputInt("##FrameCount", &tempFrameCount, 1, 10))
                        if (tempFrameCount >= 1 && tempFrameCount <= 1000)
                            loadingScreen->setSpritesheetAnimation(tempFrameCount, framesPerRow, frameTime);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Total number of frames in the spritesheet");

                    int tempFramesPerRow = framesPerRow;
                    ImGui::Text("Frames Per Row:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::InputInt("##FramesPerRow", &tempFramesPerRow, 1, 10))
                        if (tempFramesPerRow >= 1 && tempFramesPerRow <= 100)
                            loadingScreen->setSpritesheetAnimation(frameCount, tempFramesPerRow, frameTime);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Number of frames per row in the spritesheet layout");

                    float tempFrameTime = frameTime;
                    ImGui::Text("Frame Duration:");
                    ImGui::SetNextItemWidth(150);
                    if (ImGui::SliderFloat("##FrameTime", &tempFrameTime, 0.01f, 1.0f, "%.2f sec"))
                        loadingScreen->setSpritesheetAnimation(frameCount, framesPerRow, tempFrameTime);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Time to display each frame");

                    bool tempEnabled = enabled;
                    if (ImGui::Checkbox("Enable Animation", &tempEnabled))
                        loadingScreen->setAnimationEnabled(tempEnabled);

                    ImGui::Spacing();

                    bool showBG = loadingScreen->getShowBG();
                    if (ImGui::Checkbox("Texture Background Shown", &showBG)) loadingScreen->setShowBG(showBG);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Texture As Background on loading screen");

                    ImGui::Spacing();

                    bool showOverlay = loadingScreen->getShowOverlay();
                    if (ImGui::Checkbox("Overlay Shown", &showOverlay)) loadingScreen->setShowOverlay(showOverlay);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show Overlay on loading screen");

                    ImGui::Unindent();
                }

                ImGui::Spacing();

                // ============================================================
                // Live Preview
                // ============================================================
                if (ImGui::CollapsingHeader("Live Preview")) {
                    ImGui::Indent();

                    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.8f, 1.0f), "Loading Screen Preview");
                    ImGui::TextWrapped("Preview the loading screen in real-time without reloading the scene.");
                    ImGui::Spacing();

                    static float previewProgress = 0.5f;
                    static char  previewStatus[256] = "Loading assets...";
                    static bool  autoAnimate = false;
                    static bool  enablePreview = false;

                    ImGui::Text("Progress:");
                    ImGui::SetNextItemWidth(200);
                    ImGui::SliderFloat("##PreviewProgress", &previewProgress, 0.0f, 1.0f, "%.2f");

                    ImGui::Spacing();
                    ImGui::Text("Status Text:");
                    ImGui::SetNextItemWidth(300);
                    ImGui::InputText("##PreviewStatus", previewStatus, IM_ARRAYSIZE(previewStatus));

                    ImGui::Spacing();
                    ImGui::Checkbox("Auto-animate progress", &autoAnimate);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Automatically cycle progress from 0%% to 100%%");

                    ImGui::Spacing();
                    ImGui::Checkbox("Enable Live Preview", &enablePreview);
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle real-time preview rendering (may impact performance)");

                    if (autoAnimate) {
                        previewProgress += 0.01f * ImGui::GetIO().DeltaTime;
                        if (previewProgress > 1.0f) previewProgress = 0.0f;
                    }

                    if (enablePreview)
                        loadingScreen->renderPreview(previewProgress, std::string(previewStatus));

                    ImGui::Unindent();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Default Configuration##LoadingScreen")) loadingScreen->defaultSetup();

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
                ImGui::TextWrapped("All settings are applied in real-time. Changes will be visible the next time the loading screen is displayed.");
                ImGui::PopStyleColor();
            }

            // ----------------------------------------------------------------
            // onAttach
            // ----------------------------------------------------------------
            void ScenesPanel::onAttach() {
#ifdef PN_PLATFORM_WINDOWS
                registerPopUp("CreateScene", createScenePopup("CreateScene"));
                registerPopUp("SaveSceneAs", saveSceneAsPopup("SaveSceneAs"));
                registerPopUp("DeleteScene", deleteScenePopup("DeleteScene"));
#endif
                registerPopUp("Info", defPopUp("Info"));
            }

            // ----------------------------------------------------------------
            // onUpdate
            // ----------------------------------------------------------------
            void ScenesPanel::onUpdate(AppTiming timing) {
                auto scn_service = services->get<Scene::SceneManager>();
                auto asset_service = services->get<Assets::Manager>();

                // ---- Active scene status ----
                {
                    auto scn_id = scn_service->getCurrScnID();
                    bool hasScene = asset_service->checkAssetRegistered(scn_id);
                    std::string scnName = hasScene ? asset_service->getAssetData(scn_id)->name : "(none)";
                    const auto dot = scnName.rfind('.');
                    if (dot != std::string::npos) scnName = scnName.substr(0, dot);

                    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Active Scene:");
                    ImGui::SameLine();
                    ImGui::TextUnformatted(scnName.c_str());
                }

                ImGui::Spacing();

                // ---- Toolbar strip ----
                {
                    bool hasSelected = selected.IsValid() && !selected_scn_name.empty();
                    bool isActiveScene = hasSelected && (scn_service->getCurrScnID() == selected);
                    bool isPlaying = scn_service->isPlaying();

#ifdef PN_PLATFORM_WINDOWS
                    if (ImGui::Button("+ New")) openPopUp("CreateScene");
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Create a new scene");
                    ImGui::SameLine();
#endif

                    ImGui::BeginDisabled(!hasSelected || isActiveScene);
                    if (ImGui::Button("Load")) scn_service->loadScene(selected);
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip(isActiveScene ? "Already the active scene" : "Load selected scene");
                    ImGui::SameLine();

#ifdef PN_PLATFORM_WINDOWS
                    ImGui::BeginDisabled(!isActiveScene || isPlaying);
                    if (ImGui::Button("Save")) {
                        scn_service->saveActiveScene(selected);
                        openPopUp("Info", std::make_shared<std::string>("Scene Saved!"));
                    }
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                        if (isPlaying)           ImGui::SetTooltip("Cannot save while playing");
                        else if (!isActiveScene) ImGui::SetTooltip("Select the active scene to save");
                        else                     ImGui::SetTooltip("Save current scene");
                    }
                    ImGui::SameLine();

                    ImGui::BeginDisabled(!isActiveScene);
                    if (ImGui::Button("Save As")) openPopUp("SaveSceneAs");
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("Save current scene under a new name");
                    ImGui::SameLine();

                    ImGui::BeginDisabled(!hasSelected);
                    if (ImGui::Button("Delete")) openPopUp("DeleteScene");
                    ImGui::EndDisabled();
                    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
                        ImGui::SetTooltip("Delete selected scene");
#endif
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // ---- Scene cards ----
                {
                    auto allScenes = asset_service->getAllAssetDataOfType(Assets::Type::Scenes);
                    Assets::GUID currScnID = scn_service->getCurrScnID();

                    static char sceneSearchBuf[128] = "";
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputTextWithHint("##SceneSearch", "Search scenes...", sceneSearchBuf, sizeof(sceneSearchBuf));
                    ImGui::Spacing();

                    if (allScenes.empty()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        float tw = ImGui::CalcTextSize("No scenes found").x;
                        ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - tw) * 0.5f);
                        ImGui::TextUnformatted("No scenes found");
                        ImGui::PopStyleColor();
                    }
                    else {
                        ImGui::BeginChild("##SceneCardList", ImVec2(0, 200), true,
                            ImGuiWindowFlags_HorizontalScrollbar);

                        std::string filter = sceneSearchBuf;
                        std::transform(filter.begin(), filter.end(), filter.begin(), ::tolower);

                        for (const auto& sceneData : allScenes) {
                            std::string displayName = sceneData->name;
                            const auto dot = displayName.rfind('.');
                            if (dot != std::string::npos) displayName = displayName.substr(0, dot);

                            if (!filter.empty()) {
                                std::string nameLower = displayName;
                                std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
                                if (nameLower.find(filter) == std::string::npos) continue;
                            }

                            ImGui::PushID(sceneData->guid.ToString().c_str());

                            bool isActive = (sceneData->guid == currScnID);
                            bool isSelected = (sceneData->guid == selected);

                            ImVec2 cardPos = ImGui::GetCursorScreenPos();
                            ImVec2 cardSize = ImVec2(ImGui::GetContentRegionAvail().x, 52.f);

                            ImU32 cardBg = isActive ? IM_COL32(40, 80, 55, 255)
                                : isSelected ? IM_COL32(50, 70, 100, 255)
                                : IM_COL32(38, 42, 48, 255);

                            ImU32 borderColor = isActive ? IM_COL32(80, 200, 120, 255)
                                : isSelected ? IM_COL32(100, 150, 230, 255)
                                : IM_COL32(60, 65, 70, 255);

                            ImDrawList* dl = ImGui::GetWindowDrawList();
                            dl->AddRectFilled(cardPos,
                                ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), cardBg, 4.f);
                            dl->AddRect(cardPos,
                                ImVec2(cardPos.x + cardSize.x, cardPos.y + cardSize.y), borderColor, 4.f, 0, 1.5f);

                            // Thumbnail placeholder
                            ImVec2 thumbMin = ImVec2(cardPos.x + 6, cardPos.y + 6);
                            ImVec2 thumbMax = ImVec2(cardPos.x + 42, cardPos.y + 42);
                            dl->AddRectFilled(thumbMin, thumbMax, IM_COL32(30, 30, 35, 255), 3.f);
                            dl->AddRect(thumbMin, thumbMax, IM_COL32(70, 70, 80, 255), 3.f);
                            dl->AddText(ImVec2(thumbMin.x + 10, thumbMin.y + 8),
                                IM_COL32(150, 150, 170, 255), isActive ? ">>>" : "SCN");

                            // Card text
                            ImGui::SetCursorScreenPos(ImVec2(cardPos.x + 50, cardPos.y + 8));
                            ImGui::BeginGroup();
                            {
                                if (isActive) {
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.6f, 1.0f));
                                    ImGui::TextUnformatted(displayName.c_str());
                                    ImGui::PopStyleColor();
                                    ImGui::SameLine();
                                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.5f, 1.0f));
                                    ImGui::TextUnformatted("[ACTIVE]");
                                    ImGui::PopStyleColor();
                                }
                                else {
                                    ImGui::TextUnformatted(displayName.c_str());
                                }

                                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.45f, 0.5f, 1.0f));
                                ImGui::TextUnformatted(sceneData->name.c_str());
                                ImGui::PopStyleColor();
                            }
                            ImGui::EndGroup();

                            // Invisible button for interaction
                            ImGui::SetCursorScreenPos(cardPos);
                            ImGui::InvisibleButton("##SceneCard", cardSize);

                            if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
                                selected = sceneData->guid;
                                selected_scn_name = sceneData->name;
                            }

                            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                                if (!isActive) {
                                    scn_service->loadScene(sceneData->guid);
                                    selected = sceneData->guid;
                                    selected_scn_name = sceneData->name;
                                }
                            }

                            if (ImGui::IsItemHovered())
                                ImGui::SetTooltip(isActive
                                    ? "Active scene\nDouble-click to reload"
                                    : "Click to select\nDouble-click to load");

                            ImGui::Dummy(ImVec2(0, 3));
                            ImGui::PopID();
                        }

                        ImGui::EndChild();
                    }
                }

                ImGui::Separator();
                ImGui::Spacing();

                // ---- Collapsing sections ----
                if (ImGui::CollapsingHeader("Graphics Settings"))  drawGraphicsSettingsPanel();
                if (ImGui::CollapsingHeader("Minimap Settings"))   drawMinimapSettingsPanel();
                if (ImGui::CollapsingHeader("Floor Settings"))     drawFloorSettingsPanel();
                if (ImGui::CollapsingHeader("Layer Settings"))     drawLayerManagementPanel();
                if (ImGui::CollapsingHeader("Camera Settings"))    drawActiveCamPanel();
                if (ImGui::CollapsingHeader("Loading Screen"))     drawLoadingScreenPanel();

                // ---- Modals ----
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