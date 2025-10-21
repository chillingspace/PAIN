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

                            // For simplicity, just add a new entity with a generic name
                            glm::vec3 pos = glm::vec3(1.f, 1.f, 1.f);
                            glm::quat rot = { 0.f,0.f,0.f, 0.f };
                            glm::vec3 scale = { 1.f, 1.f, 1.f };

                            ECS::Entity::Type entity = ecs->createEntity();
                            ecs->addEntityComponent(entity, MetaData::EntityName{ entity_name });
                            ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
                            ecs->addEntityComponent(entity, MeshRenderer{ Mesh::LoadObj() });

                            };

                        //Undo Action
                        create.undo_action = [&, shared_id]() {
                            // Find entity by name and destroy it
                            auto entity = PN_METADATA_SERVICE->getEntityByName(*shared_id);
                            if (entity.has_value()) {
                                PN_ECS_SERVICE->destroyEntity(entity.value());
                            }
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
                        ECS::Entity::Type entity_to_remove = selected_entity;
                        //int layer_id = PN_METADATA_SERVICE->getEntityLayerID(selected_entity);

                        //Setup undo action for remove
                        remove.undo_action = [&, shared_id, entity_to_remove]() {

                                // TODO: Need use clone
                        };

                        //Setup action for removing entity
                        remove.do_action = [&, shared_id]() {

                            if (PN_ECS_SERVICE->checkEntity(entity_to_remove)) {
                                PN_ECS_SERVICE->destroyEntity(entity_to_remove);
                            }
                            selected_entity = ECS::Entity::INVALID;
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
                auto ecs = PN_ECS_SERVICE;
                auto ser = services->get<Serialization::Service>();

                // Detect scene changes
                bool sceneChanged = ser && ser->consumeSceneChanged();
                if (sceneChanged) {
                    selected_entity = ECS::Entity::INVALID;
                    selectedEntityIndex = -1;
                    editor_entities.clear();
                    total_entities = 0;
                    b_entity_changed = true;
                    PN_CORE_INFO("[EntityPanel] Scene changed detected, list reset");
                }

                // Get current entity count from ECS
                size_t current_count = static_cast<size_t>(ecs->getEntitiesCount());

                // Rebuild entity list if count changed
                if (total_entities != current_count) {
                    total_entities = current_count;
                    editor_entities.clear();

                    // Rebuild list by iterating entities with EntityName component
                    auto& registry = ecs->getRegistry();
                    auto view = registry.view<MetaData::EntityName>();

                    for (auto entity : view) {
                        auto& name_comp = view.get<MetaData::EntityName>(entity);
                        editor_entities.push_back({
                            static_cast<ECS::Entity::Type>(entity),
                            name_comp.name
                        });
                    }

                    // Verify our list matches the count (sanity check)
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

                // Render entity list
                for (size_t i = 0; i < editor_entities.size(); ++i) {
                    ECS::Entity::Type entity_id = editor_entities[i].first;
                    bool isSelected = (selected_entity == entity_id);
                    std::string label = editor_entities[i].second + "##" + std::to_string(entity_id);

                    if (ImGui::Selectable(label.c_str(), isSelected)) {
                        if (isSelected) {
                            // Deselect if already selected
                            selected_entity = ECS::Entity::INVALID;
                            selectedEntityIndex = -1;
                        }
                        else {
                            // Select entity
                            selected_entity = entity_id;
                            selectedEntityIndex = static_cast<int>(i);
                        }
                        b_entity_changed = true;
                    }

                    // Tooltip on hover
                    if (ImGui::IsItemHovered()) {
                        ImGui::BeginTooltip();
                        ImGui::Text("Entity ID: %u", static_cast<uint32_t>(entity_id));

                        // Show locked status if applicable
                        auto metadata = PN_METADATA_SERVICE;
                        if (metadata && metadata->isLocked(entity_id)) {
                            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "LOCKED");
                        }

                        ImGui::EndTooltip();
                    }
                }

                ImGui::Spacing();
                ImGui::Separator();

                // Add Entity button
                if (ImGui::Button("Add Entity", ImVec2(-1, 0))) {
                    openPopUp("Create Entity");
                }

                // Show Remove/Clone buttons when entity is selected
                if (ecs->checkEntity(selected_entity)) {
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::BeginGroup();

                    // Remove button (also works with Delete key)
                    if (ImGui::Button("Remove Entity", ImVec2(-1, 0)) ||
                        (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete))) {
                        openPopUp("Remove Entity");
                    }

                    // Clone button
                    if (ImGui::Button("Clone Entity", ImVec2(-1, 0))) {
                        openPopUp("Clone Entity");
                    }

                    ImGui::EndGroup();
                }

                // Render any open popups
                renderPopUps();

                ImGui::End();
            }


        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
#endif