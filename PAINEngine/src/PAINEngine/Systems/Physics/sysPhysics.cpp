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

static constexpr float PI = 3.14159265358979323846f;

namespace PAIN {

	namespace Physics {

		void System::joltSetup()
		{
			// Important: Follow the setup order below, otherwise creating bodies will crash if any step is missed

			// Register allocators + types
			JPH::RegisterDefaultAllocator();
			JPH::Factory::sInstance = new JPH::Factory();
			JPH::RegisterTypes();

			// Allocator + job system inits to run jolt update
			// 10 MB allocation 
			temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(32 * 1024 * 1024);

			job_system = std::make_unique<JPH::JobSystemThreadPool>(
				JPH::cMaxPhysicsJobs,
				JPH::cMaxPhysicsBarriers,
				std::thread::hardware_concurrency() - 1);

			// Create new physics system
			jolt_physics = std::make_unique<JPH::PhysicsSystem>();

			// Init physics system
			jolt_physics->Init(
				c_max_bodies,
				c_num_body_mutexes,
				c_max_body_pairs,
				c_max_contact_constraints,
				mBroadPhaseLayerInterface,
				mObjectVsBroadPhaseLayerFilter,
				mObjectVsObjectLayerFilter
			);

			physics_settings.mSpeculativeContactDistance = 0.005f; // smaller than default
			physics_settings.mPenetrationSlop = 0.001f;            // almost no penetration
			physics_settings.mMaxPenetrationDistance = 0.05f;      // limit max correction per step

			// Set physics settings
			jolt_physics->SetPhysicsSettings(physics_settings);

			// Set gravity
			jolt_physics->SetGravity(JPH::Vec3(0, -9.81f, 0));

			// Get body interface
			body_interface = &jolt_physics->GetBodyInterface();
		}

		System::System(std::shared_ptr<Services> svc) : ISystem(svc), c_max_bodies{ 10240 }, c_num_body_mutexes{ 0 }, c_max_body_pairs{ 65536 }, c_max_contact_constraints{ 20480 }, collision_steps{ 1 }
		{
			joltSetup();

			create_floor();
		}

		System::~System()
		{
			// Cleanup
			// 1. Destroy PhysicsSystem first (before destroying allocators it depends on)
			jolt_physics.reset();

			// 2. Destroy job system and temp allocator
			job_system.reset();
			temp_allocator.reset();

			// 3. Unregister all types and clean up default material
			JPH::UnregisterTypes();

			// 4. Delete the factory instance
			delete JPH::Factory::sInstance;
			JPH::Factory::sInstance = nullptr;
		}

		void System::onUpdate(AppTiming timing, entt::registry& registry)
		{
			// To get fixed delta time here
			const float delta_time = 1.0f / 60.0f;

			if (temp_allocator && job_system && jolt_physics)
			{
				syncNewBodies(registry);
				jolt_physics->Update(delta_time, collision_steps, temp_allocator.get(), job_system.get());

				auto view = registry.view<Transform, Physics::RigidBody3D>();
				for (auto&& [entity, transform, rigidBody] : view.each()) {
					//PN_CORE_TRACE("Arrived here");

					//PN_CORE_TRACE("Entity name {}", name.name);

					transform = view.get<Transform>(entity);
					rigidBody = view.get<Physics::RigidBody3D>(entity);

					// Lock the body for reading (thread-safe)
					const JPH::BodyLockRead lock(jolt_physics->GetBodyLockInterface(), rigidBody.bodyID);
					if (lock.Succeeded()) {
						const JPH::Body& body = lock.GetBody();
						const JPH::RVec3 position = body.GetPosition();
						const JPH::Quat rotation = body.GetRotation();

						transform.position = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
						transform.rotation = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

						//std::cout << "pos = " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << "\n";
					}
					else {
						PN_CORE_WARN("Failed to lock body for reading");
					}
				}
				//PN_CORE_TRACE("updating physics");
			}
		}

		void System::onEvent(Event::Event& e)
		{
		}

		void System::syncNewBodies(entt::registry& registry) {
			auto& bodyInterface = jolt_physics->GetBodyInterface();

			auto view = registry.view<Transform, Physics::RigidBody3D>();
			for (auto&& [entity, transform, rigidBody] : view.each()) {
				transform = view.get<Transform>(entity);
				rigidBody = view.get<Physics::RigidBody3D>(entity);

				// Only create if not already created
				if (rigidBody.bodyID.IsInvalid()) {
					// Create Jolt body settings
					// Create BoxShape
					JPH::BoxShape* boxShape = new JPH::BoxShape(
						JPH::Vec3(0.5f * transform.scale.x, 0.5f * transform.scale.y, 0.5f * transform.scale.z),
						0.0f // convex radius
					);

					// Create Jolt body settings
					JPH::BodyCreationSettings settings(
						boxShape,
						JPH::RVec3(transform.position.x, transform.position.y, transform.position.z),
						JPH::Quat::sIdentity(),
						JPH::EMotionType::Dynamic,
						NIKE::Layer::MOVING
					);

					settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
					settings.mMassPropertiesOverride.mMass = 10.0f; // any positive number
					settings.mMassPropertiesOverride.mInertia = JPH::Mat44::sScale(1.0f); // simple placeholder inertia tensor

					JPH::BodyID body_id = body_interface->CreateAndAddBody(settings, JPH::EActivation::Activate);
					rigidBody.bodyID = body_id;

					PN_CORE_INFO("Created Jolt body for entity {} with ID {}",
						(uint32_t)entity,
						rigidBody.bodyID.GetIndexAndSequenceNumber());
				}
			}
		}

		void System::create_floor()
		{
			if (!body_interface)
				return;

			// Half extents
			JPH::Vec3 halfExtent(100.0f, 1.0f, 100.0f);

			// Create shape
			JPH::Ref<JPH::BoxShape> floorShape = new JPH::BoxShape(halfExtent, 0.0f);

			// Body settingsx
			JPH::BodyCreationSettings floorSettings(
				floorShape,
				JPH::RVec3(0.0f, -1.f, 0.0f), // move down by halfHeight
				JPH::Quat::sIdentity(),
				JPH::EMotionType::Static,
				NIKE::Layer::NON_MOVING
			);

			// Create and add
			JPH::Body* floorBody = body_interface->CreateBody(floorSettings);
			body_interface->AddBody(floorBody->GetID(), JPH::EActivation::DontActivate);
		}
	} // Physics
} // PAIN