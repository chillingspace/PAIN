/*****************************************************************//**
 * \file   sysPhysics.cpp
 * \brief  Definition of physics system states
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (5%)
 * \co-author Ong Jun Han, Benjamin, 2301532, o.junhanbenjamin@digipen.edu (95%)
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysPhysics.h"

static constexpr float PI = 3.14159265358979323846f;

namespace PAIN {
	
	// To-do for ben: add proper documentation
	namespace Physics {

		// Jolt Physics setup
		void System::joltSetup()
		{
			static bool jolt_initialized = false;

			if (jolt_initialized) {
				return;
			}

			jolt_initialized = true;

			// Important: Follow the setup order below, otherwise creating bodies will crash if any step is missed

			// Register allocators + types
			JPH::RegisterDefaultAllocator();
			JPH::Factory::sInstance = new JPH::Factory();
			JPH::RegisterTypes();

			// Allocator + job system inits to run jolt update
			// Windows can spare more memory for temp allocator
#ifdef PN_PLATFORM_WINDOWS
			temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(32 * 1024 * 1024);
			unsigned numThreads = std::thread::hardware_concurrency() - 1;

#else
			// Android does not have that much memory to spare, so allocate lesser (1MB), and make sure at least 1 thread
			// To test: allocate 2MB 
			temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(1 * 1024 * 1024);
			unsigned numThreads = std::max<unsigned>(1u, std::thread::hardware_concurrency() - 1);
#endif

			job_system = std::make_unique<JPH::JobSystemThreadPool>(
				JPH::cMaxPhysicsJobs,
				JPH::cMaxPhysicsBarriers,
				numThreads);
	
			if (temp_allocator == nullptr) PN_CORE_WARN("Temp Alloc failed!");
			if (job_system == nullptr) PN_CORE_WARN("Job System failed!");

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

			// Physics settings tweaks, affects how deep objects penetrate each other on rest
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

		System::System(std::shared_ptr<Services> svc) : ISystem(svc), 
														c_max_bodies{ 512 },				// These values work for android implementation, increasing it would cause crashes due to memory constraints
														c_num_body_mutexes{ 64 },			// To test: increase these values and see if it still works on android devices
														c_max_body_pairs{ 2048 }, 
														c_max_contact_constraints{ 1024 }, 
														collision_steps{ 1 }
		{
			PN_CORE_TRACE("Physics::System constructor");

			joltSetup();

			create_floor();

		}

		System::~System()
		{
			// Cleanup
			PN_CORE_TRACE("Physics::System destructor starting");

			// Clear body interface reference
			body_interface = nullptr;

			// Destroy physics system FIRST
			if (jolt_physics) {
				jolt_physics.reset();
				PN_CORE_TRACE("jolt_physics destroyed");
			}

			// Destroy allocators
			job_system.reset();
			temp_allocator.reset();
			PN_CORE_TRACE("allocators destroyed");

			// CRITICAL: Unregister types before deleting factory
			JPH::UnregisterTypes();
			PN_CORE_TRACE("types unregistered");

			// Delete factory
			if (JPH::Factory::sInstance) {
				delete JPH::Factory::sInstance;
				JPH::Factory::sInstance = nullptr;
				PN_CORE_TRACE("factory deleted");
			}
		}

		void System::onUpdate(AppTiming timing, entt::registry& registry)
		{
			// To get fixed delta time here
			const float delta_time = 1.f / 60.f;

			if (temp_allocator && job_system && jolt_physics)
			{
				syncNewBodies(registry);
				jolt_physics->Update(delta_time, collision_steps, temp_allocator.get(), job_system.get());

				auto& body_interface = jolt_physics->GetBodyInterface();

				// Find all entities with Transform and RigidBody3D components
				auto view = registry.view<Transform, Physics::RigidBody3D>();
				for (auto&& [entity, transform, rigidBody] : view.each()) {

					transform = view.get<Transform>(entity);
					rigidBody = view.get<Physics::RigidBody3D>(entity);

					// Lock body for reading
					{
						// Lock the body for reading
						const JPH::BodyLockRead lock(jolt_physics->GetBodyLockInterface(), rigidBody.bodyID);
						if (lock.Succeeded()) {
							const JPH::Body& body = lock.GetBody();
							const JPH::RVec3 position = body.GetPosition();
							const JPH::Quat rotation = body.GetRotation();

							transform.position = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
							transform.rotation = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
						}
						else {
							PN_CORE_ERROR("Failed to lock body for reading");
						}
					}
					// end lock

					if (registry.all_of<Audio::AudioSource, MetaData::EntityName>(entity)) {
						auto& name = registry.get<MetaData::EntityName>(entity);
						if (name.name == "screen") {
							applyBounce(registry, entity, 50.f);
						}
					}
				}
			}
		}

		void System::onEvent(Event::Event& e)
		{
		}

		void System::syncNewBodies(entt::registry& registry) {
			auto view = registry.view<Transform, Physics::RigidBody3D>();
			for (auto&& [entity, transform, rigidBody] : view.each()) {
				transform = view.get<Transform>(entity);
				rigidBody = view.get<Physics::RigidBody3D>(entity);
				
				// Only create if not already created
				if (rigidBody.bodyID.IsInvalid()) {
					// Get rotation
					const glm::quat& q = glm::normalize(transform.rotation);

					JPH::Quat rotationQuat(q.x, 
										   q.y, 
										   q.z, 
										   q.w); // Jolt uses x, y, z, w order

					// Create Jolt body settings
					// Create BoxShape
					JPH::Ref<JPH::BoxShape> boxShape = new JPH::BoxShape(
						JPH::Vec3(.5f * transform.scale.x, 
								  .5f * transform.scale.y,
								  .5f * transform.scale.z),
								  .0f);

					// Create Jolt body settings
					JPH::BodyCreationSettings settings(
						boxShape,
						JPH::RVec3(transform.position.x, 
								   transform.position.y, 
								   transform.position.z),
						rotationQuat,
						JPH::EMotionType::Dynamic,
						PAIN::Layer::MOVING
					);

					settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
					settings.mMassPropertiesOverride.mMass = 10.0f; // any positive number
					settings.mMassPropertiesOverride.mInertia = JPH::Mat44::sScale(1.0f); // placeholder inertia tensor

					JPH::BodyID body_id = body_interface->CreateAndAddBody(settings, JPH::EActivation::Activate);
					rigidBody.bodyID = body_id;

					PN_CORE_TRACE("Created Jolt body for entity {} with ID {}",
								   (uint32_t)entity,
								   rigidBody.bodyID.GetIndexAndSequenceNumber());
				}
			}
		}

		void System::applyBounce(entt::registry& registry, entt::entity targetEntity, float jumpImpulse)
		{
			auto view = registry.view<Transform, Physics::RigidBody3D, Audio::AudioSource>();
			if (!view.contains(targetEntity))
				return;

			auto& transform = view.get<Transform>(targetEntity);
			auto& rigidBody = view.get<RigidBody3D>(targetEntity);

			//// Lock body for reading
			//const JPH::BodyLockRead lock(jolt_physics->GetBodyLockInterface(), rigidBody.bodyID);
			//if (!lock.Succeeded()) {
			//	PN_CORE_WARN("Failed to lock body for reading");
			//	return;
			//}

			//const JPH::Body& body = lock.GetBody();

			// Ground check
			glm::f32 half_height = 0.5f * transform.scale.y;
			JPH::RRayCast ray{};
			ray.mOrigin = JPH::RVec3(transform.position.x, transform.position.y - (half_height + 0.05f), transform.position.z);
			ray.mDirection = JPH::Vec3(0, -1, 0) * 0.5f;

			JPH::RayCastResult result;
			bool onGround = jolt_physics->GetNarrowPhaseQuery().CastRay(ray, result);

			if (onGround) {
				body_interface->ActivateBody(rigidBody.bodyID);
				body_interface->AddImpulse(rigidBody.bodyID, JPH::Vec3(0, jumpImpulse, 0));
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
				PAIN::Layer::NON_MOVING
			);

			// Create and add
			JPH::Body* floorBody = body_interface->CreateBody(floorSettings);
			body_interface->AddBody(floorBody->GetID(), JPH::EActivation::DontActivate);
		}
	} // Physics
} // PAIN