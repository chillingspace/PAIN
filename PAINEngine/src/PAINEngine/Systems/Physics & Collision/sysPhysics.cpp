/*****************************************************************//**
 * \file   sysPhysics.cpp
 * \brief  Definition of physics system states
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "Core/pch.h"
#include "sysPhysics.h"

namespace PAIN {

	void PhysicsSystem::joltSetup()
	{
		// Register allocators + types
		JPH::RegisterDefaultAllocator();
		JPH::Factory::sInstance = new JPH::Factory();
		JPH::RegisterTypes();

		// Allocator + job system
		temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024); // 10 MB scratch
		job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);
	}

	PhysicsSystem::PhysicsSystem() : c_max_bodies{ 1024 }, c_num_body_mutexes{ 0 }, c_max_body_pairs{ 1024 }, c_max_contact_constraints{ 1024 }, 
		temp_allocator{ nullptr }, job_system{ nullptr }, collision_steps{1}
	{
		// Above numbers are placeholders, 1024 will be swapped with ENTITY_MAX or such relavant values
		// Allocate physics system
		jolt_physics = new JPH::PhysicsSystem();

		joltSetup();
	}

	PhysicsSystem::~PhysicsSystem()
	{
		delete jolt_physics;
		delete temp_allocator;
		delete job_system;


		jolt_physics = nullptr;
		temp_allocator = nullptr;
		job_system = nullptr;

		// Cleanup factory
		if (JPH::Factory::sInstance)
		{
			delete JPH::Factory::sInstance;
			JPH::Factory::sInstance = nullptr;
		}
	}

	void PhysicsSystem::init()
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

	void PhysicsSystem::update()
	{
		// To get fixed delta time here
		const float delta_time = 1.0f / 60.0f;

		jolt_physics->Update(delta_time, collision_steps, temp_allocator, job_system);
	}
}