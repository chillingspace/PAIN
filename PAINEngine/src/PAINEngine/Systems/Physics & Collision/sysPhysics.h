/*****************************************************************//**
 * \file   sysPhysics.h
 * \brief  Declaration of physics system states
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifndef SYS_PHYSICS_H
#define SYS_PHYSICS_H

#include "PAINEngine/Core/pch.h"
#include "sCollision.h"

using namespace glm;

namespace PAIN {

	namespace Physics {

		// Broad phase layer 1-1 mapping with number of obj layers
		// 4 due to trigger, static, dynamic, character (player) layers

		// Object layers are used to define who collides with who. (Static collide with dynamic)
		// (Dynamic collide with Static, but static will not collide with static)
		// If is trigger, no physics resolve, just report overlap
		namespace ObjectLayers
		{
			static constexpr JPH::ObjectLayer ol_static = 0;
			static constexpr JPH::ObjectLayer ol_dynamic = 1;
			static constexpr JPH::ObjectLayer ol_trigger = 2;
			static constexpr JPH::ObjectLayer ol_character = 3;

			static constexpr JPH::ObjectLayer num_obj_layers = 4;
		}

		// From gpt explanation: 
		/*BroadPhase Layers are categories you assign to objects so the broad phase can skip unnecessary checks.
		Example:

		Static terrain → never collides with other static terrain.

		Projectiles → might only collide with characters, not with each other.*/
		namespace BroadPhaseLayers
		{
			/* Gpt explanation : Characters (usually kinematic controllers like capsules) are a special case — they typically:
			Collide with static & dynamic so they walk on floors and bump into things.
			Don’t collide with other characters (to avoid jitter/pushing).
			Usually don’t collide with triggers physically, but still report overlaps (so you can detect pickups/volumes).*/

			// Trigger layers are like for pickup objects, detect collision but no resolution

			static constexpr JPH::BroadPhaseLayer bp_static{ 0 };
			static constexpr JPH::BroadPhaseLayer bp_dynamic{ 1 };
			static constexpr JPH::BroadPhaseLayer bp_trigger{ 2 };
			static constexpr JPH::BroadPhaseLayer bp_character{ 3 };

			static constexpr JPH::uint num_bp_layers = 4;
		}

		class System
		{
		public:
			System();
			~System();

			// To add virtual and override in when abstract systems come in
			void update();
			void init();
			std::string getSysName() { return "Physics System"; }

		private:

			std::unique_ptr<JPH::PhysicsSystem> jolt_physics;

			// Owned memory helpers, for jolt update
			std::unique_ptr<JPH::TempAllocator> temp_allocator;
			std::unique_ptr<JPH::JobSystem> job_system;

			// Jolt init values

			// Max number of rigid bodies that can exist at once in your physics world.
			// Each body = one collider + optional rigidbody data.
			const i32 c_max_bodies;

			// Jolt uses body mutexes for thread safety (when multiple threads read/write bodies).
			// 0 = let Jolt auto - pick based on hardware(good default).
			const i32 c_num_body_mutexes;

			// Max number of potential collision pairs that can be tracked in a single simulation step.
			// Each pair = two bodies that the broad phase says might collide.
			// If you set this too low and you have too many objects close together, some collisions might be ignored.
			const i32 c_max_body_pairs;

			// Max number of actual contact points/constraints Jolt can resolve in a single step.
			// A single collision between two complex shapes might generate multiple contact points.
			// This caps how many constraints the solver can handle per step.
			const i32 c_max_contact_constraints;

			const i32 collision_steps;

			// Jolt init setup
			void joltSetup();

			std::unique_ptr<Collision::Service> collision_service;

		};
	}

}

#endif