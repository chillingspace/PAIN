#include "pch.h"
#include "sysUIAnimation.h"

namespace PAIN {

    namespace UI {

        AnimationSystem::AnimationSystem(std::shared_ptr<Services> svc) : ISystem(svc)
        {
        }

        AnimationSystem::~AnimationSystem()
        {
        }
        void AnimationSystem::onUpdate(AppTiming timing, entt::registry& registry) {

            auto view = registry.group<UIAnimation>(entt::get<UIRectTransform>);

            for (auto&& [entity, ani_comp, rect_transform] : view.each()) {
                ani_comp = view.get<UIAnimation>(entity);
                rect_transform = view.get<UIRectTransform>(entity);

                if (!ani_comp.b_playing) continue;

                ani_comp.elapsed += timing.dt;
                float t = glm::clamp(ani_comp.elapsed / ani_comp.duration, 0.0f, 1.0f);
                t = easeInOut(t);

                switch (ani_comp.anim_type) {
                case PAIN::AnimationType::Position:
                    rect_transform.local_position = glm::mix(ani_comp.start_vec3, ani_comp.end_vec3, t);
                    break;
                case PAIN::AnimationType::Scale:
                    rect_transform.scale = glm::mix(ani_comp.start_vec3, ani_comp.end_vec3, t);
                    break;
                case PAIN::AnimationType::Rotation:
                    // Interpolate quaternion
                    break;
                }

                // Check completion
                if (ani_comp.elapsed >= ani_comp.duration) {
                    if (ani_comp.b_loop) {
                        ani_comp.elapsed = 0.0f;
                    }
                    else {
                        ani_comp.b_playing = false;
                    }
                }
            }
        }

        float AnimationSystem::easeInOut(float t) {
            return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
        }

    }
}