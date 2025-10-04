#pragma once

#include "Panels.h"
#include "CoreSystems/Scene/Scene.h"

#include "ECS/Controller.h"
#include "LayeredSystems/LevelEditor/Command.h"
#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

            class EntityPanel : public IPanel {
            public:
                EntityPanel();
                void onUpdate(PAIN::AppTiming timing) override;
                void nextWindowSettings() override;

                // Button actions
                void createEntity();
                void removeEntity();

                //Get selected entity
                ECS::Entity::Type getSelectedEntity() const;

                //Unselect entity
                void unselectEntity();

                //Check entity changed
                bool isEntityChanged() const;

            private:
                std::vector<std::pair<ECS::Entity::Type, std::string>> editor_entities;

                //Selected entity
                ECS::Entity::Type selected_entity;

                //Error msg
                std::shared_ptr<std::string> error_msg;

                //Selected tag
                std::string selected_tag;

                //Entity changed event boolean
                bool b_entity_changed;


                int total_entities;
                int selectedEntityIndex = -1;       // Selected entity index


                //Add tag popup
                std::function<void()> addTagPopUp(std::string const& popup_id);

                //Remove tag popup
                std::function<void()> removeTagPopUp(std::string const& popup_id);

                //Create entity popup
                std::function<void()> createEntityPopUp(std::string const& popup_id);

                //Remove Entity popup
                std::function<void()> removeEntityPopUp(std::string const& popup_id);

                //Clone entity popup
                std::function<void()> cloneEntityPopUp(std::string const& popup_id);
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
#endif