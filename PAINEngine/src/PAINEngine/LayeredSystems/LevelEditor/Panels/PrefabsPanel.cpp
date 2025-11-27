#ifdef _DEBUG
#include "pch.h"
#include "PrefabsPanel.h"
#include "CoreSystems/Prefabs/sPrefab.h"
#include "ECS/Controller.h"
#include "ECS/Components/cEntity.h"
#include "ECS/Components/AllComponents.h"
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Serialization/sSerialization.h"

#include "LayeredSystems/LevelEditor/Editor.h"


namespace PAIN {
    namespace Editor {
        namespace Panel {

            void PrefabPanel::onDetach() {
                // Ensure cleanup if still in edit mode
                if (isInEditMode) {
                    exitEditMode(false); // Discard unsaved changes
                }
            }

            entt::entity PrefabPanel::findRootEntity(entt::registry& registry) {
                // Find entity with no parent (root of hierarchy)
                auto hierarchyView = registry.view<Entity::Hierarchy>();

                for (auto entity : hierarchyView) {
                    const auto& hierarchy = hierarchyView.get<Entity::Hierarchy>(entity);
                    if (!hierarchy.parentGUID.IsValid()) {
                        return entity;
                    }
                }
                // Fallback: return first entity with GUID
                auto guidView = registry.view<Entity::GUID>();
                if (!guidView.empty()) {
                    return *guidView.begin();
                }
                return entt::null;
            }

            bool PrefabPanel::enterEditMode(const Assets::GUID& prefabGUID) {
                if (isInEditMode) {
                    PN_CORE_WARN("[PrefabEditMode] Already in edit mode. Exit first.");
                    return false;
                }

                PN_CORE_INFO("[PrefabEditMode] Entering edit mode for prefab: {}", prefabGUID.ToString());

                // Get required services
                auto ecsController = services->get<ECS::Controller>();
                auto assetManager = services->get<Assets::Manager>();
                auto prefabService = services->get<Prefab::Service>();
                if (!ecsController || !assetManager || !prefabService) {
                    PN_CORE_ERROR("[PrefabEditMode] Required services not available");
                    return false;
                }

                //Load prefab asset
                auto prefabAssetOpt = assetManager->getAsset<Prefab::PrefabAsset>(prefabGUID);
                if (!prefabAssetOpt.has_value()) {
                    PN_CORE_ERROR("[PrefabEditMode] Prefab asset not found: {}", prefabGUID.ToString());
                    return false;
                }

                //Get prefab asset
                auto prefabAsset = prefabAssetOpt.value();

                //Set curr prefab name
                currentPrefabName = prefabAsset->prefabName.empty() ? prefabAsset->main_relative_path.stem().string() : prefabAsset->prefabName;

                //Create isolated registry for editing
                std::string registryName = "PrefabEdit_" + currentPrefabName;
                editRegistryID = ecsController->createRegistry(registryName, false);
                PN_CORE_INFO("[PrefabEditMode] Created edit registry: {} (ID: {})", registryName, editRegistryID);

                //Reference registry for editing
                auto& editRegistry = ecsController->getRegistry(editRegistryID);

                //Instantiate prefab into edit registry
                editRootEntity = prefabService->instantiatePrefab(prefabGUID, editRegistryID);
                if (editRootEntity == entt::null) {
                    PN_CORE_ERROR("[PrefabEditMode] Failed to instantiate prefab in edit registry");
                    ecsController->destroyRegistry(editRegistryID);
                    return false;
                }

                // IMPORTANT: Remove PrefabInstance components from all entities
                // We're editing the SOURCE prefab, not an instance
                auto prefabInstanceView = editRegistry.view<Prefab::PrefabInstance>();
                std::vector<entt::entity> entitiesToClean;

                for (auto entity : prefabInstanceView) {
                    entitiesToClean.push_back(entity);
                }
                for (auto entity : entitiesToClean) {
                    editRegistry.remove<Prefab::PrefabInstance>(entity);
                }
                PN_CORE_INFO("[PrefabEditMode] Removed {} PrefabInstance components", entitiesToClean.size());
                
                //Set up prefab editing state
                isInEditMode = true;
                currentEditingPrefabGUID = prefabGUID;
                hasUnsavedChanges = false;

                //Set the state of panels
                entity_panel.lock()->setRegistry(editRegistryID);
                comp_panel.lock()->setRegistry(editRegistryID);
                viewport_panel.lock()->setRegistry(editRegistryID);

                //Set only the prefab registry to simulate
                ecsController->setRegistryAutoSimulate(editRegistryID, true);
                ecsController->setRegistryAutoSimulate(ECS::MAIN_REGISTRY_ID, false);

                PN_CORE_INFO("[PrefabEditMode] Successfully entered edit mode. Root entity: {}", static_cast<uint32_t>(editRootEntity));
                return true;
            }

