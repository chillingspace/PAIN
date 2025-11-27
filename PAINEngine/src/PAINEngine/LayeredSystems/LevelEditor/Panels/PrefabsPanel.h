#pragma once

#ifndef PREFAB_PANEL_HPP
#define PREFAB_PANEL_HPP

#include "AssetData.h"
#include "CoreSystems/Assets/Types/Prefab.h"
#include "ECS/Controller.h"
#include "EntityPanel.h"
#include "ComponentsPanel.h"

namespace PAIN {
    namespace Editor {
        namespace Panel {

            //Prefab panel implementation
            class PrefabPanel : public IPanel {
            private:

                //Reference to the entity and component panel
                std::weak_ptr<EntityPanel> entity_panel;
                std::weak_ptr<ComponentsPanel> comp_panel;

                // Current state
                bool isInEditMode = false;
                Assets::GUID currentEditingPrefabGUID;
                std::string currentPrefabName;
                ECS::RegistryID editRegistryID = ECS::MAIN_REGISTRY_ID;
                entt::entity editRootEntity = entt::null;

                // Track if there are unsaved changes
                bool hasUnsavedChanges = false;

                //Boolean for unsaved prefabs
                bool showUnsavedWarning = false;

                // Helper: Find root entity in the edit registry
                entt::entity findRootEntity(entt::registry& registry);

                //Entering editing mode
                bool enterEditMode(const Assets::GUID& prefabGUID);

                //Exit editing mode
                bool exitEditMode(bool saveChanges = true);

                //Save curr prefab
                bool saveCurrentPrefab();

                //Revert changes to the prefab
                bool revertChanges();

                //Propogate to instances
                void propagateToInstances(bool preserveOverrides = true);

                //Show unsaved changes
                bool showUnsavedChangesDialog();

                //Render tool bar
                void renderToolbar();

                //Render prefab info
                void renderPrefabInfo();

                //Render
                void render();
            public:
                PrefabPanel() = default;
                ~PrefabPanel() override = default;

                void nextWindowSettings() override;
                void onAttach() override;
                void onDetach() override;
                void onUpdate(PAIN::AppTiming timing) override;
            };
        }
    }
}
#endif 