#include "pch.h"
#include "ComponentsPanel.h"
#include "EntityPanel.h"
#include "Core.h"
#include "../Editor.h"
#include "ECS/sMetaData.h"
#include "ECS/Components/cAudioSource.h"

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
                PAIN::Editor::Panel::RegisterReflected<PAIN::Transform>(*this, "Transform");

                PAIN::Editor::Panel::RegisterReflected<PAIN::MeshRenderer>(*this, "Mesh Renderer");

                PAIN::Editor::Panel::RegisterReflected<PAIN::Lighting>(*this, "Lighting");

                PAIN::Editor::Panel::RegisterReflected<PAIN::Audio::AudioSource>(*this, "Audio Source");

                PAIN::Editor::Panel::RegisterReflected<PAIN::BoundingVolume>(*this, "Bounding Volume"); 

                PAIN::Editor::Panel::RegisterReflected<PAIN::Hierarchy>(*this, "Hierarchy");

                //auto ecs = services->get<ECS::Controller>();
                //for (auto const& [name, _] : ecs->getComponentFactories()) {
                //    PN_CORE_INFO("Factory: {}", name); 
                //}
                
                // Get entity panel reference
                auto editor = services->get<PAIN::Editor::Editor>();
                if (editor) {
                    entities_panel = editor->getPanel<EntityPanel>();
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
                        if (ImGui::Button("Close", ImVec2(-1, 0))) {
                            closePopUp(popup_id);
                        }
                        return;
                    }

                    entt::entity selected_entity = entity_panel->getSelectedEntity();

                    if (!ecs->checkEntity(selected_entity)) {
                        ImGui::Text("No valid entity selected");
                        if (ImGui::Button("Close", ImVec2(-1, 0))) {
                            closePopUp(popup_id);
                        }
                        return;
                    }

                    ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "Add Component");
                    ImGui::Separator();
                    ImGui::Spacing();

                    static char search_filter[256] = "";
                    ImGui::SetNextItemWidth(-1);
                    ImGui::InputTextWithHint("##Search", "Search components...", search_filter, 256);
                    ImGui::Spacing();

                    ImGui::BeginChild("##ComponentList", ImVec2(400, 350), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

                    bool found_any = false;

                    // Iterate registered component factories
                    for (const auto& [comp_name, factory_func] : ecs->getComponentFactories()) {
                        
                        if (comp_name == "Name" ||
                            comp_name == "Tag" ||
                            comp_name == "Editor Visiblity" ||
                            comp_name == "Relation" ||
                            comp_name == "Group") {
                            continue;
                        }

                        // Skip if entity already has this component
                        if (ecs->hasComponentByName(selected_entity, comp_name)) {
                            continue;
                        }

                        // Search filter
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

                        if (ImGui::Selectable(comp_name.c_str(), false)) {
                            ecs->addComponentByName(selected_entity, comp_name);
                            search_filter[0] = '\0';
                            closePopUp(popup_id);
                        }
                    }

                    if (!found_any) {
                        ImGui::TextDisabled("No available components");
                    }

                    ImGui::EndChild();
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

                    entt::entity entity = entity_panel->getSelectedEntity();
                    if (entity == entt::null || !ecs->checkEntity(entity)) {
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

                    bool cancel_clicked = ImGui::Button("Cancel", ImVec2(button_width, 0));

                    if (remove_clicked) {

                        if (comp_string_ref == "Name" ||
                            comp_string_ref == "Tag" ||
                            comp_string_ref == "Editor Visiblity" ||
                            comp_string_ref == "Relation" ||
                            comp_string_ref == "Group") {
                            closePopUp(popup_id);
                            comp_string_ref.clear();
                            return;
                        }

                        // Use new removeComponentByName method
                        if (ecs->hasComponentByName(entity, comp_string_ref)) {
                            ecs->removeComponentByName(entity, comp_string_ref);
                        }

                        closePopUp(popup_id);
                        comp_string_ref.clear();
                    }

                    if (cancel_clicked) {
                        closePopUp(popup_id);
                    }
                };
            }

            void ComponentsPanel::renderEntityComponents(entt::entity entity) {
                auto ecs = services->get<ECS::Controller>();

                if (!ecs || !ecs->checkEntity(entity)) {
                    ImGui::Spacing();
                    ImGui::TextDisabled("Invalid entity");
                    return;
                }

                // Get all component names for this entity
                auto component_names = ecs->getEntityComponentNames(entity);

                if (component_names.empty()) {
                    ImGui::Spacing();
                    ImGui::TextDisabled("No components attached");
                    return;
                }

                for (const auto& comp_name : component_names) {
                    // Skip Metadata Component
                    if (comp_name == "Name" ||
                        comp_name == "Tag" ||
                        comp_name == "Editor Visiblity" ||
                        comp_name == "Relation" ||
                        comp_name == "Group") {
                        continue;
                    }

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
                        ImGui::TextColored(ImVec4(0.8f, 0.9f, 1.0f, 1.0f), "%s", comp_name.c_str());
                        ImGui::Separator();

                        if (ImGui::MenuItem("Remove Component")) {
                            comp_string_ref = comp_name;
                            should_open_remove_popup = true;
                            ImGui::CloseCurrentPopup();
                        }

                        if (ImGui::MenuItem("Reset to Default")) {
                            // TODO: Implement reset - would need default component values
                            // For now, could remove and re-add with defaults
                            ecs->removeComponentByName(entity, comp_name);
                            ecs->addComponentByName(entity, comp_name);
                        }

                        ImGui::Separator();

                        if (ImGui::MenuItem("Copy Component")) {
                            // TODO: Serialize component to clipboard
                            ImGui::SetClipboardText(comp_name.c_str());
                        }

                        if (ImGui::MenuItem("Paste Component Values")) {
                            // TODO: Deserialize from clipboard
                        }

                        ImGui::EndPopup();
                    }

                    if (node_open) {
                        ImGui::Spacing();

                        // Get component pointer (type-erased)
                        void* comp_ptr = ecs->getComponentPtrByName(entity, comp_name);

                        // Render component-specific UI
                        if (comps_ui.find(comp_name) != comps_ui.end() && comp_ptr) {
                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                            comps_ui.at(comp_name)(*this, comp_ptr);
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
                auto ecs = services->get<ECS::Controller>();
                if (!ecs) {
                    ImGui::Spacing();
                    ImGui::TextDisabled("ECS Controller unavailable");
                    return;
                }

                // Rebuild component name set
                for (const auto& [comp_name, factory] : ecs->getComponentFactories()) {
                    // Register default UI handlers for any components that don't have one yet
                    if (comps_ui.find(comp_name) == comps_ui.end()) {
                        comps_ui.emplace(comp_name, [](ComponentsPanel&, void*) {
                            ImGui::TextDisabled("No UI registered for this component");
                            });
                    }
                }

                // Ensure entity panel reference is valid
                auto entity_panel = entities_panel.lock();
                if (!entity_panel) {
                    // Recover weak_ptr if it expired (happens on panel reload/scene change)
                    auto editor = services->get<PAIN::Editor::Editor>();
                    if (editor) {
                        auto ep = editor->getPanel<Panel::EntityPanel>();
                        if (ep) {
                            entities_panel = ep;
                            entity_panel = ep;
                        }
                    }
                }

                if (!entity_panel) {
                    ImGui::Spacing();
                    ImGui::TextDisabled("Entity Panel not available");
                    return;
                }

                // Get selected entity
                entt::entity selected = entity_panel->getSelectedEntity();

                // No entity selected - show placeholder
                if (selected == entt::null || !ecs->checkEntity(selected)) {
                    ImGui::Spacing();
                    ImGui::Spacing();

                    // Centered placeholder text
                    const char* placeholder = "Select an entity to inspect";
                    float text_width = ImGui::CalcTextSize(placeholder).x;
                    float window_width = ImGui::GetContentRegionAvail().x;
                    float offset = (window_width - text_width) * 0.5f;
                    if (offset > 0) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", placeholder);
                    return;
                }

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.9f, 1.0f, 1.0f));
                //ImGui::Text("Entity ID: %u", static_cast<uint32_t>(selected));
                ImGui::PopStyleColor();

                // Display entity name from metadata
                auto metadata = services->get<MetaData::Service>();
                if (metadata) {
                    std::string entity_name = metadata->getEntityName(selected);

                    ImGui::SameLine(0, 20);

                    // Checkbox
                    static bool checkbox = true;
                    ImGui::PushID("Chkbox");
                    if (ImGui::Checkbox("", &checkbox)) {
                        // logic for checkbox here
                    }
                    ImGui::PopID();
                    ImGui::SameLine(0, 8);

                    // Entity Name
                    char name_buf[128];
                    strncpy(name_buf, entity_name.c_str(), sizeof(name_buf));
                    name_buf[sizeof(name_buf) - 1] = '\0';

                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.7f);
                    if (ImGui::InputText("##entityName", name_buf, sizeof(name_buf), ImGuiInputTextFlags_EnterReturnsTrue)) {
                        std::string new_name(name_buf);
                        if (!new_name.empty() && metadata->isNameValid(new_name)) {
                            metadata->setEntityName(selected, new_name);
                        }
                    }

                    // Tag Dropdown
                    ImGui::Separator();
                    ImGui::Spacing();
                    ImGui::SameLine(0, 20);
                    std::string tag_value = "Untagged";
                    auto curr_tags = metadata->getRegisteredTags();
                    if (!curr_tags.empty()) {
                        std::vector<const char*> tag_items;
                        for (const auto& tag : curr_tags)
                            tag_items.push_back(tag.c_str());

                        for (const auto& tag : curr_tags)
                            if (metadata->hasTag(selected, tag))
                                tag_value = tag;

                        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.3f);
                        if (ImGui::BeginCombo("##TagCombo", tag_value.c_str())) {
                            for (size_t i = 0; i < tag_items.size(); ++i) {
                                bool is_selected = (tag_value == tag_items[i]);
                                if (ImGui::Selectable(tag_items[i], is_selected)) {
                                    metadata->setEntityTag(selected, tag_items[i]);
                                }
                                if (is_selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                    ImGui::SameLine(0, 5);
                    ImGui::Text("Tag");

                    // Layer Dropdown
                    //ImGui::Spacing();
                    ImGui::SameLine(0, 20);
                    static const char* layers[] = { "Default", "Testing2", "Testing3" };
                    static int layer_idx = 0;
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.5f);
                    if (ImGui::Combo("##LayerCombo", &layer_idx, layers, IM_ARRAYSIZE(layers))) {
                        //metadata layer logic
                    }
                    ImGui::SameLine(0, 5);
                    ImGui::Text("Layer");


                    if (metadata->isLocked(selected)) {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "[LOCKED]");
                        ImGui::Separator();
                        ImGui::Spacing();
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.6f, 0.0f, 1.0f));
                        ImGui::Text("Entity is locked");
                        ImGui::PopStyleColor();
                        ImGui::Spacing();
                        ImGui::TextWrapped("Unlock this entity in the Entity Panel to edit its components.");
                        return;
                    }
                }
                ImGui::Spacing();

                renderEntityComponents(selected);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImVec2 button_size = ImVec2(ImGui::GetContentRegionAvail().x, 35);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.8f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.4f, 0.7f, 1.0f));

                if (ImGui::Button("+ Add Component", button_size)) {
                    openPopUp("AddComponent");
                }

                ImGui::PopStyleColor(3);

                // Open remove popup after context menu closes (prevents ImGui state conflicts)
                if (should_open_remove_popup) {
                    openPopUp("RemoveComponent");
                    should_open_remove_popup = false;
                }

                renderPopUps();
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
