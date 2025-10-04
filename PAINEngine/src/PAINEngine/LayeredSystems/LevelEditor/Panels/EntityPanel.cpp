#include "pch.h"
#include "EntityPanel.h"

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
                auto scene = services->get<Scene>();
                int total_entities = scene->GetObjects().size();
                // For simplicity, just add a new entity with a generic name
                glm::mat4 transform = glm::translate(glm::mat4(1.f), glm::vec3(1.f, 1.f, 1.f * total_entities));

                if (scene) {
                    scene->AddObject(Mesh::LoadObj(), transform);
                }

            }

            void EntityPanel::removeEntity() {
                if (selectedEntityIndex != -1) {
                    auto scene = services->get<Scene>();
                    scene->DeleteObject(selectedEntityIndex);
                }
            }

            void EntityPanel::onUpdate(PAIN::AppTiming timing) {

                // Update entity list
                auto scene = services->get<Scene>();
                if (total_entities != scene->GetObjects().size()) {
                    total_entities = scene->GetObjects().size();
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
