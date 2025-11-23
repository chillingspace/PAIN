#include "pch.h"
#include "ComponentsPanel.h"
#include "EntityPanel.h"
#include "ResourcePanel.h"
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

// ---- Transform ---- (FIXED: Skip detection after undo/redo)
                registerCompUIFunc<PAIN::Transform>("Transform",
                    [this](ComponentsPanel& panel, PAIN::Transform& transform_ref) {
                        static struct {
                            entt::entity entity = entt::null;
                            Transform original_transform;
                            Transform last_frame_transform;
                            bool is_editing = false;
                            int skip_frames = 0;  // NEW: Skip detection for N frames
                        } state;

                        auto entity_panel = entities_panel.lock();
                        if (!entity_panel) {
                            DrawWithReflection(transform_ref);
                            return;
                        }

                        entt::entity selected = entity_panel->getSelectedEntity();
                        if (selected == entt::null) {
                            DrawWithReflection(transform_ref);
                            return;
                        }

                        // NEW: Skip detection if undo/redo is executing
                        if (command_manager && command_manager->isExecutingUndoRedo()) {
                            state.skip_frames = 2;  // Skip next 2 frames
                            state.is_editing = false;
                            DrawWithReflection(transform_ref);
                            return;
                        }

                        // NEW: Decrement skip counter
                        if (state.skip_frames > 0) {
                            state.skip_frames--;
                            DrawWithReflection(transform_ref);
                            return;
                        }

                        // Start tracking when any ImGui item becomes active
                        if (ImGui::IsAnyItemActive() && !state.is_editing) {
                            state.entity = selected;
                            state.original_transform = transform_ref;
                            state.last_frame_transform = transform_ref;
                            state.is_editing = true;
                        }

                        // Draw the reflection UI
                        DrawWithReflection(transform_ref);

                        // Detect if transform changed this frame
                        if (state.is_editing) {
                            if (state.last_frame_transform.position != transform_ref.position ||
                                state.last_frame_transform.rotation != transform_ref.rotation ||
                                state.last_frame_transform.scale != transform_ref.scale) {
                                state.last_frame_transform = transform_ref;
                            }
                        }

                        // When user stops editing
                        if (state.is_editing && !ImGui::IsAnyItemActive() && !ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                            // Check if anything actually changed
                            if (state.original_transform.position != transform_ref.position ||
                                state.original_transform.rotation != transform_ref.rotation ||
                                state.original_transform.scale != transform_ref.scale) {

                                // Create undo/redo action
                                Transform final_transform = transform_ref;
                                Transform old_transform = state.original_transform;
                                entt::entity entity = selected;

                                auto ecs = services->get<ECS::Controller>();
                                auto metadata = services->get<MetaData::Service>();

                                std::string entity_name = "Entity";
                                if (metadata) {
                                    entity_name = metadata->getEntityName(entity);
                                }

                                command_manager->executeAction(Action{
                                    [ecs, entity, final_transform]() {
                                        if (ecs->checkEntity(entity)) {
                                            auto transform_opt = ecs->getEntityComponent<Transform>(entity);
                                            if (transform_opt.has_value()) {
                                                transform_opt.value().get() = final_transform;
                                            }
                                        }
                                    },
                                    [ecs, entity, old_transform]() {
                                        if (ecs->checkEntity(entity)) {
                                            auto transform_opt = ecs->getEntityComponent<Transform>(entity);
                                            if (transform_opt.has_value()) {
                                                transform_opt.value().get() = old_transform;
                                            }
                                        }
                                    },
                                    "Modify Transform: " + entity_name
                                    });
                            }

                            // Reset editing state
                            state.is_editing = false;
                        }
                    });

                // ---- ModelRenderer ---- (UNCHANGED)
                    registerCompUIFunc<PAIN::ModelRenderer>("ModelRenderer",
                        [this](ComponentsPanel& panel, PAIN::ModelRenderer& renderer) {
                            // Model GUID selector (using reflection)
                            bool changed = false;

                            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

                            // Model Asset Selection
                            if (DrawAssetSelectorField("Select A Model",
                                renderer.modelGUID,
                                PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Model),
                                panel)) {
                                changed = true;
                            }

                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();

                            // Rendering Options
                            if (ImGui::CollapsingHeader("Rendering Options")) {
                                ImGui::Indent(10.0f);
                                changed |= ImGui::Checkbox("Visible", &renderer.visible);
                                changed |= ImGui::Checkbox("Cast Shadows", &renderer.castShadows);
                                changed |= ImGui::Checkbox("Receive Shadows", &renderer.receiveShadows);
                                ImGui::Unindent(10.0f);
                            }

                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::Spacing();

                            // MATERIALS SECTION - This is where the magic happens!
                            if (DrawField("Materials", renderer.materials, &panel)) {
                                changed = true;
                            }

                            ImGui::PopStyleVar();

                            // Optional: Add animation info if present
                            if (renderer.currentAnimationIndex >= 0) {
                                ImGui::Spacing();
                                ImGui::Separator();
                                ImGui::Spacing();

                                if (ImGui::CollapsingHeader("Animation (Debug Info)")) {
                                    ImGui::BeginDisabled();
                                    ImGui::Text("Current Animation: %d", renderer.currentAnimationIndex);
                                    ImGui::Text("Animation Time: %.2f", renderer.animationTime);
                                    ImGui::Text("Is Playing: %s", renderer.isPlaying ? "Yes" : "No");
                                    ImGui::EndDisabled();
                                }
                            }
                        });

                // ---- Light ---- (UNCHANGED)
                registerCompUIFunc<PAIN::Lighting>("Lighting",
                    [](ComponentsPanel&, PAIN::Lighting& as) { DrawWithReflection(as); });

                // ---- AudioSource ---- (UNCHANGED)
                registerCompUIFunc<PAIN::Audio::AudioSource>("AudioSource",
                    [this](ComponentsPanel&, PAIN::Audio::AudioSource& as) { DrawWithReflection(as, static_cast<ComponentsPanel*>(this)); });

                // ---- BoundingVolume ---- (UNCHANGED)
                registerCompUIFunc<PAIN::BoundingVolume>("BoundingVolume",
                    [](ComponentsPanel&, PAIN::BoundingVolume& as) { DrawWithReflection(as); });

                // ---- Hierarchy ---- (UNCHANGED)
                registerCompUIFunc<PAIN::Hierarchy>("Hierarchy",
                    [](ComponentsPanel&, PAIN::Hierarchy& as) { DrawWithReflection(as); });

                // ---- Physics ---- (UNCHANGED)
                registerCompUIFunc<PAIN::Joint>("Joint",
                    [](ComponentsPanel&, PAIN::Joint& as) { DrawWithReflection(as); });

                registerCompUIFunc<Physics::RigidBody3D>("RigidBody3D",
                    [](ComponentsPanel&, Physics::RigidBody3D& rb) { DrawWithReflection(rb); });

                // ---- Script ---- (UNCHANGED)
                registerCompUIFunc<PAIN::Script>("Script",
                    [this](ComponentsPanel&, PAIN::Script& as) { DrawWithReflection(as, static_cast<ComponentsPanel*>(this)); });

                PAIN::Editor::Panel::RegisterColliderUI(*this);

                // Get entity panel reference
                auto editor = services->get<PAIN::Editor::Editor>();
                if (editor) {
                    entities_panel = editor->getPanel<EntityPanel>();
                    resources_panel = editor->getPanel<ResourcePanel>();
                }

                // Register Add Component popup
                registerPopUp("AddComponent", addComponentPopUp("AddComponent"));


                // Register Remove Component popup
                registerPopUp("RemoveComponent", removeComponentPopUp("RemoveComponent"));

				// Register RigidBody3D Config popup
                registerPopUp("AddRigidBody3DConfig", addRigidBodyConfigPopUp("AddRigidBody3DConfig"));
            }



            void ComponentsPanel::nextWindowSettings() {
                ImGui::SetNextWindowSize(ImVec2(350, 600), ImGuiCond_FirstUseEver);
            }

            std::function<void(std::any const&)> ComponentsPanel::addComponentPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
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
                            if (comp_name == "RigidBody3D") {
                                openPopUp("AddRigidBody3DConfig");
                                search_filter[0] = '\0';
                                closePopUp(popup_id);
                            }
                            else {
                                ecs->addComponentByName(selected_entity, comp_name);
                                search_filter[0] = '\0';
                                closePopUp(popup_id);
                            }
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


            std::function<void(std::any const&)> ComponentsPanel::removeComponentPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
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

            std::function<void(std::any const&)> ComponentsPanel::addRigidBodyConfigPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
                    auto ecs = services->get<ECS::Controller>();
                    auto entity_panel = entities_panel.lock();
                    if (!entity_panel) return;

                    entt::entity selected_entity = entity_panel->getSelectedEntity();
                    if (!ecs->checkEntity(selected_entity)) return;

                    static int motion_type_idx = 1; // Default to Dynamic
                    const char* motion_names[] = { "Static", "Dynamic", "Kinematic" };

                    ImGui::Text("Select Motion Type:");
                    ImGui::Combo("Motion Type", &motion_type_idx, motion_names, IM_ARRAYSIZE(motion_names));

                    ImGui::Spacing();
                    if (ImGui::Button("Add RigidBody3D", ImVec2(-1, 0))) {
                        // Add the component

                        if (!ecs->hasComponentByName(selected_entity, "RigidBody3D")) {
                            Physics::RigidBody3D rb;
                            rb.motion_type = static_cast<PAIN::Physics::MotionType>(motion_type_idx);
                            ecs->addEntityComponent<PAIN::Physics::RigidBody3D>(selected_entity, std::move(rb));
                        }
                        closePopUp(popup_id);
                    }
                    if (ImGui::Button("Cancel", ImVec2(-1, 0))) {
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

                // if lua script active.

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

                        //Start drag-and-drop source
                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {

                            //Set drag payload with asset name
                            ImGui::SetDragDropPayload(std::string(comp_name + "_COMP").c_str(), &comp_ptr, sizeof(comp_ptr));

                            //Render the icon or name at the cursor during dragging
                            ImGui::Text("%s", comp_name.c_str());
                            ImGui::EndDragDropSource();
                        }

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

            void ComponentsPanel::setScriptChanged(bool is_script_changed) {
                is_script_loaded = is_script_changed;
            }

            bool ComponentsPanel::getScriptChanged() {
                return is_script_loaded;
            }


            void ComponentsPanel::setScriptSaved(bool is_script_saved_) {
                is_script_saved = is_script_saved_;
            }

            bool ComponentsPanel::getScriptSaved() {
                return is_script_saved;
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

                // Ensure resource panel reference is valid
                auto resource_panel = resources_panel.lock();
                if (!resource_panel) {
                    auto editor = services->get<PAIN::Editor::Editor>();
                    if (editor) {
                        auto rp = editor->getPanel<Panel::ResourcePanel>();
                        if (rp) {
                            resources_panel = rp;
                            resource_panel = rp;
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
                auto selected_filepath = resource_panel->getSelectedFilePath();

                if (entity_panel->isEntityAndScriptSwitched()) {
                    resource_panel->setSelectedFilePath("");
                    entity_panel->setEntityAndScriptSwitched(false);
                }

                // Lua Script Display
                if (!selected_filepath.empty()) {

                    static char script_buffer[65536] = "";

                    if (!getScriptChanged()) {
                        std::ifstream file(selected_filepath, std::ios::in | std::ios::binary);
                        if (file) {
                            file.read(script_buffer, sizeof(script_buffer) - 1);
                            std::streamsize count = file.gcount();
                            script_buffer[count] = '\0';
                            file.close();
                        }
                        else {
                            script_buffer[0] = '\0';
                            
                            PN_CORE_WARN("Failed to open file: ", selected_filepath);
                        }
                        setScriptChanged(true);
                    }

                    // Editable text Input
                    if (ImGui::InputTextMultiline("##Script", script_buffer, sizeof(script_buffer),
                        ImVec2(-1.0f, 400), ImGuiInputTextFlags_AllowTabInput)) {
                        setScriptSaved(false);
                    }

                    // Save button (TODO: Check if it updates real time.)
                    if (!getScriptSaved()) {
                        if (ImGui::Button("Save Script")) {
                            std::ofstream file(selected_filepath, std::ios::out | std::ios::binary);
                            if (file) {
                                file.write(script_buffer, strlen(script_buffer));
                                file.close();
                                setScriptSaved(true);
                            }
                            else {
                                PN_CORE_WARN("Failed to save file: ", selected_filepath);
                            }
                        }
                    }
                    else {
                        ImGui::BeginDisabled();
                        ImGui::Button("Save Script");
                        ImGui::EndDisabled();
                    }

                    ImGui::SameLine();

                    // Open in Visual Studio Code button
                    if (ImGui::Button("Open in VS Code")) {
                        std::string command = "code \"" + selected_filepath + "\"";
                        system(command.c_str());
                    }

                    return;
                }

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
