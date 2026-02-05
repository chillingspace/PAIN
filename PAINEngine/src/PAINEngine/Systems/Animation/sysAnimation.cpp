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
			auto group = reg.group<Animation>(entt::get<ModelRenderer>);

			for (auto [entity, anim, renderer] : group.each()) {

				// Skip if model missing or no animations
				if (!renderer.cachedModelAsset || renderer.cachedModelAsset->animations.empty()) continue;

				// Check for valid index
				if (anim.currentAnimationIndex < 0 || anim.currentAnimationIndex >= renderer.cachedModelAsset->animations.size()) {
					int targetIndex = 0;

					// Use default if there is one
					if (anim.defaultAnimationIndex >= 0 &&
						anim.defaultAnimationIndex < renderer.cachedModelAsset->animations.size()) {
						targetIndex = anim.defaultAnimationIndex;
					}

					// Play animations immediately
					anim.PlayAnimation(targetIndex, anim.loopAnimation, anim.playbackSpeed);
				}

				// Skip if paused
				if (!anim.isPlaying) continue;

				float duration = renderer.cachedModelAsset->animations[anim.currentAnimationIndex].duration;
				if (duration <= 0.001f) duration = 1.0f; // Prevent divide by zero

				anim.animationDuration = duration;

				// Progress the animation time
				anim.animationTime += deltaTime * anim.playbackSpeed;

				// Handle looping / stopping of animation
				if (anim.animationTime >= duration) {
					if (anim.loopAnimation) {
						//looping
						anim.animationTime = fmod(anim.animationTime, duration);
					}
					else {
						//stop
						anim.animationTime = duration;
					}
				}

				// Sample Current Animation
				LocalPose finalPose = SampleAnimation(renderer.cachedModelAsset.get(), anim.currentAnimationIndex, anim.animationTime);

				// Handle Blending of different animation states
				if (anim.nextAnimationIndex != -1) {

					// Check if next index is valid
					if (anim.nextAnimationIndex >= 0 && anim.nextAnimationIndex < renderer.cachedModelAsset->animations.size())
					{
						// Update Weight
						anim.transitionWeight += deltaTime / anim.transitionDuration;

						float nextDuration = renderer.cachedModelAsset->animations[anim.nextAnimationIndex].duration;
						if (nextDuration <= 0.001f) nextDuration = 1.0f;

						float normalizedTime = anim.animationTime / duration;
						float nextTime = normalizedTime * nextDuration;
						

						// Sample Next Animation (Using same time for sync - simplistic but works for now)
						LocalPose nextPose = SampleAnimation(renderer.cachedModelAsset.get(), anim.nextAnimationIndex, anim.animationTime);

						// Mix them
						float w = glm::clamp(anim.transitionWeight, 0.0f, 1.0f);
						finalPose = BlendPoses(finalPose, nextPose, w);

						// Check if done
						if (anim.transitionWeight >= 1.0f) {
							anim.currentAnimationIndex = anim.nextAnimationIndex;
							anim.nextAnimationIndex = -1;
							anim.transitionWeight = 0.0f;
						}
					}
					else {
						anim.nextAnimationIndex = -1;
						anim.transitionWeight = 0.0f;
					}
				}

				// convert all to matrices
				anim.boneTransforms = ConvertPoseToMatrices(finalPose, renderer.cachedModelAsset->skeleton);

				// set bones in renderer
				renderer.boneTransforms = anim.boneTransforms;
			}
		}

		// Returns pose for a specific time of the animation
		LocalPose System::SampleAnimation(const Assets::Model* model, int animIndex, float time)
		{
			LocalPose pose;
			if (!model || animIndex < 0 || animIndex >= model->animations.size()) return pose;

			const auto& animClip = model->animations[animIndex];
			const auto& skeleton = model->skeleton;

			pose.Resize(skeleton.size());

			for (size_t i = 0; i < skeleton.size(); ++i) {
				const auto& bone = skeleton[i];

				// Defaults (Identity)
				glm::vec3 translation(0.0f);
				glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
				glm::vec3 scale(1.0f);

				auto it = animClip.track_map.find(bone.name);
				if (it != animClip.track_map.end()) {
					const auto& track = it->second;

					if (!track.empty()) {
						// if only one track
						if (track.size() == 1) {
							translation = track[0].translation;
							rotation = track[0].rotation;
							scale = track[0].scale;
						}
						// if time is past the end
						else if (time >= track.back().time) {
							const auto& last = track.back();
							translation = last.translation;
							rotation = last.rotation;
							scale = last.scale;
						}
						// handle as per normal
						else {
							// Find the frame index 'k' such that: track[k].time <= time < track[k+1].time
							size_t k = 0;
							for (; k < track.size() - 2; ++k) {
								if (time < track[k + 1].time) break;
							}

							const auto& k1 = track[k];
							const auto& k2 = track[k + 1];

							// Interpolate
							float duration = k2.time - k1.time;
							float t = (duration > 1e-5f) ? (time - k1.time) / duration : 0.0f;

							translation = glm::mix(k1.translation, k2.translation, t);
							rotation = glm::slerp(k1.rotation, k2.rotation, t);
							scale = glm::mix(k1.scale, k2.scale, t);
						}
					}
				}

				pose.translations[i] = translation;
				pose.rotations[i] = rotation;
				pose.scales[i] = scale;
			}
			return pose;
		}

		// Blends two different states to make it seemless
		LocalPose System::BlendPoses(const LocalPose& poseA, const LocalPose& poseB, float weight)
		{
			LocalPose result;
			size_t count = poseA.translations.size();
			if (poseB.translations.size() != count) return result; // Error mismatch



			result.Resize(count);

			for (size_t i = 0; i < count; ++i) {
				// Linear Interpolate Position
				result.translations[i] = glm::mix(poseA.translations[i], poseB.translations[i], weight);

				// Spherical Interpolate Rotation (Vital!)
				result.rotations[i] = glm::slerp(poseA.rotations[i], poseB.rotations[i], weight);

				// Linear Interpolate Scale
				result.scales[i] = glm::mix(poseA.scales[i], poseB.scales[i], weight);
			}
			return result;
		}

		std::vector<glm::mat4> System::ConvertPoseToMatrices(const LocalPose& pose, const std::vector<Assets::Bone>& skeleton)
		{

			std::vector<glm::mat4> matrices(skeleton.size());
			std::vector<glm::mat4> globalTransforms(skeleton.size());

			for (size_t i = 0; i < skeleton.size(); ++i) {

				// Calc Local Matrix
				glm::mat4 T = glm::translate(glm::mat4(1.0f), pose.translations[i]);
				glm::mat4 R = glm::mat4_cast(pose.rotations[i]);
				glm::mat4 S = glm::scale(glm::mat4(1.0f), pose.scales[i]);
				glm::mat4 localMat = T * R * S;

				// Calc Global Matrix (Parent * Local)
				int parentIndex = skeleton[i].parent;
				if (parentIndex != -1) {
					globalTransforms[i] = globalTransforms[parentIndex] * localMat;
				}
				else {
					globalTransforms[i] = localMat;
				}

				// Multiply by Inverse Bind Pose (For Skinning)
				matrices[i] = globalTransforms[i] * skeleton[i].bindPose;
			}
			return matrices;
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
