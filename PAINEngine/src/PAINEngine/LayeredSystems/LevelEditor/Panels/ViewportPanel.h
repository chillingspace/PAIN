/*****************************************************************/ /**
 * \file   ViewportPanel.h
 * \brief
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 *
 * \date   October 2025
 * All content  2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifdef _DEBUG
#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "Panels.h"
#include "CoreSystems/Scene/sCameraController.h"
#include "ImGuizmo.h"

#include "ComponentsPanel.h"
#include "EntityPanel.h"
#include "PrefabsPanel.h"

// Forward declarations - no include needed
namespace PAIN {
    namespace Editor {
        namespace Panel {
            struct File;  // Forward declare File here
        }
    }
}

namespace PAIN {
    namespace Editor {
        namespace Panel {

            class ViewportPanel : public IPanel {
            public:
                ViewportPanel();

                void onAttach() override;
                void onUpdate(PAIN::AppTiming timing) override;
                void nextWindowSettings() override;

                float getTimeScale() const;
                void setRenderTexture(ImTextureID texID, int width, int height);

                glm::vec3 getWorldPositionAtMouse(ImVec2 localMousePos, ImVec2 viewportSize, float defaultDistance);
                void handlePrefabDrop(File* prefabFile, ImVec2 localMousePos, ImVec2 viewportSize);
                void handleTemplateDrop(File* prefabFile, ImVec2 localMousePos, ImVec2 viewportSize);

                bool wantsInput() const { return contentHovered && isFocused; }

                void setRegistry(ECS::RegistryID registryID) {
                    currentRegistryID = registryID;
                }

                ECS::RegistryID getCurrentRegistry() const {
                    return currentRegistryID;
                }

            private:

                //Set registry ID
                ECS::RegistryID currentRegistryID = ECS::MAIN_REGISTRY_ID;
                ImTextureID renderTexture;
                int texWidth, texHeight;

                bool isInputPaused;
                bool isSimulationPaused;

                bool contentHovered = false;
                bool isFocused = false;

                ImGuizmo::OPERATION m_GizmoOperation;
                ImGuizmo::MODE m_GizmoMode;

                entt::entity m_HoveredEntity = entt::null;

                std::shared_ptr<EntityPanel> m_EntityPanel;
#ifdef PN_PLATFORM_WINDOWS
                std::shared_ptr<ComponentsPanel> m_comp_panel;
                std::shared_ptr<PrefabPanel> m_prefab_panel;
#endif

                entt::entity m_DragHoveredEntity = entt::null;  // Add this
                entt::entity findEntityAtMousePos(ImVec2 localMousePos, ImVec2 viewportSize);

                // Ray casting methods
                glm::vec3 screenToWorldRay(ImVec2 mousePos, ImVec2 viewportSize,
                    const glm::mat4& view, const glm::mat4& projection);
                glm::vec3 getCameraPosition(const glm::mat4& viewMatrix);
                void performMousePicking(ImVec2 localMousePos, ImVec2 viewportSize);

                // Use forward declared File struct
                void handleMaterialDrop(File* materialFile,
                    ImVec2 localMousePos,
                    ImVec2 viewportSize);

                entt::entity pendingDeleteEntity = entt::null;
                std::string pendingDeleteName;
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif
