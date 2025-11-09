/*****************************************************************//**
 * \file   EntityPanel.cpp
 * \brief  Definition of animation system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "EntityPanel.h"
#include "ECS/Controller.h"
#include "ECS/sMetaData.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cHierarchy.h"

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

#define PN_ECS_SERVICE services->get<ECS::Controller>()
#define PN_METADATA_SERVICE services->get<MetaData::Service>()
#define PN_SERI_SERVICE services->get<Serialization::Service>()


            EntityPanel::EntityPanel() {
                name = "Entity Panel";
                flags = ImGuiWindowFlags_None;

                // Register popups
                registerPopUp("Create Entity", createEntityPopUp("Create Entity"));
                registerPopUp("Remove Entity", removeEntityPopUp("Remove Entity"));
                registerPopUp("Clone Entity", cloneEntityPopUp("Clone Entity"));
                registerPopUp("Error", defPopUp("Error"));
            }

            std::function<void(std::any const&)> EntityPanel::createEntityPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
                    static char entity_name[128] = "";

                    ImGui::Text("Enter a name for the new entity:");
                    if (ImGui::InputText("##Entity Name", entity_name, sizeof(entity_name))) {
                    }

                    if (strlen(entity_name) > 0 && (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                        Action create;
                        std::shared_ptr<std::string> shared_id = std::make_shared<std::string>(entity_name);

                        create.do_action = [&, shared_id]() {
                            auto ecs = services->get<ECS::Controller>();
                            auto scene = services->get<Scene>();

                            glm::vec3 pos = glm::vec3(0.f, 0.f, 0.f);
                            glm::quat rot = { 1.f, 0.f, 0.f, 0.f };
                            glm::vec3 scale = { 1.f, 1.f, 1.f };

                            entt::entity entity = ecs->createEntity();
                            ecs->addEntityComponent(entity, MetaData::EntityName{ *shared_id });
                            ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
                            ecs->addEntityComponent(entity, Hierarchy{});
                            if (scene) {
                                ecs->addEntityComponent(entity, MeshRenderer{ scene->getMeshId("") });
                            }
                            };

                        create.undo_action = [&, shared_id]() {
                            auto entity = PN_METADATA_SERVICE->getEntityByName(*shared_id);
                            if (entity.has_value()) {
                                PN_ECS_SERVICE->destroyEntity(entity.value());
                            }
                            };

                        command_manager->executeAction(std::move(create));
                        entity_name[0] = '\0';
                        closePopUp(popup_id);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        entity_name[0] = '\0';
                        closePopUp(popup_id);
                    }
                    };
            }

            std::function<void(std::any const&)> EntityPanel::removeEntityPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
                    std::string selected_name = PN_ECS_SERVICE->getEntityComponent<MetaData::EntityName>(selected_entity).value().get().name;

                    ImGui::Spacing();
                    ImGui::TextWrapped("Are you sure you want to remove '%s'?", selected_name.c_str());
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Warning: This action cannot be undone!");
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    float button_width = ImGui::GetContentRegionAvail().x * 0.48f;

                    if (ImGui::Button("Remove", ImVec2(button_width, 40))) {
                        Action remove;
                        std::shared_ptr<std::string> shared_id = std::make_shared<std::string>(selected_name);
                        entt::entity entity_to_remove = selected_entity;

                        remove.undo_action = [&, shared_id, entity_to_remove]() {
                            // TODO: Need to use clone
                            };

                        remove.do_action = [&, entity_to_remove]() {
                            if (PN_ECS_SERVICE->checkEntity(entity_to_remove)) {
                                removeEntityWithChildren(entity_to_remove);
                            }
                            selected_entity = entt::null;
                            force_refresh = true;
                            };

                        command_manager->executeAction(std::move(remove));
                        closePopUp(popup_id);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(button_width, 40))) {
                        closePopUp(popup_id);
                    }

                    ImGui::Spacing();
                    };
            }

            std::function<void(std::any const&)> EntityPanel::cloneEntityPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
                    static char entity_name[128] = "";

                    ImGui::Text("Enter a new name for the cloned entity:");
                    if (ImGui::InputText("##Entity Clone Name", entity_name, sizeof(entity_name))) {
                    }

                    if (strlen(entity_name) > 0 && (ImGui::Button("Clone") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                        Action clone;
                        std::shared_ptr<std::string> shared_id = std::make_shared<std::string>(entity_name);
                        entt::entity clone_entity = selected_entity;

                        clone.do_action = [&, shared_id, clone_entity]() {
                            if (PN_ECS_SERVICE->checkEntity(clone_entity)) {
                                entt::entity new_id = PN_ECS_SERVICE->cloneEntity(clone_entity);

                                if (!shared_id->empty() && PN_METADATA_SERVICE->isNameValid(*shared_id)) {
                                    PN_METADATA_SERVICE->setEntityName(new_id, *shared_id);
                                }
                                else {
                                    shared_id->assign(PN_METADATA_SERVICE->getEntityName(new_id));
                                }

                                // Clone children recursively
                                cloneEntityChildren(clone_entity, new_id);
                            }
                            };

                        clone.undo_action = [&, shared_id]() {
                            auto entity = PN_METADATA_SERVICE->getEntityByName(*shared_id);
                            if (entity.has_value()) {
                                removeEntityWithChildren(entity.value());
                            }
                            };

                        command_manager->executeAction(std::move(clone));
                        entity_name[0] = '\0';
                        closePopUp(popup_id);
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Cancel")) {
                        entity_name[0] = '\0';
                        closePopUp(popup_id);
                    }
                    };
            }

            void EntityPanel::ungroupEntity(entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;
                auto& registry = ecs->getRegistry();

                if (!ecs->checkEntity(entity) || !ecs->hasEntityComponent<Hierarchy>(entity)) {
                    return;
                }

                auto hierarchy = ecs->getEntityComponent<Hierarchy>(entity);
                entt::entity parent = entt::null;
                if (hierarchy.has_value()) { entt::entity parent = hierarchy.value().get().parent; }

                // Get all children
                std::vector<entt::entity> children = getEntityChildren(entity);

                // Move each child to this entity's parent (or root)
                for (auto child : children) {
                    if(parent != entt::null) { setEntityParent(child, parent); }
                }

                force_refresh = true;
            }

            void EntityPanel::setSelectedEntity(entt::entity entity) {
                // Only update if the entity is different
                if (selected_entity != entity) {
                    selected_entity = entity;
                    b_entity_changed = true;

                    // Update the selected entity index
                    if (entity != entt::null) {
                        // Find the index of the selected entity in the editor_entities list
                        for (int i = 0; i < editor_entities.size(); ++i) {
                            if (editor_entities[i].first == entity) {
                                selectedEntityIndex = i;
                                break;
                            }
                        }
                    }
                    else {
                        selectedEntityIndex = -1;
                    }
                }
            }

            void EntityPanel::nextWindowSettings() {
            }

            entt::entity EntityPanel::getSelectedEntity() const {
                return selected_entity;
            }

            void EntityPanel::unselectEntity() {
                selected_entity = entt::null;
            }

            bool EntityPanel::isEntityChanged() const {
                return b_entity_changed;
            }

            void EntityPanel::onAttach() {
            }

            void EntityPanel::onUpdate(PAIN::AppTiming timing) {
                auto ecs = PN_ECS_SERVICE;
                auto metadata = PN_METADATA_SERVICE;
                auto ser = services->get<Serialization::Service>();

                // AUTO-ADD HIERARCHY COMPONENT
                auto& registry = ecs->getRegistry();
                auto view_all = registry.view<MetaData::EntityName>();
                for (auto entity : view_all) {
                    if (!registry.all_of<PAIN::Hierarchy>(entity)) {
                        ecs->addEntityComponent(entity, Hierarchy{});
                    }
                }

                // Detect scene changes
                if (ser->consumeSceneChanged()) {
                    selected_entity = entt::null;
                    selectedEntityIndex = -1;
                    editor_entities.clear();
                    total_entities = 0;
                    b_entity_changed = true;
                    force_refresh = true;
                }

                // Detect name changes
                if (metadata->entityNameChanged()) {
                    editor_entities.clear();
                    total_entities = 0;
                    b_entity_changed = true;
                    PN_CORE_INFO("[EntityPanel] Entity Name changed detected, list reset");
                }

                // Get current entity count from ECS
                size_t current_count = static_cast<size_t>(ecs->getEntitiesCount());

                // Rebuild entity list if needed
                if (total_entities != current_count || force_refresh) {
                    total_entities = current_count;
                    editor_entities.clear();
                    force_refresh = false;

                    auto view = registry.view<MetaData::EntityName>();
                    for (auto entity : view) {
                        auto& name_comp = view.get<MetaData::EntityName>(entity);
                        editor_entities.push_back({
                            entity,
                            name_comp.name
                            });
                    }

                    if (editor_entities.size() != total_entities) {
                        PN_CORE_WARN("[EntityPanel] Entity count mismatch: displayed={}, total={}",
                            editor_entities.size(), total_entities);
                    }
                }

                // Begin ImGui window
                ImGui::Begin(name.c_str());
                dock_id = ImGui::GetWindowDockID();
                ImGui::Spacing();

                // Display entity count
                ImGui::Text("Number of entities in level: %d", total_entities);
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Drag entities to group them");
                ImGui::Spacing();

                // Sort button
                if (ImGui::Checkbox("Sort A-Z", &sort_alphabetically)) {
                    force_refresh = true;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Sort entities alphabetically");
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Get root entities (entities without parents)
                std::vector<entt::entity> root_entities;
                for (const auto& [entity_id, name] : editor_entities) {
                    if (ecs->hasEntityComponent<Hierarchy>(entity_id)) {
                        auto hierarchy = ecs->getEntityComponent<Hierarchy>(entity_id);
                        if (hierarchy.has_value()) {
                            if (hierarchy.value().get().parent == entt::null) {
                                root_entities.push_back(entity_id);
                            }
                        }

                    }
                    else {
                        root_entities.push_back(entity_id);
                    }
                }

                // Sort root entities if enabled
                if (sort_alphabetically) {
                    std::sort(root_entities.begin(), root_entities.end(), [&](entt::entity a, entt::entity b) {
                        std::string nameA = "Unnamed";
                        std::string nameB = "Unnamed";

                        auto name_comp_a = ecs->getEntityComponent<MetaData::EntityName>(a);
                        if (name_comp_a.has_value()) {
                            nameA = name_comp_a.value().get().name;
                        }

                        auto name_comp_b = ecs->getEntityComponent<MetaData::EntityName>(b);
                        if (name_comp_b.has_value()) {
                            nameB = name_comp_b.value().get().name;
                        }

                        return nameA < nameB;
                        });
                }

                // Render hierarchy starting from root entities
                for (auto entity : root_entities) {
                    drawEntityHierarchy(entity, 0);
                }

                ImGui::Spacing();
                ImGui::Separator();

                // Add Entity button
                if (ImGui::Button("Add Entity", ImVec2(-1, 0))) {
                    openPopUp("Create Entity");
                }

                // Show Ungroup button when entity with children is selected
                if (ecs->checkEntity(selected_entity)) {
                    std::vector<entt::entity> children = getEntityChildren(selected_entity);
                    if (!children.empty()) {
                        ImGui::Spacing();
                        ImGui::Separator();
                        ImGui::Spacing();

                        if (ImGui::Button("Ungroup Children", ImVec2(-1, 0))) {
                            ungroupEntity(selected_entity);
                        }
                    }
                }

                // Render any open popups
                renderPopUps();

                ImGui::End();

                // Reset entity changed flag
                if (b_entity_changed) {
                    b_entity_changed = false;
                }
            }

            void EntityPanel::drawEntityHierarchy(entt::entity entity_id, int depth) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->checkEntity(entity_id)) {
                    return;
                }

                // Get entity name
                std::string entity_name = "Unnamed";
                auto name_comp = ecs->getEntityComponent<MetaData::EntityName>(entity_id);
                if (name_comp.has_value()) {
                    entity_name = name_comp.value().get().name;
                }

                bool is_selected = (selected_entity == entity_id);
                std::vector<entt::entity> children = getEntityChildren(entity_id);
                bool has_children = !children.empty();

                // Create indentation based on depth
                std::string indent = "";
                for (int i = 0; i < depth; i++) {
                    indent += "  ";  // Two spaces per level
                }

                // Add visual hierarchy indicators
                std::string prefix = "";
                if (depth > 0) {
                    prefix = "└─ ";  // Child indicator
                }
                if (has_children) {
                    prefix += "[G] ";  // Group indicator
                }

                std::string display_label = indent + prefix + entity_name;
                std::string unique_label = display_label + "##" + std::to_string(static_cast<uint32_t>(entity_id));

                // Apply different color for child entities
                if (depth > 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                }



                // Render selectable
                if (ImGui::Selectable(unique_label.c_str(), is_selected)) {
                    if (is_selected) {
                        selected_entity = entt::null;
                        selectedEntityIndex = -1;
                    }
                    else {
                        selected_entity = entity_id;
                        selectedEntityIndex = -1;
                    }
                    b_entity_changed = true;
                }

                if (depth > 0) {
                    ImGui::PopStyleColor();
                }

                // DRAG SOURCE - Start dragging this entity
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    ImGui::SetDragDropPayload("ENTITY_DRAG", &entity_id, sizeof(entt::entity));
                    ImGui::Text("Dragging: %s", entity_name.c_str());
                    ImGui::EndDragDropSource();
                }

                // DROP TARGET - Drop onto this entity to make it the parent
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG")) {
                        entt::entity dragged_entity = *(const entt::entity*)payload->Data;

                        // Don't allow entity to be parented to itself
                        if (dragged_entity != entity_id) {
                            // Check if target is not already a child of the dragged entity (prevent circular parenting)
                            if (!isAncestor(dragged_entity, entity_id)) {
                                PN_CORE_INFO("[EntityPanel] Parenting entity {} to {}",
                                    static_cast<uint32_t>(dragged_entity),
                                    static_cast<uint32_t>(entity_id));

                                setEntityParent(dragged_entity, entity_id);
                                force_refresh = true;
                            }
                            else {
                                PN_CORE_WARN("[EntityPanel] Cannot parent entity to its own descendant");
                            }
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // RIGHT-CLICK CONTEXT MENU
                if (ImGui::BeginPopupContextItem()) {
                    selected_entity = entity_id;
                    b_entity_changed = true;

                    if (ImGui::MenuItem("Clone")) {
                        PN_CORE_INFO("[EntityPanel] Cloning entity {}", static_cast<uint32_t>(entity_id));

                        // Clone the entity
                        entt::entity new_id = ecs->cloneEntity(entity_id);

                        // Generate a unique name for the clone
                        std::string original_name = entity_name;
                        std::string clone_name = original_name + " (Clone)";
                        int counter = 1;

                        while (!PN_METADATA_SERVICE->isNameValid(clone_name)) {
                            clone_name = original_name + " (Clone " + std::to_string(counter++) + ")";
                        }

                        PN_METADATA_SERVICE->setEntityName(new_id, clone_name);

                        // Clone children recursively
                        cloneEntityChildren(entity_id, new_id);

                        force_refresh = true;
                        ImGui::CloseCurrentPopup();
                    }

                    if (ImGui::MenuItem("Create Prefab")) {
                        PN_CORE_INFO("[EntityPanel] Creating prefab from entity {}", static_cast<uint32_t>(entity_id));

                        // Generate unique prefab name
                        std::string prefab_name = generateUniquePrefabName(entity_name);

                        // Collect entity and children
                        std::vector<entt::entity> entities_to_save;
                        collectEntityHierarchy(entity_id, entities_to_save);

                        // Save prefab (serialization service handles the rest!)
                        PN_SERI_SERVICE->savePrefabToFile(prefab_name, entities_to_save);

                        PN_CORE_INFO("[EntityPanel] Created prefab '{}' with {} entities",
                            prefab_name, entities_to_save.size());

                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::Separator();

                    if (has_children) {
                        if (ImGui::MenuItem("Ungroup")) {
                            PN_CORE_INFO("[EntityPanel] Ungrouping entity {}", static_cast<uint32_t>(entity_id));
                            ungroupEntity(entity_id);
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    if (ecs->hasEntityComponent<Hierarchy>(entity_id)) {
                        auto hierarchy = ecs->getEntityComponent<Hierarchy>(entity_id);
                        if (hierarchy.has_value()) {
                            if (hierarchy.value().get().parent != entt::null) {
                                if (ImGui::MenuItem("Move to Root")) {
                                    PN_CORE_INFO("[EntityPanel] Moving entity {} to root", static_cast<uint32_t>(entity_id));
                                    setEntityParent(entity_id, entt::null);
                                    force_refresh = true;
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                        }

                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Delete", "Del")) {
                        PN_CORE_INFO("[EntityPanel] Deleting entity {}", static_cast<uint32_t>(entity_id));
                        removeEntityWithChildren(entity_id);
                        if (selected_entity == entity_id) {
                            selected_entity = entt::null;
                        }
                        force_refresh = true;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }

                // Recursively draw children with increased depth
                if (has_children) {
                    for (auto child : children) {
                        drawEntityHierarchy(child, depth + 1);
                    }
                }
            }

            // Helper functions
            std::vector<entt::entity> EntityPanel::getEntityChildren(entt::entity parent) {
                std::vector<entt::entity> children;
                auto ecs = PN_ECS_SERVICE;
                auto& registry = ecs->getRegistry();

                auto view = registry.view<Hierarchy>();
                for (auto entity : view) {
                    auto& hierarchy = view.get<Hierarchy>(entity);
                    if (hierarchy.parent == parent) {
                        children.push_back(entity);
                    }
                }

                return children;
            }

            void EntityPanel::setEntityParent(entt::entity child, entt::entity parent) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->hasEntityComponent<Hierarchy>(child)) {
                    ecs->addEntityComponent(child, Hierarchy{});
                }

                auto child_hierarchy = ecs->getEntityComponent<Hierarchy>(child);

                if (child_hierarchy.has_value()) {
                    // Remove from old parent
                    if (child_hierarchy.value().get().parent != entt::null) {
                        if (ecs->hasEntityComponent<Hierarchy>(child_hierarchy.value().get().parent)) {
                            auto old_parent_hierarchy = ecs->getEntityComponent<Hierarchy>(child_hierarchy.value().get().parent);
                            if (old_parent_hierarchy.has_value()) {
                                old_parent_hierarchy.value().get().children.erase(
                                    std::remove(old_parent_hierarchy.value().get().children.begin(),
                                        old_parent_hierarchy.value().get().children.end(), child),
                                    old_parent_hierarchy.value().get().children.end()
                                );
                            }

                        }
                    }
                }



                // Set new parent
                if (child_hierarchy.has_value()) {
                    child_hierarchy.value().get().parent = parent;
                }

                // Add to new parent's children
                if (parent != entt::null) {
                    if (!ecs->hasEntityComponent<Hierarchy>(parent)) {
                        ecs->addEntityComponent(parent, Hierarchy{});
                    }
                    auto parent_hierarchy = ecs->getEntityComponent<Hierarchy>(parent);
                    if(parent_hierarchy.has_value()) { parent_hierarchy.value().get().children.push_back(child); }

                }
            }

            bool EntityPanel::isAncestor(entt::entity potential_ancestor, entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;

                // Check if initial entity has hierarchy component
                auto hierarchy_opt = ecs->getEntityComponent<Hierarchy>(entity);
                if (!hierarchy_opt) {
                    return false;  // No hierarchy component
                }

                // Track visited entities to prevent cycles
                std::unordered_set<entt::entity> visited;
                visited.insert(entity);

                // Get parent from the optional safely
                entt::entity current_parent = hierarchy_opt->get().parent;

                // Traverse up the hierarchy
                constexpr size_t MAX_DEPTH = 100;
                size_t depth = 0;

                while (current_parent != entt::null && depth++ < MAX_DEPTH) {
                    // Check for cycles
                    if (visited.count(current_parent) > 0) {
                        PN_CORE_ERROR("Cycle detected in hierarchy for entity {}",
                            static_cast<uint32_t>(entity));
                        return false;
                    }
                    visited.insert(current_parent);

                    // Found the ancestor
                    if (current_parent == potential_ancestor) {
                        return true;
                    }

                    // Get parent's hierarchy component
                    auto parent_hierarchy_opt = ecs->getEntityComponent<Hierarchy>(current_parent);
                    if (!parent_hierarchy_opt) {
                        // Parent has no hierarchy component - end of chain
                        break;
                    }

                    // Move to next parent
                    current_parent = parent_hierarchy_opt->get().parent;
                }

                return false;
            }


            void EntityPanel::removeEntityWithChildren(entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->checkEntity(entity)) {
                    return;
                }

                // Get all children first
                std::vector<entt::entity> children = getEntityChildren(entity);

                // Recursively remove children
                for (auto child : children) {
                    removeEntityWithChildren(child);
                }

                // Remove from parent's children list
                if (ecs->hasEntityComponent<Hierarchy>(entity)) {
                    auto hierarchy_opt = ecs->getEntityComponent<Hierarchy>(entity);
                    if (hierarchy_opt.has_value()) {
                        auto& hierarchy = hierarchy_opt.value().get();
                        if (hierarchy.parent != entt::null && ecs->checkEntity(hierarchy.parent)) {
                            auto parent_hierarchy_opt = ecs->getEntityComponent<Hierarchy>(hierarchy.parent);
                            if (parent_hierarchy_opt.has_value()) {
                                auto& parent_hierarchy = parent_hierarchy_opt.value().get();
                                parent_hierarchy.children.erase(
                                    std::remove(parent_hierarchy.children.begin(),
                                        parent_hierarchy.children.end(), entity),
                                    parent_hierarchy.children.end()
                                );
                            }
                        }
                    }
                }

                // Destroy the entity
                ecs->destroyEntity(entity);
            }

            void EntityPanel::cloneEntityChildren(entt::entity source, entt::entity cloned_parent) {
                auto children = getEntityChildren(source);
                auto ecs = PN_ECS_SERVICE;

                for (auto child : children) {
                    entt::entity cloned_child = ecs->cloneEntity(child);
                    setEntityParent(cloned_child, cloned_parent);

                    // Recursively clone grandchildren
                    cloneEntityChildren(child, cloned_child);
                }
            }

            /**********************************
            * Prefab Helper functions
            ******************************/

            // Generate unique prefab filename (without path, just name)
            std::string EntityPanel::generateUniquePrefabName(const std::string& base_name) {
                auto seri_service = services->get<PAIN::Serialization::Service>();

                std::string prefab_name = base_name;
                int counter = 1;

                std::string test_path = seri_service->resolvePrefabPath(prefab_name);

                while (std::filesystem::exists(test_path)) {
                    prefab_name = base_name + " (" + std::to_string(counter++) + ")";
                    test_path = seri_service->resolvePrefabPath(prefab_name);
                }
                
                return prefab_name;
            }

            // Recursively collect entity and all children
            void EntityPanel::collectEntityHierarchy(entt::entity entity, std::vector<entt::entity>& out_entities) {
                out_entities.push_back(entity);

                // Get children safely
                auto children = getEntityChildren(entity);
                for (auto child : children) {
                    collectEntityHierarchy(child, out_entities);
                }
            }


        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
#endif