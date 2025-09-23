#include "pch.h"
#include "ScenesPanel.h"

#ifdef _DEBUG

#include <algorithm>

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

            ScenesPanel::ScenesPanel(std::shared_ptr<CommandManager> cm, ScenesHooks hooks)
                : IPanel(std::move(cm)), hooks_(std::move(hooks)) {

                name = "Scene Manager";                 // visible window title
                flags = ImGuiWindowFlags_None;    // normal tool window

                // seed with a default layer so UI is usable immediately
                layers_.push_back(Layer{ 0, true });
                rebuildMaskSize(layers_.size());
            }

            void ScenesPanel::nextWindowSettings() {
                // default window; leave docking/frameless tricks to ToolsPanel only
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
                        // reset “scene” in our temporary model
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
                ImGui::OpenPopup("Edit Layer Bit Mask");
                if (ImGui::BeginPopupModal("Edit Layer Bit Mask", &showEditMask_, ImGuiWindowFlags_AlwaysAutoResize)) {
                    const unsigned n = static_cast<unsigned>(layers_.size());
                    ImGui::TextUnformatted("Bitmask Grid:");
                    if (ImGui::BeginTable("##BitmaskGrid", n + 1, ImGuiTableFlags_Borders)) {
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn(); ImGui::Text("Layer\\Mask");
                        for (unsigned j = 0; j < n; ++j) { ImGui::TableNextColumn(); ImGui::Text("L%u", layers_[j].id); }

                        for (unsigned i = 0; i < n; ++i) {
                            ImGui::TableNextRow();
                            ImGui::TableNextColumn(); ImGui::Text("L%u", layers_[i].id);

                            for (unsigned j = 0; j < n; ++j) {
                                ImGui::TableNextColumn();
                                if (i == j) { ImGui::TextUnformatted("X"); continue; }
                                bool bit = mask_[i][j];
                                if (ImGui::Checkbox((std::string("##m_") + std::to_string(i) + "_" + std::to_string(j)).c_str(), &bit)) {
                                    // symmetric toggle
                                    mask_[i][j] = mask_[j][i] = bit;
                                }
                            }
                        }
                        ImGui::EndTable();
                    }
                    ImGui::Spacing();
                    if (ImGui::Button("Done")) {
                        showEditMask_ = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }

            // ---------- Main draw ----------
            void ScenesPanel::onUpdate() {
                // Title & dock are handled by IPanel
                ImGui::Text("Scene ID: %s", currSceneId_.empty() ? "(none)" : currSceneId_.c_str());

                // Create / Delete / Save / Save As
                if (ImGui::Button("Create New Scene")) { showCreate_ = true; }

                if (!currSceneId_.empty()) {
                    ImGui::SameLine();
                    if (ImGui::Button("Delete Scene")) { showDelete_ = true; }

                    if (ImGui::Button("Save Scene As")) { showSaveAs_ = true; }

                    ImGui::SameLine();
                    if (ImGui::Button("Save Curr Scene")) {
                        if (hooks_.onSaveCurrent) hooks_.onSaveCurrent(currSceneId_);
                    }
                }

                ImGui::Separator();

                // Layers
                const unsigned n = static_cast<unsigned>(layers_.size());
                ImGui::Text("Total Layers: %u", n);

                if (ImGui::Button("Create Layer")) {
                    unsigned nextId = n ? (layers_.back().id + 1) : 0;
                    layers_.push_back(Layer{ nextId, true });
                    rebuildMaskSize(layers_.size());
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Layer")) {
                    if (layers_.size() > 1) {
                        layers_.erase(layers_.begin() + std::min<unsigned>(selectedLayerIdx_, layers_.size() - 1));
                        // re-number IDs compactly for this temporary model
                        for (unsigned i = 0; i < layers_.size(); ++i) layers_[i].id = i;
                        rebuildMaskSize(layers_.size());
                        selectedLayerIdx_ = layers_.empty() ? 0 : std::min<unsigned>(selectedLayerIdx_, layers_.size() - 1);
                        ensureAtLeastOneLayer();
                    }
                }

                ImGui::TextUnformatted("Layer List:");
                ImGui::BeginChild("##LayerList", ImVec2(0, 200), true);
                for (unsigned i = 0; i < layers_.size(); ++i) {
                    bool vis = layers_[i].visible;
                    if (ImGui::Checkbox((std::string("##vis_") + std::to_string(i)).c_str(), &vis)) {
                        layers_[i].visible = vis;
                    }
                    ImGui::SameLine();
                    std::string label = "Layer " + std::to_string(layers_[i].id);
                    if (ImGui::Selectable(label.c_str(), selectedLayerIdx_ == i)) {
                        selectedLayerIdx_ = i;
                    }
                }
                ImGui::EndChild();

                if (ImGui::Button("Edit Layer Bit Mask")) { showEditMask_ = true; }

                // Modals last
                drawCreateModal();
                drawDeleteModal();
                drawSaveAsModal();
                drawEditMaskModal();
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