            bool PrefabPanel::exitEditMode(bool saveChanges) {
                if (!isInEditMode) {
                    PN_CORE_WARN("[PrefabEditMode] Not in edit mode");
                    return false;
                }

                //Start exiting
                PN_CORE_INFO("[PrefabEditMode] Exiting edit mode (save: {})", saveChanges);

                //Get controllers
                auto ecsController = services->get<ECS::Controller>();
                if (!ecsController) {
                    PN_CORE_ERROR("[PrefabEditMode] ECS Controller not available");
                    return false;
                }

                //Save changes if requested
                if (saveChanges && hasUnsavedChanges) {
                    if (!saveCurrentPrefab()) {
                        PN_CORE_ERROR("[PrefabEditMode] Failed to save prefab. Exit cancelled.");
                        return false;
                    }
                }

                //Destroy the edit registry to clean up resources
                ecsController->setRegistryAutoSimulate(editRegistryID, false);
                ecsController->destroyRegistry(editRegistryID);
                PN_CORE_INFO("[PrefabEditMode] Destroyed edit registry: {}", editRegistryID);

                //Reset state
                isInEditMode = false;
                currentEditingPrefabGUID = Assets::GUID();
                currentPrefabName.clear();

                //Reset main registry to simulate again
                editRegistryID = ECS::MAIN_REGISTRY_ID;
                ecsController->setRegistryAutoSimulate(editRegistryID, true);
                editRootEntity = entt::null;
                hasUnsavedChanges = false;

                //Set the state of panels
                entity_panel.lock()->setRegistry(editRegistryID);
                comp_panel.lock()->setRegistry(editRegistryID);
                viewport_panel.lock()->setRegistry(editRegistryID);

                PN_CORE_INFO("[PrefabEditMode] Successfully exited edit mode");
                return true;
            }

            bool PrefabPanel::saveCurrentPrefab() {
                if (!isInEditMode) {
                    PN_CORE_ERROR("[PrefabEditMode] Cannot save - not in edit mode");
                    return false;
                }

                //Get services
                auto ecsController = services->get<ECS::Controller>();
                auto assetManager = services->get<Assets::Manager>();
                auto prefabService = services->get<Prefab::Service>();
                auto path_service = services->get<Path::Path>();
                if (!ecsController || !assetManager || !prefabService || !path_service) {
                    PN_CORE_ERROR("[PrefabEditMode] Required services not available");
                    return false;
                }

                //Attempting to save prefab
                PN_CORE_INFO("[PrefabEditMode] Saving prefab: {}", currentPrefabName);

                //Reference editing registry
                auto& editRegistry = ecsController->getRegistry(editRegistryID);

                //Collect all entities in hierarchy
                std::vector<entt::entity> hierarchyEntities;
                prefabService->collectHierarchy(editRootEntity, hierarchyEntities, editRegistryID);
                PN_CORE_INFO("[PrefabEditMode] Collected {} entities to serialize", hierarchyEntities.size());

                //Create prefab
                Prefab::PrefabAsset updatedPrefab;
                updatedPrefab.rootEntityGUID = editRegistry.get<Entity::GUID>(editRootEntity).guid;
                updatedPrefab.name = currentPrefabName;
                updatedPrefab.prefabName = currentPrefabName;

                //Add entities into prefab
                for (auto e : hierarchyEntities) {
                    updatedPrefab.entities.push_back(prefabService->serializeEntity(e, editRegistryID));
                }

                //Craft path to prefab assets
                auto prefab_ext = *Assets::getAllExtensions()[Assets::Type::Prefabs].begin();
                auto prefab_folder = Assets::getAllGameFolders()[Assets::Type::Prefabs];
                std::string virt_path_to_prefab = path_service->aliasCombineRelative(Path::main_assets_alias, prefab_folder.string() + "/" + updatedPrefab.name + prefab_ext);

                //Save prefab to file
                prefabService->savePrefabToFile(updatedPrefab, virt_path_to_prefab, editRegistryID);

                //Reload asset in asset manager
                assetManager->reshipAsset(currentEditingPrefabGUID);

                //Propagate changes to instances in main registry
                propagateToInstances(true);
                hasUnsavedChanges = false;
                PN_CORE_INFO("[PrefabEditMode] Successfully saved prefab: {}", updatedPrefab.name);
                return true;
            }

            bool PrefabPanel::revertChanges() {
                if (!isInEditMode) {
                    PN_CORE_ERROR("[PrefabEditMode] Cannot revert - not in edit mode");
                    return false;
                }
                PN_CORE_INFO("[PrefabEditMode] Reverting changes to prefab: {}", currentPrefabName);
                //Baic approach
                Assets::GUID prefabGUID = currentEditingPrefabGUID;

                if (!exitEditMode(false)) {
                    return false;
                }
                return enterEditMode(prefabGUID);
            }

