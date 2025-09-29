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

#include "pch.h"
#include "Core.h"
#include "CoreSystems/Collision/sCollision.h"
#include "CoreSystems/Collision/sLayer.h"

using namespace glm;

namespace PAIN {

	namespace Physics {

		class System : public ECS::ISystem
		{
		public:
			System();
			~System();

			// To add virtual and override in when abstract systems come in
			virtual void onUpdate() override;
			virtual void onAttach() override;

			virtual void onDetach() override;
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

		};
	}

}

#endif