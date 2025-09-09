/*****************************************************************//**
 * \file   sysPhysics.h
 * \brief  Declaration of physics system states
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "../../pch.h"

namespace PAIN {
	class PhysicsSystem
	{
	public:
		PhysicsSystem();
		~PhysicsSystem();

		// To add virtual and override in when abstract systems come in
		void update();
		void init();
		std::string getSysName() { return "PhysicsSystem"; }

	private:
		JPH::PhysicsSystem* jolt_physics;

		void joltSetup(JPH::PhysicsSystem* jolt);

	};
}