#pragma once

#ifdef _DEBUG
#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "Panels.h"
#include "CoreSystems/Scene/sCameraController.h"
#include "ImGuizmo.h"

// Forward declaration
namespace PAIN {
    namespace Editor {
        namespace Panel {
            class EntityPanel;
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

                bool wantsInput() const { return contentHovered && isFocused; }

                void setSimulationState(bool isPaused) {
                    isSimulationPaused = isPaused;
                }

                void setEntityPanel(std::shared_ptr<EntityPanel> panel) { m_EntityPanel = panel; }

            private:
                ImTextureID renderTexture;
                int texWidth, texHeight;

                bool isInputPaused;
                bool isSimulationPaused;

                bool contentHovered = false;
                bool isFocused = false;

                ImGuizmo::OPERATION m_GizmoOperation;
                ImGuizmo::MODE m_GizmoMode;

                entt::entity m_HoveredEntity = entt::null;
                entt::entity findEntityUnderMouse(ImVec2 localMousePos, ImVec2 viewportSize);

                std::shared_ptr<EntityPanel> m_EntityPanel;

                // Ray casting methods
                glm::vec3 screenToWorldRay(ImVec2 mousePos, ImVec2 viewportSize,
                    const glm::mat4& view, const glm::mat4& projection);
                glm::vec3 getCameraPosition(const glm::mat4& viewMatrix);
                void performMousePicking(ImVec2 localMousePos, ImVec2 viewportSize);
                bool rayIntersectsSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                    const glm::vec3& sphereCenter, float sphereRadius,
                    float& distance);
                bool rayIntersectsAABB(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                    const Transform& transform, float& distance);
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
#endif
