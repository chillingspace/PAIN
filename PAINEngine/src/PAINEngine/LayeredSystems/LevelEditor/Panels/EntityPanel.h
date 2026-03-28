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
                void nextWindowSettings() override {}

                // Core API
                entt::entity getSelectedEntity() const;
                void setSelectedEntity(entt::entity entity);
                void unselectEntity();
                bool isEntityChanged() const;

                bool isEntityAndScriptSwitched() const;
                void setEntityAndScriptSwitched(bool is_switched);

                void setRegistry(ECS::RegistryID registryID) {
                    currentRegistryID = registryID;
                }

                ECS::RegistryID getCurrentRegistry() const {
                    return currentRegistryID;
                }

            private:

                //Set registry ID
                ECS::RegistryID currentRegistryID = ECS::MAIN_REGISTRY_ID;

                // State
                std::vector<std::pair<entt::entity, std::string>> editor_entities;

                //Selected entity
                entt::entity selected_entity;

                //Error msg
                std::shared_ptr<std::string> error_msg;

                //Selected tag
                std::string selected_tag;

                //Entity changed event boolean
                bool b_entity_changed;
                bool b_entity_script_switched = false;

                int total_entities;
                int selectedEntityIndex = -1;       // Selected entity index

                char search_buffer[256] = "";
                bool sort_alphabetically = false;
                bool force_refresh = false;
                std::set<entt::entity> collapsed_groups;  // Tracks which groups are collapsed

                // Popup functions
                std::function<void(std::any const&)> createEntityPopUp(std::string const& popup_id);
                std::function<void(std::any const&)> removeEntityPopUp(std::string const& popup_id);
                std::function<void(std::any const&)> cloneEntityPopUp(std::string const& popup_id);

                // Core hierarchy operations (GUID-based)
                void drawEntityHierarchy(entt::entity entity, int depth, const std::string& search_filter = "");
                std::vector<entt::entity> getRootEntities();
                std::vector<entt::entity> getEntityChildren(entt::entity parent);

                // Search helper
                bool entityMatchesSearch(entt::entity entity, const std::string& search_filter);

                void collapseAll();
                void expandAll();

                void setEntityParent(entt::entity child, entt::entity parent);
                void removeParent(entt::entity child);
                bool isAncestor(entt::entity potential_ancestor, entt::entity entity);

                void removeEntityWithChildren(entt::entity entity);
                void cloneEntityWithChildren(entt::entity source, entt::entity cloned_parent);
                void ungroupEntity(entt::entity entity);

                // Siblings index (ordering)
                void normalizeSiblings(std::vector<entt::entity>& siblings);
                std::vector<entt::entity> getSiblings(entt::entity e);

                // Prefab helpers
#ifdef PN_PLATFORM_WINDOWS
                std::string generateUniquePrefabName(const std::string& base_name);
                std::string generateUniqueTemplateName(const std::string& base_name);
#endif
                void collectEntityHierarchy(entt::entity entity, std::vector<entt::entity>& out_entities);

                // Utility
                std::string getEntityName(entt::entity entity);
            };

        }
    }
}

#endif
