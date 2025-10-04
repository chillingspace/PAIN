#include "pch.h"
#include "ComponentsPanel.h"
#include "EntityPanel.h"
#include "Core.h"
#include "../Editor.h"
#include "ECS/sMetaData.h"

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

            ComponentsPanel::ComponentsPanel() {
                name = "Components";
                flags = ImGuiWindowFlags_None;
            }

            void ComponentsPanel::onAttach() {
                // Register component-specific UI
                RegisterTransformUI(*this);

                // Get entity panel reference
                auto editor = services->get<PAIN::Editor::Editor>();
                if (editor) {
                    entities_panel = editor->getPanel<EntityPanel>();
                }

                // Get ECS and initialize components list
                auto ecs = services->get<ECS::Controller>();
                if (ecs) {
                    comps = ecs->getAllComponentTypes();

                    // Initialize empty UI functions for unregistered components
                    for (auto& [comp_name, comp_type] : comps) {
                        if (comps_ui.find(comp_name) == comps_ui.end()) {
                            comps_ui.emplace(comp_name, [](ComponentsPanel&, void*) {
                                ImGui::TextDisabled("No UI registered");
                                });
                        }
                    }
                }

                // Register Add Component popup
                registerPopUp("AddComponent", addComponentPopUp("AddComponent"));

                // Register Remove Component popup
                registerPopUp("RemoveComponent", removeComponentPopUp("RemoveComponent"));
            }

            void ComponentsPanel::nextWindowSettings() {
                ImGui::SetNextWindowSize(ImVec2(350, 600), ImGuiCond_FirstUseEver);
            }

            std::function<void()> ComponentsPanel::addComponentPopUp(std::string const& popup_id) {
                return [this, popup_id]() {
                    auto ecs = services->get<ECS::Controller>();
                    auto entity_panel = entities_panel.lock();

                    if (!entity_panel) {
                        ImGui::Text("EntityPanel not available");
                        ImGui::Spacing();
                        if (ImGui::Button("Close", ImVec2(-1, 0))) {
                            closePopUp(popup_id);
                        }
                        return;
                    }

                    ECS::Entity::Type selected_entity = entity_panel->getSelectedEntity();

                    // Title
                    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Add Component");
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Search filter
                    static char search_filter[256] = "";
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputTextWithHint("##Search", "Search components...", search_filter, 256);
                    ImGui::Spacing();

                    // Scrollable component list
                    ImGui::BeginChild("##ComponentList", ImVec2(400, 350), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    bool found_any = false;
                    for (const auto& [comp_name, comp_type] : comps) {
                        // Apply search filter
                        if (strlen(search_filter) > 0) {
                            std::string comp_lower = comp_name;
                            std::string search_lower = search_filter;
                            std::transform(comp_lower.begin(), comp_lower.end(), comp_lower.begin(), ::tolower);
                            std::transform(search_lower.begin(), search_lower.end(), search_lower.begin(), ::tolower);

                            if (comp_lower.find(search_lower) == std::string::npos) {
                                continue;
                            }
                        }

                        found_any = true;

                        // Check if already added
                        bool already_has = ecs->checkEntityComponent(selected_entity, comp_type);

                        if (already_has) {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.7f));
                            ImGui::Selectable(comp_name.c_str(), false, ImGuiSelectableFlags_Disabled);
                            ImGui::PopStyleColor();
                            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                                ImGui::SetTooltip("Component already added");
                            }
                        }
                        else {
                            // Selectable component
                            if (ImGui::Selectable(comp_name.c_str(), false)) {
                                ecs->addDefEntityComponent(selected_entity, comp_type);
                                search_filter[0] = '\0';
                                closePopUp(popup_id);
                            }
                        }
                    }

                    if (!found_any) {
                        ImGui::TextDisabled("No components found");
                    }

                    ImGui::EndChild();

                    // Bottom buttons
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
                        search_filter[0] = '\0';
                        closePopUp(popup_id);
                    }
                };
            }

            std::function<void()> ComponentsPanel::removeComponentPopUp(std::string const& popup_id) {
                return [this, popup_id]() {
                    auto ecs = services->get<ECS::Controller>();
                    auto entity_panel = entities_panel.lock();

                    if (!ecs || !entity_panel) {
                        ImGui::Text("Required services not available");
                        ImGui::Spacing();
                        if (ImGui::Button("Close", ImVec2(-1, 0))) {
                            closePopUp(popup_id);
                        }
                        return;
                    }

                    ECS::Entity::Type entity = entity_panel->getSelectedEntity();
                    if (entity == ECS::Entity::INVALID || !ecs->checkEntity(entity)) {
                        ImGui::Text("No valid entity selected");
                        ImGui::Spacing();
                        if (ImGui::Button("Close", ImVec2(-1, 0))) {
                            closePopUp(popup_id);
                        }
                        return;
                    }

                    if (comp_string_ref.empty()) {
                        ImGui::Text("No component selected");
                        ImGui::Spacing();
                        if (ImGui::Button("Close", ImVec2(-1, 0))) {
                            closePopUp(popup_id);
                        }
                        return;
                    }

                    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Remove Component");
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Component:");
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", comp_string_ref.c_str());

                    ImGui::Spacing();
                    ImGui::TextWrapped("Are you sure you want to remove this component?");
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "This action cannot be undone.");

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // Center buttons
                    float button_width = 120.0f;
                    float spacing = ImGui::GetStyle().ItemSpacing.x;
                    float total_width = (button_width * 2) + spacing;
                    float offset = (ImGui::GetContentRegionAvail().x - total_width) * 0.5f;
                    if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                    // Remove button (red)
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));

                    bool remove_clicked = ImGui::Button("Remove", ImVec2(button_width, 0));

                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();

                    // Cancel button
                    bool cancel_clicked = ImGui::Button("Cancel", ImVec2(button_width, 0));

                    // Handle actions AFTER rendering all UI to avoid state conflicts
                    if (remove_clicked) {
                        // Get fresh component list
                        auto fresh_comps = ecs->getAllComponentTypes();
                        auto it = fresh_comps.find(comp_string_ref);

                        if (it != fresh_comps.end()) {
                            if (ecs->checkEntityComponent(entity, it->second)) {
                                ecs->removeEntityComponent(entity, it->second);
                            }
                        }

                        closePopUp(popup_id);
                        comp_string_ref.clear();  // Clear ref after removal
                    }

                    if (cancel_clicked) {
                        closePopUp(popup_id);
                    }
                    };
            }


            void ComponentsPanel::renderEntityComponents(ECS::Entity::Type entity) {
                auto ecs = services->get<ECS::Controller>();
                auto entity_components = ecs->getAllEntityComponents(entity);

                if (entity_components.empty()) {
                    ImGui::Spacing();
                    ImGui::TextDisabled("No components attached");
                    return;
                }

                for (auto& [comp_name, comp_type] : entity_components) {
                    ImGui::PushID(comp_name.c_str());

                    // Component header with TreeNode (Unity style)
                    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_DefaultOpen |
                        ImGuiTreeNodeFlags_Framed |
                        ImGuiTreeNodeFlags_SpanAvailWidth |
                        ImGuiTreeNodeFlags_AllowItemOverlap |
                        ImGuiTreeNodeFlags_FramePadding;

                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));
                    bool node_open = ImGui::TreeNodeEx(comp_name.c_str(), node_flags);
                    ImGui::PopStyleVar();

                    // Right-click context menu
                    if (ImGui::BeginPopupContextItem()) {
                        ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s",comp_name.c_str());
                        ImGui::Separator();

                        if (ImGui::MenuItem("Remove Component")) {
                            comp_string_ref = comp_name;
                            should_open_remove_popup = true;
                            ImGui::CloseCurrentPopup();  

                        }

                        if (ImGui::MenuItem("Reset to Default")) {
                            // TODO: Reset component
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("Copy Component")) {
                            // TODO: Copy component
                        }

                        if (ImGui::MenuItem("Paste Component Values")) {
                            // TODO: Paste component
                        }

                        ImGui::EndPopup();
                    }

                    if (node_open) {
                        ImGui::Spacing();

                        // Render component-specific UI
                        if (comps_ui.find(comp_name) != comps_ui.end()) {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                            comps_ui.at(comp_name)(*this, comp_type.get());
                            ImGui::PopStyleVar();
                        }
                        else {
                            ImGui::TextDisabled("No UI registered for this component");
                        }

                        ImGui::Spacing();
                        ImGui::TreePop();
                    }

                    ImGui::PopID();
                }
            }


            void ComponentsPanel::setCompStringRef(std::string const& to_set) {
                comp_string_ref = to_set;
            }

            void ComponentsPanel::onUpdate(AppTiming timing) {
                // Update components list if changed
                auto ecs = services->get<ECS::Controller>();
                if (ecs && comps.size() != ecs->getComponentsCount()) {
                    comps = ecs->getAllComponentTypes();

                    for (auto& [comp_name, comp_type] : comps) {
                        if (comps_ui.find(comp_name) == comps_ui.end()) {
                            comps_ui.emplace(comp_name, [](ComponentsPanel&, void*) {
                                ImGui::TextDisabled("No UI registered");
                                });
                        }
                    }
                }

                // Get entity panel
                auto entity_panel = entities_panel.lock();

                // Bro somehow ah the weakptr expires here... have to recover it
                if (!entity_panel) {

                    // One time recovery
                    auto editor = services->get<PAIN::Editor::Editor>();

                    if (editor) {

                        auto ep = editor->getPanel<Panel::EntityPanel>();

                        if (ep) entities_panel = ep;

                    }

                    entity_panel = entities_panel.lock();
                }

                if (!entity_panel) return;

                ECS::Entity::Type selected = entity_panel->getSelectedEntity();

                // No entity selected
                if (selected == ECS::Entity::INVALID || !ecs->checkEntity(selected)) {
                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Select an entity to inspect");
                    return;
                }

                // Entity header
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Entity: %u", static_cast<uint32_t>(selected));

                // Entity name from metadata
                auto metadata = services->get<MetaData::Service>();
                if (metadata) {
                    std::string entity_name = metadata->getEntityName(selected);
                    if (!entity_name.empty()) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "(%s)", entity_name.c_str());
                    }

                    // Check locked state
                    if (metadata->isLocked(selected)) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "[LOCKED]");
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::TextDisabled("Entity is locked. Unlock to edit components.");
                        return;
                    }
                }

                ImGui::Separator();
                ImGui::Spacing();

                // Component list
                renderEntityComponents(selected);

                // Add Component button
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImVec2 button_size = ImVec2(ImGui::GetContentRegionAvail().x, 35);
                if (ImGui::Button("Add Component", button_size)) {
                    openPopUp("AddComponent");
                }

                // Open remove popup if flagged (after context menu is closed)
                if (should_open_remove_popup) {
                    openPopUp("RemoveComponent");
                    should_open_remove_popup = false;
                }

                // Render all registered popups
                renderPopUps();
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
