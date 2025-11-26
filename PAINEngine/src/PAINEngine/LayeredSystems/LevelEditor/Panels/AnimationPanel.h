#pragma once

#include "pch.h"
#include "Panels.h"

#ifdef _DEBUG

namespace PAIN {
    namespace Editor {
        namespace Panel {

            class AnimationPanel : public IPanel {
            public:
                AnimationPanel();
                ~AnimationPanel() override = default;

                void onUpdate(PAIN::AppTiming timing) override;
                void SetSelectedEntity(entt::entity entity);  // Changed from ECS::Entity

            private:
                void RenderAnimationList();
                void RenderAnimationControls();
                void RenderTimelineEditor();
                void RenderAnimatedEntitiesList();

                entt::entity m_SelectedEntity = entt::null;  // Changed from ECS::NULL_ENTITY
                float m_TimelineZoom = 1.0f;
                float m_TimelineScroll = 0.0f;
                bool m_IsPlaying = false;
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN

#endif
