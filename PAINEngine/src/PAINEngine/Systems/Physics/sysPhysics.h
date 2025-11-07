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
#include "ECS/System/ISystem.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cAudioSource.h"
#include "CoreSystems/Collision/sCollision.h"
#include "CoreSystems/Collision/sLayer.h"

using namespace glm;

namespace PAIN {

	// To-do for ben: add proper documentation
	namespace Physics {

		class System : public ECS::System::ISystem
		{
		public:
			explicit System(std::shared_ptr<Services> svc);

			~System();

			// To add virtual and override in when abstract systems come in
			void onUpdate(AppTiming timing, entt::registry& reg) override;

			void onFixedUpdate(AppTiming timing, entt::registry& reg) override;
			
			//Event handler for app layer
			void onEvent(Event::Event& e) override;

			void syncNewBodies(entt::registry& registry);

			// Get system name
			std::string getSysName() const override { return "Physics System"; }

			void create_floor();

			void applyBounce(entt::registry&, entt::entity, float jumpImpulse);

			// Getters
			JPH::PhysicsSystem* GetPhysicsSystem() const { return jolt_physics.get(); }
			JPH::BodyInterface& GetBodyInterface() { return jolt_physics->GetBodyInterface(); }

		private:

			std::unique_ptr<JPH::PhysicsSystem> jolt_physics;

			JPH::BodyInterface* body_interface = nullptr;

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

			// Filters and layer interfaces
			PAIN::BPLayerInterfaceImpl	mBroadPhaseLayerInterface;									// The broadphase layer interface that maps object layers to broadphase layers
			PAIN::ObjectVsBroadPhaseLayerFilterImpl mObjectVsBroadPhaseLayerFilter;					// Class that filters object vs broadphase layers
			PAIN::ObjectLayerPairFilterImpl mObjectVsObjectLayerFilter;								// Class that filters object vs object layers

			// Owned memory helpers, for jolt update
			std::unique_ptr<JPH::TempAllocator> temp_allocator;

			std::unique_ptr<JPH::JobSystem> job_system;

			JPH::PhysicsSettings physics_settings;

			const i32 collision_steps;

			// Jolt init setup
			void joltSetup();
		};

	} // Physics

} // PAIN

#endif