/*****************************************************************//**
 * \file   sysPhysics.cpp
 * \brief  Definition of physics system states
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysPhysics.h"

namespace PAIN {

	namespace Physics {

		void System::joltSetup()
		{
			// Register allocators + types
			JPH::RegisterDefaultAllocator();
			JPH::Factory::sInstance = new JPH::Factory();
			JPH::RegisterTypes();

			// Allocator + job system

			// 10 MB allocation 
			temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024); 
			job_system = std::make_unique<JPH::JobSystemThreadPool>(
				JPH::cMaxPhysicsJobs,
				JPH::cMaxPhysicsBarriers,
				std::thread::hardware_concurrency() - 1);
		}

		System::System() : c_max_bodies{ 1024 }, c_num_body_mutexes{ 0 }, c_max_body_pairs{ 1024 }, c_max_contact_constraints{ 1024 },
			temp_allocator{ nullptr }, job_system{ nullptr }, collision_steps{ 1 }
		{
			// Above numbers are placeholders, 1024 will be swapped with ENTITY_MAX or such relavant values
			// Allocate physics system
			jolt_physics = std::make_unique<JPH::PhysicsSystem>();

			joltSetup();
		}

		System::~System()
		{
			// Cleanup
		}

		void System::init()
		{
			// TODO: Init layers
			// Init Jolt physics world (broadphase, layers, allocators, etc.)
			//jolt_physics->Init(
			//	c_max_bodies,                  // max bodies
			//	c_num_body_mutexes,                     // body mutexes (0 = auto)
			//	c_max_body_pairs,                  // max body pairs
			//	c_max_contact_constraints,                  // max contacts
			//	*mBroadPhaseLayerInterface,
			//	mObjectVsBroadPhaseLayerFilter,
			//	mObjectLayerPairFilter
			//);
		}

		void System::update()
		{
			// To get fixed delta time here
			const float delta_time = 1.0f / 60.0f;

			// If both unique ptrs of job system and temp allocator are valid, then jolt update
			if (temp_allocator && job_system)
			{
				jolt_physics->Update(delta_time, collision_steps, temp_allocator.get(), job_system.get());
			}

		}

	}
}