            void PrefabPanel::propagateToInstances(bool preserveOverrides) {
                if (!isInEditMode) {
                    PN_CORE_WARN("[PrefabEditMode] Not in edit mode - cannot propagate");
                    return;
                }

                //Get services
                auto ecsController = services->get<ECS::Controller>();
                auto prefabService = services->get<Prefab::Service>();
                if (!ecsController || !prefabService) return;
                PN_CORE_INFO("[PrefabEditMode] Propagating changes to instances (preserveOverrides: {})", preserveOverrides);
                
                //Get all instances in the active scene
                auto& mainRegistry = ecsController->getRegistry(ECS::MAIN_REGISTRY_ID);
                auto instances = prefabService->getInstancesOfPrefab(currentEditingPrefabGUID, ECS::MAIN_REGISTRY_ID);
                PN_CORE_INFO("[PrefabEditMode] Found {} instances to update", instances.size());


                // TODO: Implement instance update logic
                // This is complex and depends on your requirements:
                // - If preserveOverrides = true, apply prefab changes but keep instance overrides
                // - If preserveOverrides = false, reset all instances to match prefab exactly
                // 
                // For now, this is a placeholder. Full implementation would involve:
                // 1. For each instance, get its PrefabInstance component
                // 2. Reload prefab data from asset
                // 3. Apply prefab data to instance
                // 4. If preserveOverrides, re-apply componentOverrides on top
            }

            bool PrefabPanel::showUnsavedChangesDialog() {
                bool shouldDiscard = false;
                ImGui::OpenPopup("Unsaved Changes");
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal("Unsaved Changes", nullptr,
                    ImGuiWindowFlags_AlwaysAutoResize)) {

                    ImGui::Text("You have unsaved changes to this prefab.");
                    ImGui::Text("Are you sure you want to discard them?");
                    ImGui::Separator();
                    if (ImGui::Button("Discard Changes", ImVec2(120, 0))) {
                        shouldDiscard = true;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        showUnsavedWarning = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
                return shouldDiscard;
            }

            void PrefabPanel::render() {
                
                //Render draw asset selector to edit
                Assets::GUID temp_id = currentEditingPrefabGUID;
                if (DrawAssetSelectorField("Select A Prefab",
                    temp_id,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Prefabs),
                    services)) {

                    //Check if in edit mode
                    if (isInEditMode) exitEditMode(false);

                    //Enter into prefab editing mode
                    enterEditMode(temp_id);
                }

                //Skip if prefab is not valid
                if (!currentEditingPrefabGUID.IsValid() || !isInEditMode) {
                    ImGui::Text("Select a prefab above to edit.");
                    return;
                }

                //Render prefab info
                renderPrefabInfo();

                ImGui::Separator();

                //Render tool bar
                renderToolbar();

                // Handle unsaved warning dialog
                if (showUnsavedWarning) {
                    if (showUnsavedChangesDialog()) {
                        exitEditMode(false);
                        showUnsavedWarning = false;
                    }
                }
            }

            void PrefabPanel::renderToolbar() {

                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                // Left side: Prefab name and mode indicator
                ImGui::Text("PREFAB MODE");
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), " | ");
                ImGui::SameLine();
                ImGui::Text("Editing: %s", currentPrefabName.c_str());
                if (hasUnsavedChanges) {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), " *unsaved*");
                }
                ImGui::PopStyleColor();
                // Right side: Action buttons
                ImGui::SameLine(ImGui::GetWindowWidth() - 220);
                if (ImGui::Button("Save")) {
                    if (saveCurrentPrefab()) {
                        PN_CORE_INFO("Prefab saved successfully");
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Discard & Exit")) {
                    if (hasUnsavedChanges) {
                        showUnsavedWarning = true;
                    }
                    else {
                        exitEditMode(false);
                    }
                }
            }

            void PrefabPanel::renderPrefabInfo() {
                ImGui::Text("Prefab GUID: %s", currentEditingPrefabGUID.ToString().c_str());
                ImGui::Text("Root Entity: %u", static_cast<uint32_t>(editRootEntity));
            }

            void PrefabPanel::nextWindowSettings() {
            }

            void PrefabPanel::onAttach() {
                name = "Prefabs Panel";
                flags = ImGuiWindowFlags_None;

                //Init with reference to entity and comp panel
                entity_panel = services->get<Editor>()->getPanel<EntityPanel>();
                comp_panel = services->get<Editor>()->getPanel<ComponentsPanel>();
                viewport_panel = services->get<Editor>()->getPanel<ViewportPanel>();
            }

            void PrefabPanel::onUpdate(PAIN::AppTiming timing) {

                // Begin ImGui window
                if (ImGui::Begin(name.c_str())) {
                    dock_id = ImGui::GetWindowDockID();
                    ImGui::Spacing();

                    //Render
                    render();

                    renderPopUps();
                    ImGui::End();
                }
            }
        } 
    }
}
#endif