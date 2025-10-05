#include "pch.h"
#include "EntityPanel.h"
#include "ECS/Controller.h"
#include "ECS/sMetaData.h"
#include "ECS/Components/cMetadata.h"

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

#define PN_ECS_SERVICE services->get<ECS::Controller>()
#define PN_METADATA_SERVICE services->get<MetaData::Service>()
            EntityPanel::EntityPanel() {

                name = "Entity Panel";
                flags = ImGuiWindowFlags_None;

                // Regiser popup here
                registerPopUp("Create Entity", createEntityPopUp("Create Entity"));
                registerPopUp("Remove Entity", removeEntityPopUp("Remove Entity"));
                registerPopUp("Clone Entity", cloneEntityPopUp("Clone Entity"));
                registerPopUp("Error", defPopUp("Error", error_msg));
            }

            std::function<void()> EntityPanel::createEntityPopUp(std::string const& popup_id) {
                return [this, popup_id]() {

                    //Static entity name input buffer
                    static char entity_name[128] = "";

                    //Get entity text
                    ImGui::Text("Enter a name for the new entity:");
                    if (ImGui::InputText("##Entity Name", entity_name, sizeof(entity_name))) {

                    }

                    //If enter or ok button is pressed
                    if (strlen(entity_name) > 0 && (ImGui::Button("OK") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                        //Temporary create action
                        Action create;

                        //Create a shared id for do & undo functions
                        std::shared_ptr<std::string> shared_id = std::make_shared<std::string>(entity_name);

                        //Do Action
                        create.do_action = [&, shared_id]() {
                            //Create new entity 
                            auto ecs = services->get<ECS::Controller>();

                            int total_entities = ecs->getAllEntities().size();
                            // For simplicity, just add a new entity with a generic name
                            glm::vec3 pos = glm::vec3(1.f, 1.f, 1.f * total_entities);
                            glm::quat rot = { 0.f,0.f,0.f, 0.f };
                            glm::vec3 scale = { 1.f, 1.f, 1.f };

                            ECS::Entity::Type entity = ecs->createEntity();
                            ecs->addEntityComponent(entity, MetaData::EntityName{ entity_name });
                            ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
                            ecs->addEntityComponent(entity, MeshRenderer{ Mesh::LoadObj() });

                            };

                        //Undo Action
                        create.undo_action = [&, shared_id]() {

                            };

                        //Execute create entity action
                        command_manager->executeAction(std::move(create));

                        //Reset layer id
                        //layer_id = 0;

                        //Reset entity name
                        entity_name[0] = '\0';

                        //Close popup
                        closePopUp(popup_id);
                    }

                    ImGui::SameLine();

                    //Cancel creating new entity
                    if (ImGui::Button("Cancel")) {

                        //Reset layer id
                        //layer_id = 0;

                        //Reset entity name
                        entity_name[0] = '\0';

                        //Close popup
                        closePopUp(popup_id);
                    }
                };
            }

            std::function<void()> EntityPanel::removeEntityPopUp(std::string const& popup_id)
            {
                return [this, popup_id]() {

                    //Selected entity name
                    std::string selected_name = PN_ECS_SERVICE->getEntityComponent<MetaData::EntityName>(selected_entity).value().get().name;

                    //Confirm removal of entity
                    ImGui::Text("Are you sure you want to remove %s?", selected_name.c_str());

                    //If enter or ok button is pressed
                    if (ImGui::Button("Remove") || ImGui::IsKeyPressed(ImGuiKey_Enter)) {

                        //Temporary remove action
                        Action remove;

                        //Create a shared id for do & undo functions
                        std::shared_ptr<std::string> shared_id = std::make_shared<std::string>(selected_name);

                        //Get all entity comps for pass by value storage
                        auto comps = PN_ECS_SERVICE->getAllCopiedEntityComponents(selected_entity);
                        auto comp_types = PN_ECS_SERVICE->getAllComponentTypes();
                        //int layer_id = PN_METADATA_SERVICE->getEntityLayerID(selected_entity);

                        //Setup undo action for remove
                        remove.undo_action = [&, shared_id, comps, comp_types]() {

                            //Creat new entity 
                            ECS::Entity::Type new_id = PN_ECS_SERVICE->createEntity();
                            //PN_METADATA_SERVICE->setEntityLayerID(new_id, layer_id);

                            //Add all the comps back
                            for (auto&& comp : comps) {
                                PN_ECS_SERVICE->addDefEntityComponent(new_id, comp_types.at(comp.first));
                                PN_ECS_SERVICE->setEntityComponent(new_id, comp_types.at(comp.first), comp.second);
                            }

                            //Update metadata name ref
                            PN_METADATA_SERVICE->setEntityName(new_id, *shared_id);

                            //Set selected entity back to old entity
                            selected_entity = new_id;
                            };

                        //Setup action for removing entity
                        remove.do_action = [&, shared_id]() {

                            //Check if entity is still alive
                            auto entity = PN_ECS_SERVICE->checkEntity(selected_entity);

                            if (entity) {
                                //Destroy new entity
                                PN_ECS_SERVICE->destroyEntity(selected_entity);

                            }

                            //Set selected entity to an invalid entity
                            selected_entity = UINT16_MAX;
                            };

                        //Execute remove action
                        command_manager->executeAction(std::move(remove));

                        //Close popup
                        closePopUp(popup_id);
                    }

                    ImGui::SameLine();

                    //Cancel removing entity
                    if (ImGui::Button("Cancel")) {

                        //Close popup
                        closePopUp(popup_id);
                    }
                };
            }

            std::function<void()> EntityPanel::cloneEntityPopUp(std::string const& popup_id)
            {
                return [this, popup_id]() {

                    //Static entity name input buffer
                    static char entity_name[128] = "";

                    //Get entity text
                    ImGui::Text("Enter a new name for the cloned entity:");
                    if (ImGui::InputText("##Entity Clone Name", entity_name, sizeof(entity_name))) {

                    }

                    //If enter or clone button is pressed
                    if (strlen(entity_name) > 0 && (ImGui::Button("Clone") || ImGui::IsKeyPressed(ImGuiKey_Enter))) {
                        //Temporary clone action
                        Action clone;

                        //Create a shared id for do & undo functions
                        std::shared_ptr<std::string> shared_id = std::make_shared<std::string>(entity_name);

                        //Clone entity for capturing by value
                        ECS::Entity::Type clone_entity = selected_entity;

                        //Do Action
                        clone.do_action = [&, shared_id, clone_entity]() {
                            if (PN_ECS_SERVICE->checkEntity(clone_entity)) {
                                //Clone entity 
                                ECS::Entity::Type new_id = PN_ECS_SERVICE->cloneEntity(clone_entity);
                                //PN_METADATA_SERVICE->setEntityLayerID(new_id, PN_METADATA_SERVICE->getEntityLayerID(clone_entity));

                                //If entity name is valid
                                if (!shared_id->empty() && PN_METADATA_SERVICE->isNameValid(*shared_id))
                                {
                                    PN_METADATA_SERVICE->setEntityName(new_id, *shared_id);
                                }
                                else {
                                    shared_id->assign(PN_METADATA_SERVICE->getEntityName(new_id));
                                }
                            }
                            };

                        //Undo Action
                        clone.undo_action = [&, shared_id]() {

                            //Check if entity is still alive
                            auto entity = PN_METADATA_SERVICE->getEntityByName(*shared_id);

                            if (entity.has_value()) {
                                //Destroy new entity
                                PN_ECS_SERVICE->destroyEntity(entity.value());
                            }
                            };

                        //Execute create entity action
                        command_manager->executeAction(std::move(clone));

                        //Reset entity name
                        entity_name[0] = '\0';

                        //Close popup
                        closePopUp(popup_id);
                    }

                    ImGui::SameLine();

                    //Cancel creating new entity
                    if (ImGui::Button("Cancel")) {

                        //Reset entity name
                        entity_name[0] = '\0';

                        //Close popup
                        closePopUp(popup_id);
                    }
                    };
            }

            void EntityPanel::nextWindowSettings() {
                // No special behavior here
            }
            
            ECS::Entity::Type EntityPanel::getSelectedEntity() const
            {
                return selected_entity;
            }

            void EntityPanel::unselectEntity()
            {
                selected_entity = UINT16_MAX;
            }

            bool EntityPanel::isEntityChanged() const
            {
                return b_entity_changed;
            }

            void EntityPanel::onAttach()
            {
            }

            void EntityPanel::onUpdate(PAIN::AppTiming timing) {

                // Detect if a scene was just loaded
                auto ser = services->get<Serialization::Service>();
                bool sceneChanged = ser && ser->consumeSceneChanged();
                if (sceneChanged) {
                    // Clear selection and force a rebuild next frame
                    selected_entity = ECS::Entity::INVALID;  // or call unselectEntity()
                    selectedEntityIndex = -1;
                    editor_entities.clear();
                    total_entities = 0;                     
                    // forces rebuild
                    b_entity_changed = true;
                    PN_CORE_INFO("[EntityPanel] Scene changed detected — list reset");
                }

                // Update entity list
                auto ecs = services->get<ECS::Controller>();
                auto scene = services->get<Scene>();
                if (total_entities != ecs->getAllEntities().size()) {
                    auto ecs_entities = ecs->getAllEntities();
                    total_entities = ecs_entities.size();
                    editor_entities.clear();

                    for (auto& e : ecs_entities) {
                        // Get entity name from MetaData::EntityName component
                        auto name_opt = ecs->getEntityComponent<MetaData::EntityName>(e);
                        if (name_opt.has_value()) {
                            std::string entity_name = name_opt->get().name;

                            // Optionally get tags from MetaData::Tag component
                            auto tag_opt = ecs->getEntityComponent<MetaData::Tag>(e);

                            // Store entity with its name
                            editor_entities.push_back(std::pair<ECS::Entity::Type, std::string>{e, entity_name});

                            // Optional: You can also filter by tags here if needed
                            // if (tag_opt.has_value()) {
                            //     auto& tags = tag_opt->get().tags;
                            //     // Do something with tags
                            // }
                        }
                    }

                }
                
                ImGui::Begin(name.c_str());

                //Set window dock id
                dock_id = ImGui::GetWindowDockID();

                ImGui::Spacing();

                //Entities (no layering)

                ImGui::Text("Number of entities in level: %d", PN_ECS_SERVICE->getEntitiesCount());


                ImGui::Spacing();


                // Iterate through all entities directly (no layering)
                for (size_t i = 0; i < editor_entities.size(); ++i) {
                    ECS::Entity::Type entity_id = editor_entities[i].first;
                    bool isSelected = (selected_entity == entity_id);
                    std::string label = editor_entities[i].second + "##" + std::to_string(entity_id);

                    // Direct selection without action
                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        if (isSelected) {
                            // Deselect if clicking the same entity
                            selected_entity = ECS::Entity::INVALID;
                            selectedEntityIndex = -1;
                        }
                        else {
                            // Select new entity
                            selected_entity = entity_id;
                            selectedEntityIndex = i;
                        }
                        b_entity_changed = true;
                    }

                    // Show tooltip on hover
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Entity ID: %u", static_cast<uint32_t>(entity_id));
                        auto metadata = services->get<MetaData::Service>();
                        if (metadata) {
                            if (metadata->isLocked(entity_id)) {
                                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "LOCKED");
                            }
                        }
                        ImGui::EndTooltip();
                    }
                }

                ImGui::Spacing();

                if (ImGui::Button("Add Entity")) {
                    openPopUp("Create Entity");
                }

                //for (auto entity : PN_ECS_SERVICE->getAllEntities()) {
                //    auto entity_name = PN_METADATA_SERVICE->getEntityName(entity);
                //    auto entity_tags = PN_METADATA_SERVICE->getEntityTags(entity);
                //    if (entity_name == "" || (!tag_filter.empty() && entity_tags.find(tag_filter) == entity_tags.end())) continue;

                //    bool selected = PN_ECS_SERVICE->checkEntity(selected_entity) && entity == selected_entity;

                //    // Show selectable
                //    if (ImGui::Selectable((entity_name + "##Entity").c_str(), selected)) {
                //        // Check if currently editing grid
                //        //if (tilemap_panel.lock()->checkGridEditing()) {
                //        //    error_msg->assign("Editing grid now, unable to select entity.");
                //        //    openPopUp("Error");
                //        //    unselectEntity();
                //        //    break;
                //        //}

                //        // Prepare for redo/undo if the entity selection changes
                //        if (selected_entity != entity) {
                //            Action select_entity_action;

                //            auto prev_entity = selected_entity;
                //            auto new_entity = entity;

                //            select_entity_action.do_action = [&, prev_entity, new_entity]() {
                //                selected_entity = new_entity;
                //                b_entity_changed = true;
                //                };

                //            select_entity_action.undo_action = [&, prev_entity, new_entity]() {
                //                selected_entity = prev_entity;
                //                b_entity_changed = true;
                //                };

                //            command_manager->executeAction(std::move(select_entity_action));
                //        }
                //        // Unselect the entity
                //        else {
                //            Action unselect_entity_action;

                //            auto prev_entity = selected_entity;

                //            unselect_entity_action.do_action = [&, prev_entity]() {
                //                unselectEntity();
                //                b_entity_changed = true;
                //                };

                //            unselect_entity_action.undo_action = [&, prev_entity]() {
                //                selected_entity = prev_entity;
                //                b_entity_changed = true;
                //                };

                //            command_manager->executeAction(std::move(unselect_entity_action));
                //        }
                //    }

                //    //Start drag-and-drop entity
                //    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                //        ImGui::SetDragDropPayload(std::string("Selected Entity").c_str(), &entity, sizeof(ECS::Entity::Type));
                //        ImGui::Text("%s", std::string("Entity: " + PN_METADATA_SERVICE->getEntityName(entity)).c_str());
                //        ImGui::EndDragDropSource();
                //    }
                //    else {
                //        if (ImGui::IsItemHovered()) {
                //            ImGui::BeginTooltip();
                //            ImGui::Text("Drag entity to reorder or add tags.");
                //            ImGui::EndTooltip();
                //        }
                //    }

                //    //Drop Entity tag payload
                //    if (ImGui::BeginDragDropTarget()) {
                //        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(std::string("Entity Tag").c_str())) {
                //            std::string tag(static_cast<const char*>(payload->Data));
                //            PN_METADATA_SERVICE->addEntityTag(entity, tag);
                //        }
                //        ImGui::EndDragDropTarget();
                //    }
                //}

                // Show Remove and Clone buttons only when an entity is selected and valid
                if (PN_ECS_SERVICE->checkEntity(selected_entity)) {
                    ImGui::Spacing();
                    ImGui::Separator();

                    if (ImGui::Button("Remove Entity") || ImGui::IsKeyPressed(ImGuiKey_Delete)) {
                        openPopUp("Remove Entity");
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Clone Entity")) {
                        openPopUp("Clone Entity");
                    }
                }


                renderPopUps();
                ImGui::End();
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
#endif