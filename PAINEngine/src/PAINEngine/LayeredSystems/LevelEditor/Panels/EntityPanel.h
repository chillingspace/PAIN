/*****************************************************************//**
 * \file   EntityPanel.h
 * \brief  Entity hierarchy panel with GUID-based parenting
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   November 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

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

                // Core API
                entt::entity getSelectedEntity() const;
                void setSelectedEntity(entt::entity entity);
                void unselectEntity();
                bool isEntityChanged() const;

            private:
                // State
                std::vector<std::pair<entt::entity, std::string>> editor_entities;
                entt::entity selected_entity = entt::null;
                bool b_entity_changed = false;
                int total_entities = 0;
                int selectedEntityIndex = -1;
                bool sort_alphabetically = false;
                bool force_refresh = false;
                char search_buffer[256] = "";

                // Popup functions
                std::function<void(std::any const&)> createEntityPopUp(std::string const& popup_id);
                std::function<void(std::any const&)> removeEntityPopUp(std::string const& popup_id);
                std::function<void(std::any const&)> cloneEntityPopUp(std::string const& popup_id);

                // Core hierarchy operations (GUID-based)
                void drawEntityHierarchy(entt::entity entity, int depth);
                std::vector<entt::entity> getRootEntities();
                std::vector<entt::entity> getEntityChildren(entt::entity parent);

                void setEntityParent(entt::entity child, entt::entity parent);
                void removeParent(entt::entity child);
                bool isAncestor(entt::entity potential_ancestor, entt::entity entity);

                void removeEntityWithChildren(entt::entity entity);
                void cloneEntityWithChildren(entt::entity source, entt::entity cloned_parent);
                void ungroupEntity(entt::entity entity);

                // Prefab helpers
                std::string generateUniquePrefabName(const std::string& base_name);
                void collectEntityHierarchy(entt::entity entity, std::vector<entt::entity>& out_entities);

                // Utility
                std::string getEntityName(entt::entity entity);
            };

        }
    }
}

#endif
