/*****************************************************************//**
 * \file   sysPhysics.cpp
 * \brief  Definition of physics system states
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "../../pch.h"
#include "sysPhysics.h"

namespace PAIN {

	void PhysicsSystem::joltSetup(JPH::PhysicsSystem* jolt)
	{

	}

	PhysicsSystem::PhysicsSystem()
	{

	}

	PhysicsSystem::~PhysicsSystem()
	{
	}

	void PhysicsSystem::init()
	{
		// Init Jolt physics world (broadphase, layers, allocators, etc.)
	}

	void PhysicsSystem::update()
	{
		// To get fixed delta time here
	}
}