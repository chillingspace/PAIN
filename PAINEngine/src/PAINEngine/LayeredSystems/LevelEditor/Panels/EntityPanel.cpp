#include "pch.h"
#include "EntityPanel.h"
#include "ECS/Controller.h"
#ifdef _DEBUG
namespace PAIN {
    namespace Editor {
        namespace Panel {

            EntityPanel::EntityPanel() {
                name = "Entity Panel";
                flags = ImGuiWindowFlags_None;
                
                // Seed with some entities for UI to be usable immediately
            }

            void EntityPanel::nextWindowSettings() {
                // No special behavior here
            }

            void EntityPanel::createEntity() {
                auto ecs = services->get<ECS::Controller>();

                int total_entities = ecs->getAllEntities().size();
                // For simplicity, just add a new entity with a generic name
                glm::vec3 pos = glm::vec3(1.f, 1.f, 1.f * total_entities);
                glm::quat rot = {0.f,0.f,0.f, 0.f};
                glm::vec3 scale = { 1.f, 1.f, 1.f };

                ECS::Entity::Type entity = ecs->createEntity();
                ecs->addEntityComponent(entity, Transform{ pos, rot, scale });
                ecs->addEntityComponent(entity, MeshRenderer{ Mesh::LoadObj() });
                

            }

            void EntityPanel::removeEntity() {
                if (selectedEntityIndex != -1) {
                    //auto scene = services->get<Scene>();
                    //scene->DeleteObject(selectedEntityIndex);
                }
            }

            void EntityPanel::onUpdate(PAIN::AppTiming timing) {

                // Update entity list
                auto ecs = services->get<ECS::Controller>();
                auto scene = services->get<Scene>();
                if (total_entities != ecs->getAllEntities().size()) {
                    total_entities = ecs->getAllEntities().size();
                    entities.clear();

                    for (int i = 0; i < total_entities; i++) {

                        std::string entity_name = "Entity " + std::to_string(i);
                        entities.push_back(entity_name);
                    }

                }

                if (ImGui::Begin("Entity Panel", nullptr, flags)) {
                    ImGui::Text("Entities:");

                    // Display a list of entities
                    for (size_t i = 0; i < entities.size(); ++i) {
                        bool isSelected = (i == selectedEntityIndex);
                        if (ImGui::Selectable(entities[i].c_str(), isSelected)) {
                            selectedEntityIndex = i;  // Select the entity
                        }
                    }

                    // Buttons to create or remove entities
                    if (ImGui::Button("Create Entity")) {
                        createEntity();
                    }

                    if (ImGui::Button("Remove Selected Entity") && selectedEntityIndex != -1) {
                        removeEntity();
                    }
                }
                ImGui::End();
            }

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
#endif