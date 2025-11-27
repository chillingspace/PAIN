#pragma once

#ifndef PREFAB_PANEL_HPP
#define PREFAB_PANEL_HPP

#include "AssetData.h"
#include "CoreSystems/Assets/Types/Prefab.h"
#include "ECS/Controller.h"

namespace PAIN {
    namespace Editor {
        namespace Panel {

            //Prefab panel implementation
            class PrefabPanel : public IPanel {
            private:

                // Current state
                bool isInEditMode = false;
                Assets::GUID currentEditingPrefabGUID;
                std::string currentPrefabName;
                ECS::RegistryID editRegistryID = ECS::MAIN_REGISTRY_ID;
                entt::entity editRootEntity = entt::null;

                //OG Prefab data
                nlohmann::json originalPrefabData;

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

                //Prefab checker
                bool hasPrefabChanged() const;

                //Save curr prefab
                bool saveCurrentPrefab();

                //Revert changes to the prefab
                bool revertChanges();

                //Propogate to instances
                void propagateToInstances(bool preserveOverrides = true);

                //Show unsaved changes
                void showUnsavedChangesDialog();

                //Render tool bar
                void renderToolbar();

                //Render prefab info
                void renderPrefabInfo();

                //Render instance tools
                void renderInstanceTools();

                //Render
                void render();
            public:
                PrefabPanel() = default;
                ~PrefabPanel() override = default;

                //Get in edit mode
                bool getEditingMode() const { return isInEditMode; }

                //Get edit registry id
                ECS::RegistryID getEditRegistryID() const { return editRegistryID; }

                void nextWindowSettings() override;
                void onAttach() override;
                void onDetach() override;
                void onUpdate(PAIN::AppTiming timing) override;
            };
        }
    }
}
#endif 