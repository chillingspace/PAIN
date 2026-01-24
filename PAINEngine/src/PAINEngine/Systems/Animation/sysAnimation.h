/*****************************************************************//**
 * \file   sysAnimation.h
 * \brief  Declaration of animation system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifndef SYS_ANIMATION_H
#define SYS_ANIMATION_H

#include "pch.h"
#include "ECS/System/ISystem.h"
#include "ECS/Components/cMeshRenderer.h"

namespace PAIN {

	namespace AnimationSystem {

		struct LocalPose {
			std::vector<glm::vec3> translations;
			std::vector<glm::quat> rotations;
			std::vector<glm::vec3> scales;
			std::vector<float> morphWeights;

			void Resize(size_t boneCount, size_t morphCount = 0) {
				translations.resize(boneCount);
				rotations.resize(boneCount);
				scales.resize(boneCount);
				if (morphCount > 0) morphWeights.resize(morphCount);
			}
		};

		class System : public ECS::System::ISystem
		{
		public:
			explicit System(std::shared_ptr<Services> svc);
			~System();

			// Virtual override methods for system lifecycle
			void onUpdate(AppTiming timing, entt::registry& reg) override;

			// Event handler for app layer
			void onEvent(Event::Event& e) override;
			std::string getSysName() const override { return "Animation System"; }

			// Helper methods for external control
			void enableAnimation(bool enable);
			bool isAnimationEnabled() const;

			// System-level time control
			void setGlobalTimeScale(float scale);
			float getGlobalTimeScale() const;

		private:

			// Animation system configuration values
			const int c_max_animated_entities;
			const float c_animation_blend_speed; // Blending speed for transitions

			// Animation state control
			bool b_animation_enabled;
			float global_time_scale; // Multiplier for all animations (1.0 = normal speed)

			// Animation initialization setup
			void animationSetup();

			// Core animation update logic
			void updateAllAnimations(float deltaTime, entt::registry& reg);
			void computeBoneTransforms(entt::entity entity, Animation& anim, ModelRenderer& renderer, const Assets::AnimationClip& animData);
			LocalPose SampleAnimation(const Assets::Model* model, int animIndex, float time);
			LocalPose BlendPoses(const LocalPose& poseA, const LocalPose& poseB, float weight);
			std::vector<glm::mat4> ConvertPoseToMatrices(const LocalPose& pose, const std::vector<Assets::Bone>& skeleton);
		};
	}

}

#endif
