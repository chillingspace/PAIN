#include "pch.h"
#include "EntityPanel.h"

namespace PAIN {
    namespace Editor {
        namespace Panel {

            EntityPanel::EntityPanel() {
                name = "Entity Panel";
                flags = ImGuiWindowFlags_None;

                // Seed with some entities for UI to be usable immediately
                entities.push_back("Player");
                entities.push_back("Enemy1");
                entities.push_back("Tree");
            }

            void EntityPanel::nextWindowSettings() {
                // No special behavior here
            }

            void EntityPanel::createEntity() {
                // For simplicity, just add a new entity with a generic name
                entities.push_back("New Entity");
            }

            void EntityPanel::removeEntity() {
                if (selectedEntityIndex != -1) {
                    entities.erase(entities.begin() + selectedEntityIndex);
                    selectedEntityIndex = -1;  // Deselect after removal
                }
            }

            void EntityPanel::onUpdate() {
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
