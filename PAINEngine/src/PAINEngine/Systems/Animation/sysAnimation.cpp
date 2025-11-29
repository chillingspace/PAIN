/*****************************************************************//**
 * \file   sysAnimation.cpp
 * \brief  Definition of animation system states
 *
 * \author Nicole Esther Lee, 2301544, [lee.n@digipen.edu](mailto:lee.n@digipen.edu) (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysAnimation.h"
#include "ECS/Controller.h"
#include "ECS/Components/cAnimation.h"
#include "ECS/Components/cMeshRenderer.h"

namespace PAIN {

	namespace AnimationSystem {

		void System::animationSetup()
		{
			// Initialize animation-specific data structures
			// Setup animation state machines, blend trees
			// Register animation component types if needed

			b_animation_enabled = true;

		}

		System::System(std::shared_ptr<Services> svc)
			: ECS::System::ISystem(svc)
			, c_max_animated_entities(1000)
			, c_animation_blend_speed(5.0f)
			, b_animation_enabled(true)
			, global_time_scale(1.0f)
		{
			animationSetup();
		}


		System::~System()
		{
			// Cleanup animation resources
			// Clear animation data, skeleton caches, etc.

		}

		void System::onUpdate(AppTiming timing, entt::registry& reg)
		{
			if (!b_animation_enabled) {
				return;
			}

			// Apply global time scale to delta time
			float scaledDeltaTime = timing.dt * global_time_scale;

			// Update all animations
			updateAllAnimations(scaledDeltaTime, reg);
		}

		void System::updateAllAnimations(float deltaTime, entt::registry& reg) {
			// Get all entities with BOTH Animation AND ModelRenderer components
			// Optimized using group
			auto group = reg.group<Animation>(entt::get<ModelRenderer>);

			// Count entities
			size_t entityCount = 0;
			for (auto e : group) { entityCount++; }


			// Iterate and update each animated entity
			for (auto [entity, anim, renderer] : group.each()) {
				
				// Skip if not playing or no model
				if (!anim.isPlaying) {
					continue;
				}

				if (!renderer.cachedModelAsset) {
					continue;
				}

				// Check animation index is valid
				if (anim.currentAnimationIndex < 0 ||
					anim.currentAnimationIndex >= static_cast<int>(renderer.cachedModelAsset->animations.size())) {

					continue;
				}

				// Get current animation data
				const auto& animData = renderer.cachedModelAsset->animations[anim.currentAnimationIndex];

				// Advance time
				anim.animationTime += deltaTime * anim.playbackSpeed;

				// Handle end of animation
				if (anim.animationTime >= animData.duration) {
					if (anim.loopAnimation) {
						anim.animationTime = fmod(anim.animationTime, animData.duration);
					}
					else {
						anim.animationTime = animData.duration;
						anim.isPlaying = false;
					}
				}

				computeBoneTransforms(entity, anim, renderer, animData);

				renderer.currentAnimationIndex = anim.currentAnimationIndex;
				renderer.animationTime = anim.animationTime;
				renderer.isPlaying = anim.isPlaying;
				renderer.loopAnimation = anim.loopAnimation;
				renderer.playbackSpeed = anim.playbackSpeed;

				if (!anim.boneTransforms.empty()) {
					renderer.boneTransforms = anim.boneTransforms;
				}
				if (!anim.morphWeights.empty()) {
					renderer.morphWeights = anim.morphWeights;
				}

			}
		}

		void System::computeBoneTransforms(entt::entity entity, Animation& anim, ModelRenderer& renderer, const Assets::AnimationClip& animData) {
			// Get skeleton from model
			if (!renderer.cachedModelAsset || renderer.cachedModelAsset->skeleton.empty()) {
				return;
			}

			const auto& skeleton = renderer.cachedModelAsset->skeleton;


			// Resize bone transform array if needed
			if (anim.boneTransforms.size() != skeleton.size()) {
				anim.boneTransforms.resize(skeleton.size(), glm::mat4(1.0f));
			}

			// Sample animation at current time
			float time = anim.animationTime;

			// For each bone in skeleton
			for (size_t i = 0; i < skeleton.size(); ++i) {
				const auto& bone = skeleton[i];

				// Find this bone's animation track
				auto trackIt = animData.track_map.find(bone.name);

				if (trackIt != animData.track_map.end()) {
					const auto& track = trackIt->second;

					// Sample the track at current time (simple linear interpolation)
					glm::vec3 translation = glm::vec3(0.0f);
					glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
					glm::vec3 scale = glm::vec3(1.0f);

					if (!track.empty()) {
						// Find keyframes around current time
						size_t keyIndex = 0;
						for (size_t k = 0; k < track.size() - 1; ++k) {
							if (track[k].time <= time && time < track[k + 1].time) {
								keyIndex = k;
								break;
							}
						}

						// Simple: just use the nearest keyframe (no interpolation for now)
						translation = track[keyIndex].translation;
						rotation = track[keyIndex].rotation;
						scale = track[keyIndex].scale;
					}

					// Build bone transform matrix
					glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
					glm::mat4 R = glm::mat4_cast(rotation);
					glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

					anim.boneTransforms[i] = T * R * S;
				}
				else {
					// No animation track for this bone, use identity
					anim.boneTransforms[i] = glm::mat4(1.0f);
				}
			}

		}


		void System::onEvent(Event::Event& e)
		{
			// Handle animation-related events
			// e.g., animation finished, state changed, blend completed

			// Example event handling structure:
			// Event::EventDispatcher dispatcher(e);
			// dispatcher.dispatch<Event::AnimationFinishedEvent>(PN_BIND_EVENT_FN(System::onAnimationFinished));
		}

		void System::enableAnimation(bool enable)
		{
			b_animation_enabled = enable;
		}

		bool System::isAnimationEnabled() const
		{
			return b_animation_enabled;
		}

		void System::setGlobalTimeScale(float scale) {
			global_time_scale = glm::max(0.0f, scale); // Prevent negative time scale
		}

		float System::getGlobalTimeScale() const {
			return global_time_scale;
		}

	}
}
