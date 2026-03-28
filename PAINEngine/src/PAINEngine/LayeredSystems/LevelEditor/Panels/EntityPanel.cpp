/*****************************************************************//**
 * \file   EntityPanel.cpp
 * \brief  Entity hierarchy panel with GUID-based parenting
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   November 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "../Editor.h"
#include "EntityPanel.h"
#include "ECS/Controller.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cEntity.h"
#include "Systems/Transform/sysTransform.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "CoreSystems/Prefabs/sPrefab.h"
#include "CoreSystems/EntityTemplate/sEntityTemplate.h"

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

#define PN_ECS_SERVICE services->get<ECS::Controller>()
#define PN_SERI_SERVICE services->get<Serialization::Service>()

            EntityPanel::EntityPanel() {
                name = "Entity Panel";
                flags = ImGuiWindowFlags_None;

                registerPopUp("Create Entity", createEntityPopUp("Create Entity"));
                registerPopUp("Remove Entity", removeEntityPopUp("Remove Entity"));
                registerPopUp("Clone Entity", cloneEntityPopUp("Clone Entity"));
                registerPopUp("Error", defPopUp("Error"));
            }

            std::function<void(std::any const&)> EntityPanel::createEntityPopUp(std::string const& popup_id) {
                return [this, popup_id](std::any const& data) {
                    static char entity_name[128] = "";

                    ImGui::Text("Enter a name for the new entity:");
                    ImGui::InputText("##Entity Name", entity_name, sizeof(entity_name));

                    if (strlen(entity_name) > 0 && (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                        std::string final_name = std::string(entity_name);

                        command_manager->executeAction(Action{
                            [this, final_name]() {
                                auto ecs = PN_ECS_SERVICE;
                                auto scene = services->get<Scene::SceneManager>();

                                entt::entity entity = ecs->createEntity(currentRegistryID); // Auto-assigns GUID

                                // Add core components
                                ecs->addEntityComponent(entity, Entity::Name{ final_name }, currentRegistryID);
                                ecs->addEntityComponent(entity, LocalTransform{}, currentRegistryID);
                                ecs->addEntityComponent(entity, WorldTransform{}, currentRegistryID);

                                std::vector<entt::entity> currentRoots = getRootEntities(); // Or get siblings if parenting
                                int newIndex = static_cast<int>(currentRoots.size());

                                if (auto h = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID)) {
                                    h->get().siblingIndex = newIndex;
                                }

                                // Optional: Add default model
                                //if (scene) {
                                //    auto models = services->get<Assets::Manager>()->getAllAssetDataOfType(Assets::Type::Model);
                                //    if (!models.empty()) {
                                //        auto asset_service = services->get<Assets::Manager>();

                                //        //Default obj paths
                                //        std::filesystem::path quad_path = "engine\\models\\quad.obj";
                                //        std::filesystem::path sphere_path = "engine\\models\\sphere.obj";

                                //        //3 Fallbacks incase
                                //        Assets::GUID base_model_id = asset_service->findGUID(quad_path);
                                //        if (!base_model_id.IsValid()) {
                                //            base_model_id = asset_service->findGUID(sphere_path);
                                //        }
                                //        if (!base_model_id.IsValid()) {
                                //            base_model_id = models.front()->guid;
                                //        }

                                //        //Add default model renderer
                                //        ecs->addEntityComponent(entity, ModelRenderer{ base_model_id }, currentRegistryID);
                                //    }
                                //}

                                // Mark transform dirty
                                auto transformSystem = ecs->getSystem<Transform::System>();
                                if (transformSystem) {
                                    transformSystem->markDirty(entity, ecs->getRegistry(currentRegistryID));
                                }
                            },
                            [this, final_name]() {
                                // Undo: find and delete by name
                                auto ecs = PN_ECS_SERVICE;
                                auto& registry = ecs->getRegistry(currentRegistryID);
                                auto view = registry.view<Entity::Name>();

                                for (auto entity : view) {
                                    if (view.get<Entity::Name>(entity).name == final_name) {
                                        ecs->destroyEntity(entity, currentRegistryID);
                                        break;
                                    }
                                }
                            },
                            "Create Entity: " + final_name
                            });

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
                    auto ecs = PN_ECS_SERVICE;
                    std::string selected_name = getEntityName(selected_entity);

                    ImGui::Spacing();
                    ImGui::TextWrapped("Are you sure you want to remove '%s'?", selected_name.c_str());
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Warning: This action cannot be undone!");
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    float button_width = ImGui::GetContentRegionAvail().x * 0.48f;

                    if (ImGui::Button("Remove", ImVec2(button_width, 40))) {
                        entt::entity entity_to_remove = selected_entity;
                        std::string entity_name = selected_name;

                        command_manager->executeAction(Action{
                            [this, entity_to_remove]() {
                                auto ecs = PN_ECS_SERVICE;
                                if (ecs->checkEntity(entity_to_remove, currentRegistryID)) {
                                    removeEntityWithChildren(entity_to_remove);
                                }
                                selected_entity = entt::null;
                                force_refresh = true;
                            },
                            []() {
                                PN_CORE_WARN("Undo for entity deletion not yet implemented");
                            },
                            "Delete Entity: " + entity_name
                            });

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

                    ImGui::Text("Enter a name for the cloned entity:");
                    ImGui::InputText("##Entity Clone Name", entity_name, sizeof(entity_name));

                    if (strlen(entity_name) > 0 && (ImGui::Button("Clone") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                        std::string final_name = std::string(entity_name);
                        entt::entity clone_source = selected_entity;

                        command_manager->executeAction(Action{
                            [this, final_name, clone_source]() {
                                auto ecs = PN_ECS_SERVICE;

                                if (ecs->checkEntity(clone_source, currentRegistryID)) {
                                    entt::entity new_entity = ecs->cloneEntity(clone_source, currentRegistryID, currentRegistryID);

                                    // Assign sibling index
                                    auto hNew = ecs->getEntityComponent<Entity::Hierarchy>(new_entity, currentRegistryID);
                                    if (hNew) {
                                        hNew->get().childrenGUIDs.clear();
                                        Assets::GUID parentGUID = hNew->get().parentGUID;
                                        if (parentGUID.IsValid()) {
                                            // If its a parent clone
                                            entt::entity parent = ecs->resolveGUID(parentGUID, currentRegistryID);                                         
                                            setEntityParent(new_entity, parent);
                                        }
                                        else {
                                            // If its a root clone
                                            hNew->get().parentGUID = Assets::GUID();
                                        }
                                    }

                                    // Set name
                                    if (auto name_comp = ecs->getEntityComponent<Entity::Name>(new_entity, currentRegistryID)) {
                                        name_comp.value().get().name = final_name;
                                    }

                                    // Clone children recursively
                                    cloneEntityWithChildren(clone_source, new_entity);

                                    // Set a sibling index
                                    std::vector<entt::entity> siblings = getSiblings(clone_source);

                                    // Remove the new clone from wherever it is in the list
                                    siblings.erase(std::remove(siblings.begin(), siblings.end(), new_entity), siblings.end());

                                    // Sort the remaining list to restore the "Pre-Clone" order
                                    std::stable_sort(siblings.begin(), siblings.end(), [&](entt::entity a, entt::entity b) {
                                        auto hA = ecs->getEntityComponent<Entity::Hierarchy>(a, currentRegistryID);
                                        auto hB = ecs->getEntityComponent<Entity::Hierarchy>(b, currentRegistryID);
                                        int idxA = hA ? hA->get().siblingIndex : 0;
                                        int idxB = hB ? hB->get().siblingIndex : 0;
                                        if (idxA != idxB) return idxA < idxB;
                                        return a < b;
                                        });

                                    // Find Original and Insert Clone After it
                                    auto it = std::find(siblings.begin(), siblings.end(), clone_source);
                                    if (it != siblings.end()) {
                                        siblings.insert(it + 1, new_entity);
                                    }
                                    else {
                                        siblings.push_back(new_entity);
                                    }

                                    // Manually Stamp Indices (0, 1, 2, 3...) based on this exact vector order
                                    for (int i = 0; i < siblings.size(); i++) {
                                        if (auto h = ecs->getEntityComponent<Entity::Hierarchy>(siblings[i], currentRegistryID)) {
                                            h->get().siblingIndex = i;
                                        }
                                    }


                                    // Mark dirty
                                    auto transformSystem = ecs->getSystem<Transform::System>();
                                    if (transformSystem) {
                                        transformSystem->markDirty(new_entity, ecs->getRegistry(currentRegistryID));
                                    }


                                }
                            },
                            [this, final_name]() {
                                auto ecs = PN_ECS_SERVICE;
                                auto& registry = ecs->getRegistry(currentRegistryID);
                                auto view = registry.view<Entity::Name>();

                                for (auto entity : view) {
                                    if (view.get<Entity::Name>(entity).name == final_name) {
                                        removeEntityWithChildren(entity);
                                        break;
                                    }
                                }
                            },
                            "Clone Entity: " + final_name
                            });

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

            void EntityPanel::setSelectedEntity(entt::entity entity) {
                if (selected_entity != entity) {
                    selected_entity = entity;
                    b_entity_changed = true;

                    if (entity != entt::null) {
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

            entt::entity EntityPanel::getSelectedEntity() const {
                return selected_entity;
            }

            void EntityPanel::unselectEntity() {
                selected_entity = entt::null;
            }

            bool EntityPanel::isEntityChanged() const {
                return b_entity_changed;
            }

            bool EntityPanel::isEntityAndScriptSwitched() const {
                return b_entity_script_switched;
            }

            void EntityPanel::setEntityAndScriptSwitched(bool is_switched) {
                b_entity_script_switched = is_switched;
            }

            void EntityPanel::onAttach() {
            }

            void EntityPanel::onUpdate(PAIN::AppTiming timing) {
                auto ecs = PN_ECS_SERVICE;
                auto ser = PN_SERI_SERVICE;
                auto& registry = ecs->getRegistry(currentRegistryID);

                // Auto-add required components to all entities
                auto view_all = registry.view<Entity::Name>();
                for (auto entity : view_all) {
                    if (!registry.all_of<Entity::Hierarchy>(entity)) {
                        ecs->addEntityComponent(entity, Entity::Hierarchy{}, currentRegistryID);
                    }
                    if (!registry.all_of<LocalTransform>(entity)) {
                        ecs->addEntityComponent(entity, LocalTransform{}, currentRegistryID);
                    }
                    if (!registry.all_of<WorldTransform>(entity)) {
                        ecs->addEntityComponent(entity, WorldTransform{}, currentRegistryID);
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

                // Rebuild entity list if needed
                size_t current_count = static_cast<size_t>(ecs->getEntitiesCount(currentRegistryID));
                if (total_entities != current_count || force_refresh) {
                    total_entities = current_count;
                    editor_entities.clear();
                    force_refresh = false;

                    auto view = registry.view<Entity::Name>();
                    for (auto entity : view) {
                        auto& name_comp = view.get<Entity::Name>(entity);
                        editor_entities.push_back({ entity, name_comp.name });
                    }
                }

                // Begin ImGui window
                ImGui::Begin(name.c_str());
                dock_id = ImGui::GetWindowDockID();
                ImGui::Spacing();

                ImGui::Text("Number of entities in level: %d", total_entities);
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Drag entities to group them");
                ImGui::Spacing();

                if (ImGui::Checkbox("Sort A-Z", &sort_alphabetically)) {
                    force_refresh = true;
                }
                if (ImGui::Button("Normalize Sibling Indices")) {
                    std::function<void(entt::entity)> recursiveNormalize = [&](entt::entity parent) {
                        // 1. Get children of this parent
                        std::vector<entt::entity> children = getEntityChildren(parent);

                        // 2. Use your helper function to sort & fix indices
                        normalizeSiblings(children);

                        // 3. Recurse down
                        for (auto child : children) {
                            recursiveNormalize(child);
                        }
                    };

                    std::vector<entt::entity> roots = getRootEntities();
                    normalizeSiblings(roots);

                    for (auto root : roots) {
                        recursiveNormalize(root);
                    }

                    PN_CORE_INFO("Sibling indices normalized!");
                    force_refresh = true;
                }
                // Collapse / Expand toolbar
                float btn_w = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;
                if (ImGui::Button("Collapse All", ImVec2(btn_w, 0))) {
                    collapseAll();
                }
                ImGui::SameLine();
                if (ImGui::Button("Expand All", ImVec2(btn_w, 0))) {
                    expandAll();
                }
                ImGui::Spacing();

                ImGui::Spacing();
                
                // Search bar
                ImGui::Text("Search:");
                ImGui::SameLine();
                ImGui::SetNextItemWidth(-1);
                if (ImGui::InputText("##EntitySearch", search_buffer, sizeof(search_buffer))) {
                    // Search changed - will filter on next frame
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                // Get root entities
                std::vector<entt::entity> root_entities = getRootEntities();

                // Sort if enabled
                if (sort_alphabetically) {
                    std::sort(root_entities.begin(), root_entities.end(), [&](entt::entity a, entt::entity b) {
                        return getEntityName(a) < getEntityName(b);
                        });
                }
                else {
                    std::sort(root_entities.begin(), root_entities.end(), [&](entt::entity a, entt::entity b) {
                        auto hA = ecs->getEntityComponent<Entity::Hierarchy>(a);
                        auto hB = ecs->getEntityComponent<Entity::Hierarchy>(b);
                        int idxA = hA ? hA->get().siblingIndex : 0;
                        int idxB = hB ? hB->get().siblingIndex : 0;

                        return idxA < idxB;
                        });
                }

                // Get search filter (convert to lowercase for case-insensitive search)
                std::string search_filter = search_buffer;
                std::transform(search_filter.begin(), search_filter.end(), search_filter.begin(), ::tolower);

                // Render hierarchy (filter if search is active)
                for (auto entity : root_entities) {
                    if (search_filter.empty()) {
                        drawEntityHierarchy(entity, 0, search_filter);
                    } else {
                        // Check if this entity or any of its children match the search
                        if (entityMatchesSearch(entity, search_filter)) {
                            drawEntityHierarchy(entity, 0, search_filter);
                        }
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();

                if (ImGui::Button("Add Entity", ImVec2(-1, 0))) {
                    openPopUp("Create Entity");
                }

                // Show ungroup button for entities with children
                if (ecs->checkEntity(selected_entity, currentRegistryID)) {
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

                renderPopUps();
                ImGui::End();

                if (b_entity_changed) {
                    b_entity_changed = false;
                    b_entity_script_switched = true;
                }
            }

            void EntityPanel::drawEntityHierarchy(entt::entity entity, int depth, const std::string& search_filter) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->checkEntity(entity, currentRegistryID)) return;

                std::string entity_name = getEntityName(entity);
                bool is_selected = (selected_entity == entity);
                std::vector<entt::entity> children = getEntityChildren(entity);
                bool has_children = !children.empty();

                if (!sort_alphabetically) {
                    std::sort(children.begin(), children.end(), [&](entt::entity a, entt::entity b) {
                        auto hA = ecs->getEntityComponent<Entity::Hierarchy>(a, currentRegistryID);
                        auto hB = ecs->getEntityComponent<Entity::Hierarchy>(b, currentRegistryID);
                        int idxA = hA ? hA->get().siblingIndex : 0;
                        int idxB = hB ? hB->get().siblingIndex : 0;
                        return idxA < idxB;
                        });
                }
                // Check if this entity matches the search filter
                std::string entity_name_lower = entity_name;
                std::transform(entity_name_lower.begin(), entity_name_lower.end(), entity_name_lower.begin(), ::tolower);
                bool matches_search = search_filter.empty() || entity_name_lower.rfind(search_filter, 0) == 0;

                // Check if entity is a prefab instance
                bool is_prefab_instance = services->get<ECS::Controller>()->getRegistry(currentRegistryID).any_of<Prefab::PrefabInstance>(entity);
                
                // Check if this group is collapsed
                bool is_collapsed = collapsed_groups.find(entity) != collapsed_groups.end();

                // Create indentation
                float indent_width = depth * 20.0f;
                ImGui::Dummy(ImVec2(indent_width, 0));
                ImGui::SameLine(0, 0);

                // Draw collapse toggle for entities with children
                if (has_children) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.5f));
                    
                    std::string arrow_label = std::string(is_collapsed ? ">" : "v") + "##toggle" + std::to_string(static_cast<uint32_t>(entity));
                    if (ImGui::SmallButton(arrow_label.c_str())) {
                        if (is_collapsed) {
                            collapsed_groups.erase(entity);
                        } else {
                            collapsed_groups.insert(entity);
                        }
                    }
                    ImGui::PopStyleColor(3);
                    ImGui::SameLine();
                } else if (depth > 0) {
                    // Add spacing to align with siblings that have toggle buttons
                    ImGui::Dummy(ImVec2(20.0f, 0));
                    ImGui::SameLine(0, 0);
                }

                // Build display label
                std::string prefix = "";
                if (has_children) prefix += "[G] ";
                if (is_prefab_instance) prefix = "[P] " + prefix;

                std::string display_label = prefix + entity_name;
                std::string unique_label = display_label + "##" + std::to_string(static_cast<uint32_t>(entity));

                // Apply color styling (search match takes priority for highlighting)
                bool color_pushed = false;
                if (!search_filter.empty() && matches_search) {
                    // Highlight matching entities in yellow/gold color
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));
                    color_pushed = true;
                }
                else if (is_prefab_instance) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.7f, 1.0f, 1.0f));
                    color_pushed = true;
                }
                else if (depth > 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                    color_pushed = true;
                }

                // Render selectable
                if (ImGui::Selectable(unique_label.c_str(), is_selected)) {
                    selected_entity = is_selected ? entt::null : entity;
                    selectedEntityIndex = is_selected ? -1 : 0;
                    b_entity_changed = true;
                }

                if (color_pushed) {
                    ImGui::PopStyleColor();
                }

                // Drag & Drop: Start dragging
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                    ImGui::SetDragDropPayload("ENTITY_DRAG", &entity, sizeof(entt::entity));
                    ImGui::Text("Dragging: %s", entity_name.c_str());
                    ImGui::EndDragDropSource();
                }

                // Drag & Drop: Accept as parent
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_DRAG")) {
                        entt::entity dragged_entity = *(const entt::entity*)payload->Data;

                        if (dragged_entity != entity && !isAncestor(dragged_entity, entity)) {
                            setEntityParent(dragged_entity, entity);
                            force_refresh = true;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                // Right-click context menu
                if (ImGui::BeginPopupContextItem()) {
                    selected_entity = entity;
                    b_entity_changed = true;

                    if (ImGui::MenuItem("Move Up")) {
                        std::vector<entt::entity> siblings = getSiblings(entity);

                        normalizeSiblings(siblings);

                        // find current entity index in the SORTED list
                        auto it = std::find(siblings.begin(), siblings.end(), entity);
                        if (it != siblings.end() && it != siblings.begin()) {

                            // Swap values in the ECS components
                            entt::entity other = *std::prev(it);
                            auto hCurrent = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID);
                            auto hOther = ecs->getEntityComponent<Entity::Hierarchy>(other, currentRegistryID);

                            // Simple swap
                            std::swap(hCurrent->get().siblingIndex, hOther->get().siblingIndex);

                            force_refresh = true;
                        }
                    }
                    if (ImGui::MenuItem("Move Down")) {
                        std::vector<entt::entity> siblings = getSiblings(entity);

                        normalizeSiblings(siblings);

                        // find current entity index in the SORTED list
                        auto it = std::find(siblings.begin(), siblings.end(), entity);
                        if (it != siblings.end() && std::next(it) != siblings.end()) {
                            // Swap with the one after it
                            entt::entity other = *std::next(it);

                            auto hCurrent = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID);
                            auto hOther = ecs->getEntityComponent<Entity::Hierarchy>(other, currentRegistryID);

                            // SWAP INDICES
                            std::swap(hCurrent->get().siblingIndex, hOther->get().siblingIndex);

                            force_refresh = true;
                        }
                    }

                    if (ImGui::MenuItem("Clone")) {
                        openPopUp("Clone Entity");
                    }

#ifdef PN_PLATFORM_WINDOWS
                    if (ImGui::MenuItem("Create Prefab")) {
                        if (selected_entity != entt::null) {
                            // Get prefab service
                            auto prefab_service = services->get<Prefab::Service>();

                            // Generate unique name
                            std::string prefab_name = generateUniquePrefabName(getEntityName(selected_entity));

                            // Create prefab
                            prefab_service->createPrefab(selected_entity, prefab_name, currentRegistryID);

                            PN_CORE_INFO("Created prefab: {}", prefab_name);
                        }
                    }

                    if (ImGui::MenuItem("Create Template")) {
                        if (selected_entity != entt::null) {
                            // Get prefab service
                            auto template_service = services->get<EntityTemplate::Service>();

                            // Generate unique name
                            std::string template_name = generateUniqueTemplateName(getEntityName(selected_entity));

                            // Create prefab
                            template_service->createFromEntity(selected_entity, template_name, currentRegistryID);

                            PN_CORE_INFO("Created template: {}", template_name);
                        }
                    }
#endif

                    ImGui::Separator();

                    if (has_children && ImGui::MenuItem("Ungroup")) {
                        ungroupEntity(entity);
                    }

                    if (auto hierarchy = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID)) {
                        if (hierarchy.value().get().parentGUID.IsValid() && ImGui::MenuItem("Move to Root")) {
                            removeParent(entity);
                            force_refresh = true;
                        }
                    }

                    ImGui::Separator();

                    if (ImGui::MenuItem("Delete", "Del")) {
                        entt::entity entity_to_delete = entity;
                        std::string delete_name = entity_name;

                        command_manager->executeAction(Action{
                            [this, entity_to_delete]() {
                                removeEntityWithChildren(entity_to_delete);
                                if (selected_entity == entity_to_delete) {
                                    selected_entity = entt::null;
                                }
                                force_refresh = true;
                            },
                            []() { PN_CORE_WARN("Undo for entity deletion not yet implemented"); },
                            "Delete Entity: " + delete_name
                            });
                    }

                    ImGui::EndPopup();
                }

                // Recursively draw children (filter if search is active)
                if (has_children && (!is_collapsed || !search_filter.empty())) {
                    for (auto child : children) {
                        if (search_filter.empty()) {
                            drawEntityHierarchy(child, depth + 1, search_filter);
                        } else {
                            // Only draw children that match the search or have matching descendants
                            if (entityMatchesSearch(child, search_filter)) {
                                drawEntityHierarchy(child, depth + 1, search_filter);
                            }
                        }
                    }
                }
            }

            void EntityPanel::collapseAll() {
                auto ecs = PN_ECS_SERVICE;
                auto& registry = ecs->getRegistry(currentRegistryID);
                auto view = registry.view<Entity::Hierarchy>();
                for (auto entity : view) {
                    auto h = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID);
                    if (h && !h->get().childrenGUIDs.empty()) {
                        collapsed_groups.insert(entity);
                    }
                }
            }

            void EntityPanel::expandAll() {
                collapsed_groups.clear();
            }

            bool EntityPanel::entityMatchesSearch(entt::entity entity, const std::string& search_filter) {
                auto ecs = PN_ECS_SERVICE;
                
                if (!ecs->checkEntity(entity, currentRegistryID)) return false;
                
                // Check if this entity's name matches
                std::string entity_name = getEntityName(entity);
                std::string entity_name_lower = entity_name;
                std::transform(entity_name_lower.begin(), entity_name_lower.end(), entity_name_lower.begin(), ::tolower);
                
                if (entity_name_lower.rfind(search_filter, 0) == 0) {
                    return true;
                }
                
                // Check if any children match
                std::vector<entt::entity> children = getEntityChildren(entity);
                for (auto child : children) {
                    if (entityMatchesSearch(child, search_filter)) {
                        return true;
                    }
                }
                
                return false;
            }

            std::vector<entt::entity> EntityPanel::getRootEntities() {
                std::vector<entt::entity> roots;
                auto ecs = PN_ECS_SERVICE;
                auto& registry = ecs->getRegistry(currentRegistryID);

                // 1. Gather all entities that are actually claimed as children by someone else
                std::unordered_set<entt::entity> claimed_children;
                for (const auto& [entity, name] : editor_entities) {
                    // Use your existing helper to get valid children
                    std::vector<entt::entity> children = getEntityChildren(entity);
                    for (auto child : children) {
                        claimed_children.insert(child);
                    }
                }

                // 2. Identify actual roots
                for (const auto& [entity, name] : editor_entities) {
                    // If another entity in the scene claims this as a child, it CANNOT be a root.
                    if (claimed_children.find(entity) != claimed_children.end()) {
                        continue;
                    }

                    // Optional/Fallback: You can still check parentGUID here if you want to be extra safe 
                    // against orphaned entities, but the above check guarantees it won't be drawn twice.
                    roots.push_back(entity);
                }

                return roots;
            }

            std::vector<entt::entity> EntityPanel::getEntityChildren(entt::entity parent) {
                std::vector<entt::entity> children;
                auto ecs = PN_ECS_SERVICE;

                auto hierarchy_opt = ecs->getEntityComponent<Entity::Hierarchy>(parent, currentRegistryID);
                if (!hierarchy_opt) return children;

                auto& hierarchy = hierarchy_opt.value().get();
                for (const auto& childGUID : hierarchy.childrenGUIDs) {
                    entt::entity child = ecs->resolveGUID(childGUID, currentRegistryID);
                    if (child != entt::null && ecs->checkEntity(child, currentRegistryID)) {
                        children.push_back(child);
                    }
                }

                return children;
            }

            void EntityPanel::setEntityParent(entt::entity child, entt::entity parent) {
                auto ecs = PN_ECS_SERVICE;
                auto transformSystem = ecs->getSystem<Transform::System>();

                std::vector<entt::entity> oldSiblings = getSiblings(child);

                // Remove old child
                oldSiblings.erase(std::remove(oldSiblings.begin(), oldSiblings.end(), child), oldSiblings.end());
                normalizeSiblings(oldSiblings);

                if (transformSystem) {
                    transformSystem->setParent(child, parent, ecs->getRegistry(currentRegistryID));
                    transformSystem->markDirty(child, ecs->getRegistry(currentRegistryID));

                    // Assign new sibling index
                    std::vector<entt::entity> newSiblings = getEntityChildren(parent);
                    normalizeSiblings(newSiblings);

                }
            }

            void EntityPanel::removeParent(entt::entity child) {
                auto ecs = PN_ECS_SERVICE;
                auto transformSystem = ecs->getSystem<Transform::System>();

                if (transformSystem) {
                    transformSystem->removeParent(child, ecs->getRegistry(currentRegistryID));
                    transformSystem->markDirty(child, ecs->getRegistry(currentRegistryID));
                }
            }

            bool EntityPanel::isAncestor(entt::entity potential_ancestor, entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;

                auto hierarchy_opt = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID);
                if (!hierarchy_opt) return false;

                std::unordered_set<Assets::GUID> visited;
                Assets::GUID current_parent_guid = hierarchy_opt->get().parentGUID;

                constexpr size_t MAX_DEPTH = 100;
                size_t depth = 0;

                while (current_parent_guid.IsValid() && depth++ < MAX_DEPTH) {
                    if (visited.count(current_parent_guid) > 0) {
                        PN_CORE_ERROR("Cycle detected in hierarchy");
                        return false;
                    }
                    visited.insert(current_parent_guid);

                    entt::entity current_parent = ecs->resolveGUID(current_parent_guid, currentRegistryID);
                    if (current_parent == potential_ancestor) {
                        return true;
                    }

                    auto parent_hierarchy_opt = ecs->getEntityComponent<Entity::Hierarchy>(current_parent, currentRegistryID);
                    if (!parent_hierarchy_opt) break;

                    current_parent_guid = parent_hierarchy_opt->get().parentGUID;
                }

                return false;
            }

            void EntityPanel::removeEntityWithChildren(entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->checkEntity(entity, currentRegistryID)) return;

                std::vector<entt::entity> children = getEntityChildren(entity);
                for (auto child : children) {
                    removeEntityWithChildren(child);
                }

                // Remove from parent's children list
                if (auto hierarchy = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID)) {
                    if (hierarchy.value().get().parentGUID.IsValid()) {
                        entt::entity parent = ecs->resolveGUID(hierarchy.value().get().parentGUID, currentRegistryID);
                        if (parent != entt::null && ecs->checkEntity(parent, currentRegistryID)) {
                            if (auto parent_hierarchy = ecs->getEntityComponent<Entity::Hierarchy>(parent, currentRegistryID)) {
                                auto& children_guids = parent_hierarchy.value().get().childrenGUIDs;
                                auto child_guid = ecs->getEntityComponent<Entity::GUID>(entity, currentRegistryID);
                                if (child_guid) {
                                    children_guids.erase(
                                        std::remove(children_guids.begin(), children_guids.end(), child_guid.value().get().guid),
                                        children_guids.end()
                                    );
                                }
                            }
                        }
                    }
                }

                // Adjust sibling order
                std::vector<entt::entity> siblings;

                // FIND entity's siblings
                auto h = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID);
                if (h && h->get().parentGUID.IsValid()) {
                    // Case A: Has Parent -> Siblings are Parent's children
                    entt::entity parent = ecs->resolveGUID(h->get().parentGUID, currentRegistryID);
                    siblings = getEntityChildren(parent);
                }
                else {
                    // Case B: No Parent -> Siblings are Root entities
                    siblings = getRootEntities();
                }

                siblings.erase(std::remove(siblings.begin(), siblings.end(), entity), siblings.end());
                normalizeSiblings(siblings);

                ecs->destroyEntity(entity, currentRegistryID);
            }

            void EntityPanel::cloneEntityWithChildren(entt::entity source, entt::entity cloned_parent) {
                auto children = getEntityChildren(source);
                auto ecs = PN_ECS_SERVICE;

                for (auto child : children) {
                    entt::entity cloned_child = ecs->cloneEntity(child, currentRegistryID, currentRegistryID);
                   
                    // Wipe the Hierarchy of the new child before adding it
                    if (auto h = ecs->getEntityComponent<Entity::Hierarchy>(cloned_child, currentRegistryID)) {
                        h->get().childrenGUIDs.clear();
                        h->get().parentGUID = Assets::GUID(); // Reset parent until setEntityParent sets it
                    }

                    setEntityParent(cloned_child, cloned_parent);
                    cloneEntityWithChildren(child, cloned_child);
                }
            }

            void EntityPanel::ungroupEntity(entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->checkEntity(entity, currentRegistryID)) return;

                auto hierarchy_opt = ecs->getEntityComponent<Entity::Hierarchy>(entity, currentRegistryID);
                if (!hierarchy_opt) return;

                Assets::GUID parent_guid = hierarchy_opt->get().parentGUID;
                entt::entity parent = parent_guid.IsValid() ? ecs->resolveGUID(parent_guid, currentRegistryID) : entt::null;

                std::vector<entt::entity> children = getEntityChildren(entity);
                for (auto child : children) {
                    if (parent != entt::null) {
                        setEntityParent(child, parent);
                    }
                    else {
                        removeParent(child);
                    }
                }

                force_refresh = true;
            }

            void EntityPanel::normalizeSiblings(std::vector<entt::entity>& siblings)
            {
                auto ecs = PN_ECS_SERVICE;

                // SORT based on current siblingIndex, stable_sort to keep relative order if indices are equal (e.g. both are 0)
                std::stable_sort(siblings.begin(), siblings.end(), [&](entt::entity a, entt::entity b) {
                    auto hA = ecs->getEntityComponent<Entity::Hierarchy>(a, currentRegistryID);
                    auto hB = ecs->getEntityComponent<Entity::Hierarchy>(b, currentRegistryID);

                    int idxA = hA ? hA->get().siblingIndex : 0;
                    int idxB = hB ? hB->get().siblingIndex : 0;

                    // Primary sort: The Index
                    if (idxA != idxB) {
                        return idxA < idxB;
                    }

                    // Tie-breaker: Entity ID (keeps order consistent if indices are identical)
                    return a < b;
                    });

                // RE-ASSIGN clean 0, 1, 2, 3...
                for (int i = 0; i < siblings.size(); i++) {
                    if (auto h = ecs->getEntityComponent<Entity::Hierarchy>(siblings[i], currentRegistryID)) {
                        // Only update if changed (to avoid unnecessary dirty flags)
                        if (h->get().siblingIndex != i) {
                            h->get().siblingIndex = i;
                            
                        }
                    }
                }
            }

            std::vector<entt::entity> EntityPanel::getSiblings(entt::entity e)
            {
                auto ecs = PN_ECS_SERVICE;
                auto h = PN_ECS_SERVICE->getEntityComponent<Entity::Hierarchy>(e, currentRegistryID);
                if (h && h->get().parentGUID.IsValid()) {
                    // It's a child, get parent's children
                    entt::entity parent = ecs->resolveGUID(h->get().parentGUID, currentRegistryID);
                    return getEntityChildren(parent);
                }
                return getRootEntities(); // It's a root
            }

            std::string EntityPanel::getEntityName(entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;
                auto name_comp = ecs->getEntityComponent<Entity::Name>(entity, currentRegistryID);
                return name_comp ? name_comp.value().get().name : "Unnamed";
            }

#ifdef PN_PLATFORM_WINDOWS
            std::string EntityPanel::generateUniquePrefabName(const std::string& base_name) {
                auto path_service = services->get<Path::Path>();
                auto prefab_folder = Assets::getAllGameFolders()[Assets::Type::Prefabs];
                std::filesystem::path full_path = path_service->resolvePath(Path::main_assets_alias, prefab_folder.string());

                //Prefab ext
                auto prefab_ext = *Assets::getAllExtensions()[Assets::Type::Prefabs].begin();

                int counter = 1;
                std::string prefab_name = base_name;
                full_path /= (prefab_name + prefab_ext);

                // Check if file exists and increment counter
                while (std::filesystem::exists(full_path)) {
                    prefab_name = base_name + "_" + std::to_string(counter++);
                    full_path.replace_filename(prefab_name + prefab_ext);
                }

                return prefab_name;
            }

            std::string EntityPanel::generateUniqueTemplateName(const std::string& base_name) {
                auto path_service = services->get<Path::Path>();
                auto template_folder = Assets::getAllGameFolders()[Assets::Type::Templates];
                std::filesystem::path full_path = path_service->resolvePath(Path::main_assets_alias, template_folder.string());

                //Prefab ext
                auto prefab_ext = *Assets::getAllExtensions()[Assets::Type::Templates].begin();

                int counter = 1;
                std::string template_name = base_name;
                full_path /= (template_name + prefab_ext);

                // Check if file exists and increment counter
                while (std::filesystem::exists(full_path)) {
                    template_name = base_name + "_" + std::to_string(counter++);
                    full_path.replace_filename(template_name + prefab_ext);
                }

                return template_name;
            }
#endif

            void EntityPanel::collectEntityHierarchy(entt::entity entity, std::vector<entt::entity>& out_entities) {
                out_entities.push_back(entity);
                auto children = getEntityChildren(entity);
                for (auto child : children) {
                    collectEntityHierarchy(child, out_entities);
                }
            }

        } 
    } 
} 

#endif
