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
#include "EntityPanel.h"
#include "ECS/Controller.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cEntity.h"
#include "Systems/Transform/sysTransform.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "CoreSystems/Prefabs/sPrefab.h"

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
                                auto scene = services->get<Scene>();

                                entt::entity entity = ecs->createEntity(); // Auto-assigns GUID

                                // Add core components
                                ecs->addEntityComponent(entity, MetaData::EntityName{ final_name });
                                ecs->addEntityComponent(entity, LocalTransform{});
                                ecs->addEntityComponent(entity, WorldTransform{});
                                ecs->addEntityComponent(entity, Entity::Hierarchy{});

                                // Optional: Add default model
                                if (scene) {
                                    auto models = services->get<Assets::Manager>()->getAllAssetsOfType<Assets::Model>(Assets::Type::Model);
                                    if (!models.empty()) {
                                        ecs->addEntityComponent(entity, ModelRenderer{ models.front()->guid });
                                    }
                                }

                                // Mark transform dirty
                                auto transformSystem = ecs->getSystem<Transform::System>();
                                if (transformSystem) {
                                    transformSystem->markDirty(entity, ecs->getRegistry());
                                }
                            },
                            [this, final_name]() {
                                // Undo: find and delete by name
                                auto ecs = PN_ECS_SERVICE;
                                auto& registry = ecs->getRegistry();
                                auto view = registry.view<MetaData::EntityName>();

                                for (auto entity : view) {
                                    if (view.get<MetaData::EntityName>(entity).name == final_name) {
                                        ecs->destroyEntity(entity);
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
                                if (ecs->checkEntity(entity_to_remove)) {
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

                                if (ecs->checkEntity(clone_source)) {
                                    entt::entity new_entity = ecs->cloneEntity(clone_source);

                                    // Set name
                                    if (auto name_comp = ecs->getEntityComponent<MetaData::EntityName>(new_entity)) {
                                        name_comp.value().get().name = final_name;
                                    }

                                    // Clone children recursively
                                    cloneEntityWithChildren(clone_source, new_entity);

                                    // Mark dirty
                                    auto transformSystem = ecs->getSystem<Transform::System>();
                                    if (transformSystem) {
                                        transformSystem->markDirty(new_entity, ecs->getRegistry());
                                    }
                                }
                            },
                            [this, final_name]() {
                                auto ecs = PN_ECS_SERVICE;
                                auto& registry = ecs->getRegistry();
                                auto view = registry.view<MetaData::EntityName>();

                                for (auto entity : view) {
                                    if (view.get<MetaData::EntityName>(entity).name == final_name) {
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

            void EntityPanel::nextWindowSettings() {}
            void EntityPanel::onAttach() {}

            void EntityPanel::onUpdate(PAIN::AppTiming timing) {
                auto ecs = PN_ECS_SERVICE;
                auto ser = PN_SERI_SERVICE;
                auto& registry = ecs->getRegistry();

                // Auto-add required components to all entities
                auto view_all = registry.view<MetaData::EntityName>();
                for (auto entity : view_all) {
                    if (!registry.all_of<Entity::Hierarchy>(entity)) {
                        ecs->addEntityComponent(entity, Entity::Hierarchy{});
                    }
                    if (!registry.all_of<LocalTransform>(entity)) {
                        ecs->addEntityComponent(entity, LocalTransform{});
                    }
                    if (!registry.all_of<WorldTransform>(entity)) {
                        ecs->addEntityComponent(entity, WorldTransform{});
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
                size_t current_count = static_cast<size_t>(ecs->getEntitiesCount());
                if (total_entities != current_count || force_refresh) {
                    total_entities = current_count;
                    editor_entities.clear();
                    force_refresh = false;

                    auto view = registry.view<MetaData::EntityName>();
                    for (auto entity : view) {
                        auto& name_comp = view.get<MetaData::EntityName>(entity);
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

                // Render hierarchy
                for (auto entity : root_entities) {
                    drawEntityHierarchy(entity, 0);
                }

                ImGui::Spacing();
                ImGui::Separator();

                if (ImGui::Button("Add Entity", ImVec2(-1, 0))) {
                    openPopUp("Create Entity");
                }

                // Show ungroup button for entities with children
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

                renderPopUps();
                ImGui::End();

                if (b_entity_changed) {
                    b_entity_changed = false;
                }
            }

            void EntityPanel::drawEntityHierarchy(entt::entity entity, int depth) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->checkEntity(entity)) return;

                std::string entity_name = getEntityName(entity);
                bool is_selected = (selected_entity == entity);
                std::vector<entt::entity> children = getEntityChildren(entity);
                bool has_children = !children.empty();

                // Create indentation
                std::string indent(depth * 2, ' ');
                std::string prefix = depth > 0 ? "-> " : "";
                if (has_children) prefix += "[G] ";

                std::string display_label = indent + prefix + entity_name;
                std::string unique_label = display_label + "##" + std::to_string(static_cast<uint32_t>(entity));

                // Apply color for child entities
                if (depth > 0) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.9f, 1.0f, 1.0f));
                }

                // Render selectable
                if (ImGui::Selectable(unique_label.c_str(), is_selected)) {
                    selected_entity = is_selected ? entt::null : entity;
                    selectedEntityIndex = is_selected ? -1 : 0;
                    b_entity_changed = true;
                }

                if (depth > 0) {
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

                    if (ImGui::MenuItem("Clone")) {
                        openPopUp("Clone Entity");
                    }

                    if (ImGui::MenuItem("Create Prefab from Entity")) {
                        if (selected_entity != entt::null) {
                            // Get prefab service
                            auto prefab_service = services->get<Prefab::Service>();

                            // Generate unique name
                            std::string prefab_name = generateUniquePrefabName(getEntityName(selected_entity));

                            // Create prefab
                            prefab_service->createPrefab(selected_entity, prefab_name, ecs->getRegistry());

                            PN_CORE_INFO("Created prefab: {}", prefab_name);
                        }
                    }

                    ImGui::Separator();

                    if (has_children && ImGui::MenuItem("Ungroup")) {
                        ungroupEntity(entity);
                    }

                    if (auto hierarchy = ecs->getEntityComponent<Entity::Hierarchy>(entity)) {
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

                // Recursively draw children
                if (has_children) {
                    for (auto child : children) {
                        drawEntityHierarchy(child, depth + 1);
                    }
                }
            }

            std::vector<entt::entity> EntityPanel::getRootEntities() {
                std::vector<entt::entity> roots;
                auto ecs = PN_ECS_SERVICE;
                auto& registry = ecs->getRegistry();

                for (const auto& [entity, name] : editor_entities) {
                    if (auto hierarchy = ecs->getEntityComponent<Entity::Hierarchy>(entity)) {
                        if (!hierarchy.value().get().parentGUID.IsValid()) {
                            roots.push_back(entity);
                        }
                    }
                    else {
                        roots.push_back(entity);
                    }
                }

                return roots;
            }

            std::vector<entt::entity> EntityPanel::getEntityChildren(entt::entity parent) {
                std::vector<entt::entity> children;
                auto ecs = PN_ECS_SERVICE;

                auto hierarchy_opt = ecs->getEntityComponent<Entity::Hierarchy>(parent);
                if (!hierarchy_opt) return children;

                auto& hierarchy = hierarchy_opt.value().get();
                for (const auto& childGUID : hierarchy.childrenGUIDs) {
                    entt::entity child = ecs->resolveGUID(childGUID);
                    if (child != entt::null && ecs->checkEntity(child)) {
                        children.push_back(child);
                    }
                }

                return children;
            }

            void EntityPanel::setEntityParent(entt::entity child, entt::entity parent) {
                auto ecs = PN_ECS_SERVICE;
                auto transformSystem = ecs->getSystem<Transform::System>();

                if (transformSystem) {
                    transformSystem->setParent(child, parent, ecs->getRegistry());
                    transformSystem->markDirty(child, ecs->getRegistry());
                }
            }

            void EntityPanel::removeParent(entt::entity child) {
                auto ecs = PN_ECS_SERVICE;
                auto transformSystem = ecs->getSystem<Transform::System>();

                if (transformSystem) {
                    transformSystem->removeParent(child, ecs->getRegistry());
                    transformSystem->markDirty(child, ecs->getRegistry());
                }
            }

            bool EntityPanel::isAncestor(entt::entity potential_ancestor, entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;

                auto hierarchy_opt = ecs->getEntityComponent<Entity::Hierarchy>(entity);
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

                    entt::entity current_parent = ecs->resolveGUID(current_parent_guid);
                    if (current_parent == potential_ancestor) {
                        return true;
                    }

                    auto parent_hierarchy_opt = ecs->getEntityComponent<Entity::Hierarchy>(current_parent);
                    if (!parent_hierarchy_opt) break;

                    current_parent_guid = parent_hierarchy_opt->get().parentGUID;
                }

                return false;
            }

            void EntityPanel::removeEntityWithChildren(entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->checkEntity(entity)) return;

                std::vector<entt::entity> children = getEntityChildren(entity);
                for (auto child : children) {
                    removeEntityWithChildren(child);
                }

                // Remove from parent's children list
                if (auto hierarchy = ecs->getEntityComponent<Entity::Hierarchy>(entity)) {
                    if (hierarchy.value().get().parentGUID.IsValid()) {
                        entt::entity parent = ecs->resolveGUID(hierarchy.value().get().parentGUID);
                        if (parent != entt::null && ecs->checkEntity(parent)) {
                            if (auto parent_hierarchy = ecs->getEntityComponent<Entity::Hierarchy>(parent)) {
                                auto& children_guids = parent_hierarchy.value().get().childrenGUIDs;
                                auto child_guid = ecs->getEntityComponent<Entity::GUID>(entity);
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

                ecs->destroyEntity(entity);
            }

            void EntityPanel::cloneEntityWithChildren(entt::entity source, entt::entity cloned_parent) {
                auto children = getEntityChildren(source);
                auto ecs = PN_ECS_SERVICE;

                for (auto child : children) {
                    entt::entity cloned_child = ecs->cloneEntity(child);
                    setEntityParent(cloned_child, cloned_parent);
                    cloneEntityWithChildren(child, cloned_child);
                }
            }

            void EntityPanel::ungroupEntity(entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;

                if (!ecs->checkEntity(entity)) return;

                auto hierarchy_opt = ecs->getEntityComponent<Entity::Hierarchy>(entity);
                if (!hierarchy_opt) return;

                Assets::GUID parent_guid = hierarchy_opt->get().parentGUID;
                entt::entity parent = parent_guid.IsValid() ? ecs->resolveGUID(parent_guid) : entt::null;

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

            std::string EntityPanel::getEntityName(entt::entity entity) {
                auto ecs = PN_ECS_SERVICE;
                auto name_comp = ecs->getEntityComponent<MetaData::EntityName>(entity);
                return name_comp ? name_comp.value().get().name : "Unnamed";
            }

            std::string EntityPanel::generateUniquePrefabName(const std::string& base_name) {
                auto seri = PN_SERI_SERVICE;
                std::string prefab_name = base_name;
                int counter = 1;

                while (std::filesystem::exists(seri->resolvePrefabPath(prefab_name))) {
                    prefab_name = base_name + " (" + std::to_string(counter++) + ")";
                }

                return prefab_name;
            }

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
