#pragma once

#include "pch.h"
#include "LayeredSystems/LevelEditor/EditorAttributes.h"

namespace PAIN {

    // Animation component - contains animation state for an entity
    struct Animation {
        
        //Serialization flag
        static constexpr bool ShouldSerialize = true;

        // Playback state
        int currentAnimationIndex = -1;
        float animationTime = 0.0f;
        bool isPlaying = false;
        bool loopAnimation = true;
        float playbackSpeed = 1.0f;

        // Blending state
        int nextAnimationIndex = -1;
        float transitionWeight = 0.f;
        float transitionDuration = 0.2f;
        
        // Computed animation data (DO NOT SERIALIZE)
        std::vector<glm::mat4> boneTransforms;
        std::vector<float> morphWeights;

        // Constructors
        Animation() = default;

        // Playback control
        void PlayAnimation(int animIndex, bool loop = true, float speed = 1.0f) {
            currentAnimationIndex = animIndex;
            animationTime = 0.0f;
            isPlaying = true;
            loopAnimation = loop;
            playbackSpeed = speed;
        }

        // Transition between animation
        void CrossFade(int targetIndex, float duration = 2.f) {
            // return if already playing
            if (currentAnimationIndex == targetIndex && nextAnimationIndex == -1) return;
            // return if already transitioning to this animation
            if (nextAnimationIndex == targetIndex) return;
            // snap to this animation
            if (nextAnimationIndex != -1) {
                currentAnimationIndex = nextAnimationIndex;
                animationTime = 0.0f;
            }

            nextAnimationIndex = targetIndex;
            transitionDuration = duration;
            transitionWeight = 0.f;
            isPlaying = true;
        }

        void Stop() {
            isPlaying = false;
            animationTime = 0.0f;
        }

        void Pause() {
            isPlaying = false;
        }

        void Resume() {
            isPlaying = true;
        }
    };

} // namespace PAIN

// ============================================
// REFLECTION (Editor Integration)
// ============================================
REFL_TYPE(PAIN::Animation)
REFL_FIELD(currentAnimationIndex, PAIN::Editor::Attributes::DisplayName("Animation Index"))
REFL_FIELD(nextAnimationIndex, PAIN::Editor::Attributes::DisplayName("Next Anim (Debug)"))
REFL_FIELD(transitionWeight, PAIN::Editor::Attributes::DisplayName("Blend Weight"))
REFL_FIELD(animationTime, PAIN::Editor::Attributes::DisplayName("Current Time"))
REFL_FIELD(isPlaying, PAIN::Editor::Attributes::DisplayName("Is Playing"))
REFL_FIELD(loopAnimation, PAIN::Editor::Attributes::DisplayName("Loop"))
REFL_FIELD(playbackSpeed, PAIN::Editor::Attributes::Range(0.1f, 5.0f), PAIN::Editor::Attributes::DisplayName("Speed"))
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Animation>);
