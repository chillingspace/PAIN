#pragma once

#include "Panels.h"
#include "CoreSystems/Scene/Scene.h"

#include "ECS/Controller.h"
#include "LayeredSystems/LevelEditor/Command.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "CoreSystems/Assets/sAssets.h"

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

            class EntityPanel : public IPanel {
            public:
                EntityPanel();


                void onAttach() override;
                void onUpdate(PAIN::AppTiming timing) override;

                void nextWindowSettings() override;

                //Get selected entity
                entt::entity getSelectedEntity() const;

                void setSelectedEntity(entt::entity entity);

                //Unselect entity
                void unselectEntity();

                //Check entity changed
                bool isEntityChanged() const;

            private:
                std::vector<std::pair<entt::entity, std::string>> editor_entities;

                //Selected entity
                entt::entity selected_entity;

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

                std::function<void()> removeEntityPopUp(std::string const& popup_id);

                //Clone entity popup
                std::function<void()> cloneEntityPopUp(std::string const& popup_id);

                char search_buffer[256] = "";
                bool sort_alphabetically = false;
                bool force_refresh = false;

                // Helper methods for hierarchy
                void drawEntityNode(entt::entity entity);
                std::vector<entt::entity> getRootEntities();
                std::vector<entt::entity> getEntityChildren(entt::entity parent);
                std::string getEntityName(entt::entity entity);
                void setEntityParent(entt::entity child, entt::entity parent);
                bool isAncestor(entt::entity potential_ancestor, entt::entity entity);
                void removeEntityWithChildren(entt::entity entity);
                void cloneEntityChildren(entt::entity source, entt::entity cloned_parent);

                // Prefab helper functions
                std::string generateUniquePrefabName(const std::string& base_name);
                void collectEntityHierarchy(entt::entity entity, std::vector<entt::entity>& out_entities);
                void ungroupEntity(entt::entity entity);
                void unparentEntity(entt::entity entity);

                std::function<void()> createEmptyEntityPopUp(std::string const& popup_id);
                std::function<void()> createChildEntityPopUp(std::string const& popup_id);

                entt::entity entity_pending_delete = entt::null;

                // Member variables
                std::vector<entt::entity> multi_selected_entities;

                // Member functions
                std::function<void()> groupEntitiesPopUp(std::string const& popup_id);

                void drawEntityHierarchy(entt::entity entity_id, int depth);
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
#endif