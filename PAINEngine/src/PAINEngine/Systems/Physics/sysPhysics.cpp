/*****************************************************************/ /**
 * \file   sysPhysics.cpp
 * \brief  Definition of physics system states
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (5%)
 * \co-author Ong Jun Han, Benjamin, 2301532, o.junhanbenjamin@digipen.edu (95%)
 * \date   September 2025 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "sysPhysics.h"
#include "ECS/Controller.h"
#include "Systems/Scripting/GameScriptingSystem.h"
#include "pch.h"

// Additional Jolt includes for compound colliders
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>

// Compound collider component
#include "ECS/Components/cCompoundCollider.h"

static constexpr float PI = 3.14159265358979323846f;

namespace PAIN {

	// To-do for ben: add proper documentation
	namespace Physics {
		// Global pointer to physics system (set during initialization)
		static System* g_physics_system = nullptr;

		PhysicsLayer& PhysicsLayer::operator=(JPH::ObjectLayer v) {
			if (value != v) {
				value = v;

				// Auto-update if body mesh_id
				if (bodyID_ptr && !bodyID_ptr->IsInvalid() && g_physics_system) {
					g_physics_system->updateBodyLayer(*bodyID_ptr, value);
				}
			}
			return *this;
		}

		// jolt lua bridge
		struct System::LuaContactListener : public JPH::ContactListener {
			System* owner{};
			explicit LuaContactListener(System* s) : owner{s} {}

			void OnContactAdded(const JPH::Body& b1, const JPH::Body& b2,
								const JPH::ContactManifold&,
								JPH::ContactSettings&) override {
				if (owner)
					owner->notifyContact(b1, b2);
			}

			// forward persisted contacts for continuous events
			void OnContactPersisted(const JPH::Body& b1, const JPH::Body& b2,
									const JPH::ContactManifold&,
									JPH::ContactSettings&) override {
				if (owner)
					owner->notifyContact(b1, b2);
			}
		};

		// Jolt Physics setup
		void System::joltSetup() {
			static bool jolt_initialized = false;

			if (jolt_initialized) {
				return;
			}

			jolt_initialized = true;

			// Important: Follow the setup order below, otherwise creating bodies will
			// crash if any step is missed

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
			// Android does not have that much memory to spare, so allocate lesser (1MB),
			// and make sure at least 1 thread To test: allocate 2MB
			temp_allocator = std::make_unique<JPH::TempAllocatorImpl>(1 * 1024 * 1024);
			unsigned numThreads =
				std::max<unsigned>(1u, std::thread::hardware_concurrency() - 1);
#endif

			job_system = std::make_unique<JPH::JobSystemThreadPool>(
				JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, numThreads);

			if (temp_allocator == nullptr)
				PN_CORE_WARN("Temp Alloc failed!");
			if (job_system == nullptr)
				PN_CORE_WARN("Job System failed!");

			// Create new physics system
			jolt_physics = std::make_unique<JPH::PhysicsSystem>();

			// Init physics system
			jolt_physics->Init(c_max_bodies, c_num_body_mutexes, c_max_body_pairs,
							   c_max_contact_constraints, mBroadPhaseLayerInterface,
							   mObjectVsBroadPhaseLayerFilter,
							   mObjectVsObjectLayerFilter);

			// Physics settings tweaks, affects how deep objects penetrate each other on
			// rest
			physics_settings.mSpeculativeContactDistance = 0.005f; // smaller than default
			physics_settings.mPenetrationSlop = 0.001f;			   // almost no penetration
			physics_settings.mMaxPenetrationDistance =
				0.05f; // limit max correction per step

			// Set physics settings
			jolt_physics->SetPhysicsSettings(physics_settings);

			// Set gravity
			jolt_physics->SetGravity(JPH::Vec3(0, -9.81f, 0));

			// Get body interface
			body_interface = &jolt_physics->GetBodyInterface();

			// install contact listener so can notify lua on collisions
			contact_listener = std::make_unique<LuaContactListener>(this);
			jolt_physics->SetContactListener(contact_listener.get());
		}

		void System::notifyContact(const JPH::Body& b1,
								   const JPH::Body& b2) // convert a physics body pair
														// to entt::entity and call Lua
		{
			entt::entity a =
				static_cast<entt::entity>(static_cast<uint32_t>(b1.GetUserData()));
			entt::entity b =
				static_cast<entt::entity>(static_cast<uint32_t>(b2.GetUserData()));
			// PN_CORE_WARN("[PHYSICS] Contact b1={}, b2={}", (uint32_t)a, (uint32_t)b);
			if (a == entt::null || b == entt::null)
				return;

			// Check scene layer collision rules
			if (!shouldLayersCollide(a, b))
				return;

			{
				std::lock_guard<std::mutex> lock(collisionMutex_);
				pendingCollisions_.push_back({a, b});
			}
		}

		bool System::shouldLayersCollide(entt::entity a, entt::entity b) {
			auto svc = services.lock();
			if (!svc)
				return true;

			auto ecs = svc->get<ECS::Controller>();
			if (!ecs)
				return true;

			auto& registry = ecs->getRegistry();

			// Get scene layers
			if (!registry.all_of<Entity::Layer>(a) || !registry.all_of<Entity::Layer>(b))
				return true; // No layer component = allow collision

			auto& layerA = registry.get<Entity::Layer>(a);
			auto& layerB = registry.get<Entity::Layer>(b);

			// Get scene manager for mask matrix
			auto sceneMgr = svc->get<Scene::SceneManager>();
			if (!sceneMgr)
				return true;

			// Check mask matrix
			const auto& mask_matrix = sceneMgr->getMaskMatrix();
			if (layerA.layer_id < mask_matrix.size() &&
				layerB.layer_id < mask_matrix[layerA.layer_id].size()) {
				return mask_matrix[layerA.layer_id][layerB.layer_id];
			}

			return true;
		}

		void System::dispatchCollisionEvents(entt::registry& reg) {
			// move pending collisions to a local vector so hold the mutex briefly
			std::vector<PendingCollision> localCollisions;
			{
				std::lock_guard<std::mutex> lock(collisionMutex_);
				localCollisions.swap(pendingCollisions_);
			}

			if (localCollisions.empty())
				return;

			// now can use lua on main thread
			if (auto svc = services.lock()) {
				if (auto ecs = svc->get<ECS::Controller>()) {
					if (auto gs = ecs->getSystem<PAIN::Scripting::GameScriptingSystem>()) {
						for (const auto& c : localCollisions) {
							gs->onCollision(c.a, c.b);
						}
					} else {
						PN_CORE_WARN(
							"[PHYSICS] No GameScriptingSystem in ECS; skipping Lua dispatch");
					}
				} else {
					PN_CORE_WARN(
						"[PHYSICS] No ECS::Controller in services; skipping Lua dispatch");
				}
			} else {
				PN_CORE_WARN("[PHYSICS] Services expired in dispatchCollisionEvents; "
							 "skipping Lua dispatch");
			}
		}

		System::System(std::shared_ptr<Services> svc)
			: ISystem(svc),
			  c_max_bodies{
				  512},				  // These values work for android implementation, increasing it
									  // would cause crashes due to memory constraints
			  c_num_body_mutexes{64}, // To test: increase these values and see if it
									  // still works on android devices
			  c_max_body_pairs{2048}, c_max_contact_constraints{1024},
			  collision_steps{1} {
			PN_CORE_TRACE("Physics::System constructor");

			g_physics_system = this;

			joltSetup();

			create_floor();

			auto ecs = svc->get<ECS::Controller>();
			if (ecs) {
				// Connect the OnDestroy signal for RigidBody3D components
				ecs->getRegistry()
					.on_destroy<Physics::RigidBody3D>()
					.connect<&System::onEntityDestroyed>(this);
			}
		}

		System::~System() {
			// Cleanup
			PN_CORE_TRACE("Physics::System destructor starting");

			// Unregister
			g_physics_system = nullptr;

			// Clear body interface reference
			body_interface = nullptr;

			// Disconnect signal
			if (auto svc = services.lock()) {
				if (auto ecs = svc->get<ECS::Controller>()) {
					ecs->getRegistry()
						.on_destroy<Physics::RigidBody3D>()
						.disconnect<&System::onEntityDestroyed>(this);
				}
			}

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

		void System::onFixedUpdate(AppTiming timing, entt::registry& registry) {
			// Use the fixed delta time from the application loop
			const float delta_time = timing.fixed_dt;

			if (temp_allocator && job_system && jolt_physics) {
				// Only update if time has actually passed
				if (delta_time > 0.0f) {
					auto view = registry.view<Physics::RigidBody3D, LocalTransform>();
					for (auto [entity, rb, transform] : view.each()) {
						if (rb.physics_enabled && !rb.bodyID.IsInvalid()) {
							JPH::Vec3 pos = body_interface->GetPosition(rb.bodyID);
							rb.prevPosition = glm::vec3(pos.GetX(), pos.GetY(), pos.GetZ());
						}
							

					}

					syncNewBodies(registry);
					jolt_physics->Update(delta_time, collision_steps,
						temp_allocator.get(), job_system.get());
					dispatchCollisionEvents(registry);
				}
			}
		}

		void System::onUpdate(AppTiming timing, entt::registry& registry) {
			accumulator_alpha_ = timing.alpha;

			// IF GAME IS STOPPED
			if (timing.dt <= 0.0001f) {
				return;
			}

			{ // make Physics::System discoverable via registry context
				auto& ctx = registry.ctx();
				if (!ctx.contains<PAIN::Physics::System*>()) {
					ctx.emplace<PAIN::Physics::System*>(this);
				}
			}

			// onUpdate is now ONLY responsible for syncing the physics state back to the
			// transforms for rendering.
			if (temp_allocator && job_system && jolt_physics) {
				auto& body_interface = jolt_physics->GetBodyInterface();

				// Find all entities with Transform and RigidBody3D components
				// Optimized using group
				auto group = registry.group<Physics::RigidBody3D>(
					entt::get<LocalTransform, WorldTransform>);

				for (auto&& [entity, rigidBody, transform, world] : group.each()) {

					if (!rigidBody.physics_enabled) {
						continue;
					}

					if (!rigidBody.bodyID.IsInvalid()) {
						JPH::ObjectLayer current_layer =
							body_interface.GetObjectLayer(rigidBody.bodyID);
						JPH::ObjectLayer desired_layer =
							rigidBody.physics_behavior.toJoltLayer();

						if (current_layer != desired_layer) {
							// Layer changed in component, update physics body
							updateBodyLayer(rigidBody.bodyID, desired_layer);
						}
					}

					// Check and apply motion type update
					JPH::EMotionType desired_motion_type;
					switch (rigidBody.motion_type) {
					case MotionType::Static:
						desired_motion_type = JPH::EMotionType::Static;
						break;
					case MotionType::Kinematic:
						desired_motion_type = JPH::EMotionType::Kinematic;
						break;
					case MotionType::Dynamic:
					default:
						desired_motion_type = JPH::EMotionType::Dynamic;
						break;
					}

					// Lock body for reading
					{
						// Lock the body for reading
						const JPH::BodyLockRead lock(jolt_physics->GetBodyLockInterface(),
													 rigidBody.bodyID);
						if (lock.Succeeded()) {
							const JPH::Body& body = lock.GetBody();
							const JPH::Quat rotation = body.GetRotation();
							const JPH::RVec3 position = body.GetPosition();

							glm::vec3 joltWorldPos(position.GetX(), position.GetY(), position.GetZ());
							glm::quat joltWorldRot(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

							// Will update after releasing lock
							if (body.GetMotionType() != desired_motion_type) {
							}
							
							bool hasParent = false;
							glm::mat4 parentWorld(1.0f);
							if (auto* hierarchy = registry.try_get<Entity::Hierarchy>(entity)) {
								if (hierarchy->parentGUID.IsValid()) {
									if (auto svc = services.lock()) {
										auto ecs = svc->get<ECS::Controller>();
										ECS::RegistryID regID = ECS::MAIN_REGISTRY_ID;
										for (const auto& id : ecs->getAllRegistryIDs()) {
											if (&ecs->getRegistry(id) == &registry) {
												regID = id; break;
											}
										}
										entt::entity parent = ecs->getGUIDRegistry(regID).resolveGUID(hierarchy->parentGUID);
										if (parent != entt::null && registry.valid(parent) && registry.all_of<WorldTransform>(parent)) {
											hasParent = true;
											parentWorld = registry.get<WorldTransform>(parent).matrix;
										}
									}
								}
							}
							
							if (hasParent) {
								glm::mat4 joltWorldMat = glm::translate(glm::mat4(1.0f), joltWorldPos) * glm::mat4(joltWorldRot);
								glm::mat4 localMat = glm::inverse(parentWorld) * joltWorldMat;
								
								glm::vec3 newScale, newSkew;
								glm::vec4 newPerspective;
								glm::quat newRot;
								glm::vec3 newPos;
								glm::decompose(localMat, newScale, newRot, newPos, newSkew, newPerspective);
								
								transform.position = newPos;
								transform.rotation = glm::normalize(newRot);
							} else {
								transform.position = glm::mix(rigidBody.prevPosition, joltWorldPos, accumulator_alpha_);
								transform.rotation = glm::normalize(joltWorldRot);
							}

							// // Inside your body lock read block, replace the direct assignment:
							// JPH::RVec3 position = body.GetPosition();
							// glm::vec3 currentPos(position.GetX(), position.GetY(), position.GetZ());

							// // Interpolate between previous and current physics position (which might affect accuracy but improves visual smoothness)
							// transform.position = glm::mix(rigidBody.prevPosition, currentPos, accumulator_alpha_);

							// transform.rotation = glm::quat(rotation.GetW(), rotation.GetX(),
							// 							   rotation.GetY(), rotation.GetZ());
							world.dirty = true;

							const JPH::Vec3 velocity = body.GetLinearVelocity();
							const JPH::Vec3 angVel = body.GetAngularVelocity();
							rigidBody.velocity =
								glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
							rigidBody.angular_velocity =
								glm::vec3(angVel.GetX(), angVel.GetY(), angVel.GetZ());
						} else {
							PN_CORE_ERROR("Failed to lock body for reading");
						}
					}
					// end lock

					JPH::EMotionType current_motion_type =
						body_interface.GetMotionType(rigidBody.bodyID);
					if (current_motion_type != desired_motion_type) {
						body_interface.SetMotionType(rigidBody.bodyID, desired_motion_type,
													 JPH::EActivation::Activate);
					}

					if (registry.all_of<Audio::AudioSource, Entity::Name>(entity)) {
						auto& name = registry.get<Entity::Name>(entity);
						if (name.name == "screen") {
							applyBounce(registry, entity, 50.f);
						}
					}
				}
			}
		}

		void System::onEvent(Event::Event& e) {}

		// Added to clear ridgid bodies on destruction of the entity
		void System::onEntityDestroyed(entt::registry& registry, entt::entity entity) {
			// Check if it has a RigidBody
			if (registry.all_of<Physics::RigidBody3D>(entity)) {
				auto& rb = registry.get<Physics::RigidBody3D>(entity);

				if (!rb.bodyID.IsInvalid() && body_interface) {
					body_interface->RemoveBody(rb.bodyID);
					body_interface->DestroyBody(rb.bodyID);

					// Invalidate the ID
					rb.bodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);

					//PN_CORE_TRACE("Destroyed Jolt body for entity {}", (uint32_t)entity);
				}
			}
		}

		// Print all ridgid bodies to console
		void System::logAllBodies() {
			if (!jolt_physics) {
				PN_CORE_ERROR("Cannot log bodies: Jolt system is null");
				return;
			}

			// 1. Get total count
			uint32_t count = jolt_physics->GetNumBodies();

			PN_CORE_WARN("=== JOLT PHYSICS DIAGNOSTICS ===");
			PN_CORE_WARN("Total Bodies in Simulation: {}", count);

			// 2. Retrieve all Body IDs (Active and Inactive)
			JPH::BodyIDVector bodyIDs;
			jolt_physics->GetBodies(bodyIDs);

			// 3. Iterate and print details
			auto& lockInterface =
				jolt_physics->GetBodyLockInterface(); // Need lock to read data safely

			for (const auto& id : bodyIDs) {
				// Lock body for reading
				JPH::BodyLockRead lock(lockInterface, id);
				if (lock.Succeeded()) {
					const JPH::Body& body = lock.GetBody();

					// Get the Entity ID we stored in UserData
					uint64_t userData = body.GetUserData();
					uint32_t entityID = static_cast<uint32_t>(userData);

					// Get Position for context
					const JPH::RVec3& pos = body.GetPosition();

					// Check if it's the floor (usually static, no entity ID or specific ID)
					std::string typeStr = (body.GetMotionType() == JPH::EMotionType::Static)
											  ? "Static"
											  : "Dynamic";

					PN_CORE_TRACE("BodyID [{}] | EntityID [{}] | Type: {} | Pos: ({:.2f}, "
								  "{:.2f}, {:.2f})",
								  id.GetIndexAndSequenceNumber(), entityID, typeStr,
								  (float)pos.GetX(), (float)pos.GetY(), (float)pos.GetZ());
				}
			}
			PN_CORE_WARN("================================");
		}

		void System::syncNewBodies(entt::registry& registry) {
			// Use view to avoid group ownership conflict with onUpdate
			auto view = registry.view<Physics::RigidBody3D, LocalTransform, WorldTransform>();
			for (auto&& [entity, rigidBody, transform, world] : view.each()) {

				// skip if physics disabled
				if (!rigidBody.physics_enabled) {
					continue;
				}

				// Check if scale OR offset changed in editor
				// If so, destroy the existing body so it can be recreated below with new
				// settings
				if (!rigidBody.bodyID.IsInvalid() &&
					(rigidBody.collider_scale != rigidBody.last_applied_scale ||
					 rigidBody.collider_offset != rigidBody.last_applied_offset)) {

					body_interface->RemoveBody(rigidBody.bodyID);
					body_interface->DestroyBody(rigidBody.bodyID);
					rigidBody.bodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
				}

				// Only create if not already created (or just destroyed above)
				if (rigidBody.bodyID.IsInvalid()) {
					// Get world transform
					glm::vec3 worldPos, scale, skew;
					glm::vec4 perspective;
					glm::quat worldRot;
					glm::decompose(world.matrix, scale, worldRot, worldPos, skew, perspective);
					worldRot = glm::normalize(worldRot);

					JPH::Quat rotationQuat(worldRot.x, worldRot.y, worldRot.z,
										   worldRot.w); // Jolt uses x, y, z, w order

					// Create Jolt body settings

					// Create the shape (compound or simple based on CompoundCollider
					// component)
					JPH::Ref<JPH::Shape> finalShape;

					// Check if entity has CompoundCollider component and it's enabled
					CompoundCollider* compCollider =
						registry.try_get<CompoundCollider>(entity);
					if (compCollider && compCollider->useCompoundCollider &&
						!compCollider->shapes.empty()) {
						// Create compound shape from the collider definitions
						JPH::StaticCompoundShapeSettings compoundSettings;

						for (const auto& shape : compCollider->shapes) {
							JPH::Ref<JPH::Shape> subShape;

							// Create primitive shape based on type
							switch (shape.type) {
							case ColliderShapeType::Box:
								subShape = new JPH::BoxShape(
									JPH::Vec3(shape.boxHalfExtents.x * scale.x * rigidBody.collider_scale.x,
											  shape.boxHalfExtents.y * scale.y * rigidBody.collider_scale.y,
											  shape.boxHalfExtents.z * scale.z * rigidBody.collider_scale.z),
									0.0f // convex radius
								);
								break;

							case ColliderShapeType::Sphere:
								subShape = new JPH::SphereShape(
									shape.sphereRadius * glm::max(glm::max(scale.x, scale.y), scale.z) *
									glm::max(glm::max(rigidBody.collider_scale.x,
													  rigidBody.collider_scale.y),
											 rigidBody.collider_scale.z));
								break;

							case ColliderShapeType::Capsule:
								subShape = new JPH::CapsuleShape(
									shape.capsuleHalfHeight * scale.y * rigidBody.collider_scale.y,
									shape.capsuleRadius * glm::max(scale.x, scale.z) *
										glm::max(rigidBody.collider_scale.x,
												 rigidBody.collider_scale.z));
								break;
							}

							// Calculate offset with entity scale and RigidBody collider_offset
							// applied
							JPH::Vec3 subOffset((shape.offset.x + rigidBody.collider_offset.x) *
													scale.x,
												(shape.offset.y + rigidBody.collider_offset.y) *
													scale.y,
												(shape.offset.z + rigidBody.collider_offset.z) *
													scale.z);

							// Convert rotation
							JPH::Quat subRotation(shape.rotation.x, shape.rotation.y,
												  shape.rotation.z, shape.rotation.w);

							// Add sub-shape to compound
							compoundSettings.AddShape(subOffset, subRotation, subShape);
						}

						// Create the compound shape
						JPH::ShapeSettings::ShapeResult result = compoundSettings.Create();
						if (result.IsValid()) {
							finalShape = result.Get();
						} else {
							PN_CORE_ERROR("Failed to create compound shape for entity {}",
										  (uint32_t)entity);
							// Fallback to simple box
							finalShape = new JPH::BoxShape(
								JPH::Vec3(.5f * scale.x * rigidBody.collider_scale.x,
										  .5f * scale.y * rigidBody.collider_scale.y,
										  .5f * scale.z * rigidBody.collider_scale.z),
								0.0f);
						}
					} else {
						// ensure positive dimensions in order to prevent assert errors
						float extentX = std::abs(.5f * scale.x * rigidBody.collider_scale.x);
						float extentY = std::abs(.5f * scale.y * rigidBody.collider_scale.y);
						float extentZ = std::abs(.5f * scale.z * rigidBody.collider_scale.z);

						// ensure dimensions are not exactly zero to be safe
						extentX = std::max(extentX, 0.001f);
						extentY = std::max(extentY, 0.001f);
						extentZ = std::max(extentZ, 0.001f);

						// Default: Create simple BoxShape (original behavior)
						// Note: Jolt BoxShape takes half-extents
						finalShape = new JPH::BoxShape(
							JPH::Vec3(extentX, extentY, extentZ),
							0.0f
						);

						// Apply Offset using RotatedTranslatedShape if needed
						// We wrap the box shape in a transform shape to offset it from the
						// entity center
						if (rigidBody.collider_offset != glm::vec3(0.0f)) {
							JPH::Vec3 offset(rigidBody.collider_offset.x * scale.x,
											 rigidBody.collider_offset.y * scale.y,
											 rigidBody.collider_offset.z * scale.z);
							finalShape = new JPH::RotatedTranslatedShape(
								offset, JPH::Quat::sIdentity(), finalShape);
						}
					}

					JPH::EMotionType motion_type;
					switch (rigidBody.motion_type) {
					case MotionType::Static:
						motion_type = JPH::EMotionType::Static;
						break;
					case MotionType::Kinematic:
						motion_type = JPH::EMotionType::Kinematic;
						break;
					default:
						motion_type = JPH::EMotionType::Dynamic;
						break;
					}

					JPH::ObjectLayer jolt_layer = rigidBody.physics_behavior.toJoltLayer();

					// Create Jolt body settings
					JPH::BodyCreationSettings settings(finalShape,
													   JPH::RVec3(worldPos.x,
																  worldPos.y,
																  worldPos.z),
													   rotationQuat, motion_type, jolt_layer);

					settings.mAllowDynamicOrKinematic = true;

					// tag body with owning entt::entity so contact listener can map back
					settings.mUserData = static_cast<uint64_t>(static_cast<uint32_t>(entity));

					settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
					settings.mMassPropertiesOverride.mMass = 10.f; // any positive number
					settings.mMassPropertiesOverride.mInertia =		JPH::Mat44::sScale(1.f); // placeholder inertia tensor

					settings.mMotionQuality = JPH::EMotionQuality::LinearCast;

					settings.mFriction = 0.f;
					settings.mAngularDamping = 20.f;

					settings.mAllowedDOFs = JPH::EAllowedDOFs::TranslationX |
						JPH::EAllowedDOFs::TranslationY |
						JPH::EAllowedDOFs::TranslationZ;

					JPH::Vec3 com_offset(0.0f, -0.3f * scale.y, 0.0f);
					settings.mMassPropertiesOverride.mInertia.SetTranslation(com_offset);

					JPH::BodyID body_id = body_interface->CreateAndAddBody(
						settings, JPH::EActivation::Activate);
					rigidBody.bodyID = body_id;

					// Cache the values we just used so we can detect future changes
					rigidBody.last_applied_scale = rigidBody.collider_scale;
					rigidBody.last_applied_offset = rigidBody.collider_offset;

					body_interface->SetLinearVelocity(body_id, JPH::Vec3::sZero());
					body_interface->SetAngularVelocity(body_id, JPH::Vec3::sZero());

					PN_CORE_TRACE("Created Jolt body for entity {} with ID {}",
								  (uint32_t)entity,
								  rigidBody.bodyID.GetIndexAndSequenceNumber());
				}
			}
		}

		void System::applyBounce(entt::registry& registry, entt::entity targetEntity,
								 float jumpImpulse) {
			// Optimized for single entity lookup
			if (!registry.all_of<LocalTransform, Physics::RigidBody3D,
								 Audio::AudioSource>(targetEntity))
				return;

			auto& transform = registry.get<LocalTransform>(targetEntity);
			auto& rigidBody = registry.get<RigidBody3D>(targetEntity);

			// Ground check
			glm::f32 half_height = 0.5f * transform.scale.y;
			JPH::RRayCast ray{};
			ray.mOrigin = JPH::RVec3(transform.position.x,
									 transform.position.y - (half_height + 0.05f),
									 transform.position.z);
			ray.mDirection = JPH::Vec3(0, -1, 0) * 0.5f;

			JPH::RayCastResult result;
			bool onGround = jolt_physics->GetNarrowPhaseQuery().CastRay(ray, result);

			if (onGround) {
				body_interface->ActivateBody(rigidBody.bodyID);
				body_interface->AddImpulse(rigidBody.bodyID, JPH::Vec3(0, jumpImpulse, 0));
			}
		}

		// Update body layer by removing and re-adding it
		void System::updateBodyLayer(JPH::BodyID bodyID, JPH::ObjectLayer newLayer) {
			if (bodyID.IsInvalid() || !body_interface) {
				return;
			}

			// Remove the body
			body_interface->RemoveBody(bodyID);

			// Update its layer
			body_interface->SetObjectLayer(bodyID, newLayer);

			// Re-add it with the new layer
			body_interface->AddBody(bodyID, JPH::EActivation::Activate);
		}

		void System::teleportBodyToTransform(entt::entity e, const LocalTransform& tr,
											 Physics::RigidBody3D& rb) {
			if (!body_interface || rb.bodyID.IsInvalid())
				return;

			JPH::Quat q(tr.rotation.x, tr.rotation.y, tr.rotation.z, tr.rotation.w);
			body_interface->ActivateBody(rb.bodyID);
			body_interface->SetPositionAndRotation(
				rb.bodyID, JPH::RVec3(tr.position.x, tr.position.y, tr.position.z), q,
				JPH::EActivation::Activate);
			// optional: stop any residual motion
			body_interface->SetLinearVelocity(rb.bodyID, JPH::Vec3::sZero());
			body_interface->SetAngularVelocity(rb.bodyID, JPH::Vec3::sZero());
		}

		// In your engine API / physics bridge
		void System::setVelocity(entt::entity e, const glm::vec3& v) {
			if (!body_interface)
				return;

			// Get registry from services
			auto svc = services.lock();
			if (!svc)
				return;

			auto ecs = svc->get<ECS::Controller>();
			if (!ecs)
				return;

			auto& registry = ecs->getRegistry();

			// Safety checks
			if (!registry.valid(e))
				return;
			if (!registry.all_of<Physics::RigidBody3D>(e))
				return;

			auto& rb = registry.get<Physics::RigidBody3D>(e);
			if (rb.bodyID.IsInvalid() || !body_interface)
				return;
			body_interface->SetLinearVelocity(rb.bodyID, JPH::Vec3(v.x, v.y, v.z));
		}

		glm::vec3 System::getVelocity(entt::entity e) const {
			if (!body_interface)
				return {0.f, 0.f, 0.f};

			auto svc = services.lock();
			if (!svc)
				return {0.f, 0.f, 0.f};

			auto ecs = svc->get<ECS::Controller>();
			if (!ecs)
				return {0.f, 0.f, 0.f};

			auto& registry = ecs->getRegistry();

			if (!registry.valid(e) || !registry.all_of<Physics::RigidBody3D>(e))
				return {0.f, 0.f, 0.f};

			auto& rb = registry.get<Physics::RigidBody3D>(e);
			if (rb.bodyID.IsInvalid())
				return {0.f, 0.f, 0.f};

			// Read from Jolt
			JPH::Vec3 vel = body_interface->GetLinearVelocity(rb.bodyID);
			return glm::vec3(vel.GetX(), vel.GetY(), vel.GetZ());
		}

		void System::addForce(entt::entity e, const glm::vec3& force) {
			auto svc = services.lock();
			if (!svc) return;

			auto ecs = svc->get<ECS::Controller>();
			if (!ecs) return;

			auto& registry = ecs->getRegistry();
			if (!registry.valid(e) || !registry.all_of<Physics::RigidBody3D>(e)) return;

			auto& rb = registry.get<Physics::RigidBody3D>(e);
			if (rb.bodyID.IsInvalid() || !body_interface) return;

			// body_interface->AddForce takes world space, local position
			body_interface->AddForce(rb.bodyID,
				JPH::Vec3(force.x, force.y, force.z));
		}

		void System::addImpulse(entt::entity e, const glm::vec3& impulse) {
			auto svc = services.lock();
			if (!svc) return;

			auto ecs = svc->get<ECS::Controller>();
			if (!ecs) return;

			auto& registry = ecs->getRegistry();
			if (!registry.valid(e) || !registry.all_of<Physics::RigidBody3D>(e)) return;

			auto& rb = registry.get<Physics::RigidBody3D>(e);
			if (rb.bodyID.IsInvalid() || !body_interface) return;

			// Activate body + apply impulse (instant velocity change)
			body_interface->ActivateBody(rb.bodyID);
			body_interface->AddImpulse(rb.bodyID,
				JPH::Vec3(impulse.x, impulse.y, impulse.z));
		}

		void System::create_floor() {
			if (!body_interface)
				return;

			// Remove existing floor if any
			remove_floor();

			// Get scene manager for floor settings
			auto svc = services.lock();
			glm::vec3 floorPos(0.0f, -1.0f, 0.0f);
			glm::vec3 floorExtents(100.0f, 1.0f, 100.0f);
			
			if (svc) {
				auto sceneMgr = svc->get<Scene::SceneManager>();
				if (sceneMgr) {
					floorPos = sceneMgr->getFloorPosition();
					floorExtents = sceneMgr->getFloorExtents();
				}
			}

			// Half extents
			JPH::Vec3 halfExtent(floorExtents.x, floorExtents.y, floorExtents.z);

			// Create shape
			JPH::Ref<JPH::BoxShape> floorShape = new JPH::BoxShape(halfExtent, 0.0f);

			// Body settings
			JPH::BodyCreationSettings floorSettings(
				floorShape, JPH::RVec3(floorPos.x, floorPos.y, floorPos.z),
				JPH::Quat::sIdentity(), JPH::EMotionType::Static,
				PAIN::Layer::NON_MOVING);

			// Create and add
			JPH::Body* floorBody = body_interface->CreateBody(floorSettings);
			floor_body_id = floorBody->GetID();
			
			if (floor_enabled) {
				body_interface->AddBody(floor_body_id, JPH::EActivation::DontActivate);
			}
			
			PN_CORE_INFO("[Physics] Floor created at ({:.2f}, {:.2f}, {:.2f}) with extents ({:.2f}, {:.2f}, {:.2f})",
				floorPos.x, floorPos.y, floorPos.z,
				floorExtents.x, floorExtents.y, floorExtents.z);
		}

		void System::remove_floor() {
			if (!body_interface || floor_body_id.IsInvalid())
				return;

			body_interface->RemoveBody(floor_body_id);
			body_interface->DestroyBody(floor_body_id);
			floor_body_id = JPH::BodyID(JPH::BodyID::cInvalidBodyID);
			
			PN_CORE_INFO("[Physics] Floor removed");
		}

		void System::set_floor_enabled(bool enabled) {
			if (floor_enabled == enabled)
				return;

			floor_enabled = enabled;

			if (!body_interface || floor_body_id.IsInvalid())
				return;

			if (enabled) {
				body_interface->AddBody(floor_body_id, JPH::EActivation::DontActivate);
				PN_CORE_INFO("[Physics] Floor enabled");
			} else {
				body_interface->RemoveBody(floor_body_id);
				PN_CORE_INFO("[Physics] Floor disabled");
			}
		}

		// Check if on object
		bool System::isGrounded(JPH::BodyID body_id, float maxDistance) {
			if (!jolt_physics || body_id.IsInvalid())
				return false;

			JPH::RVec3 bodyPosition;
			float shapeRadius = 0.0f;
			float shapeHeight = 0.0f;

			{
				const JPH::BodyLockRead lock(jolt_physics->GetBodyLockInterface(), body_id);
				if (lock.Succeeded()) {
					const JPH::Body& body = lock.GetBody();
					bodyPosition = body.GetPosition();

					const JPH::Shape* shape = body.GetShape();
					JPH::AABox bounds = shape->GetLocalBounds();
					shapeHeight = bounds.mMax.GetY() - bounds.mMin.GetY();
					shapeRadius = std::max(bounds.mMax.GetX() - bounds.mMin.GetX(),
						bounds.mMax.GetZ() - bounds.mMin.GetZ()) * 0.5f;
				}
				else {
					PN_CORE_ERROR("Failed to lock body for reading in isGrounded");
					return false;
				}
			}

			float bottomY = -(shapeHeight * 0.5f) + 0.05f;
			JPH::Vec3 rayDir = JPH::Vec3(0.0f, -1.0f, 0.0f) * maxDistance;
			float checkRadius = shapeRadius * 0.7f;

			JPH::RVec3 rayOrigins[] = {
				bodyPosition + JPH::RVec3(0.0f, bottomY, 0.0f),
				bodyPosition + JPH::RVec3(checkRadius, bottomY, 0.0f),
				bodyPosition + JPH::RVec3(-checkRadius, bottomY, 0.0f),
				bodyPosition + JPH::RVec3(0.0f, bottomY, checkRadius),
				bodyPosition + JPH::RVec3(0.0f, bottomY, -checkRadius),
			};

			JPH::RayCastResult result;
			for (const auto& origin : rayOrigins) {
				JPH::RRayCast ray;
				ray.mOrigin = origin;
				ray.mDirection = rayDir;

				if (jolt_physics->GetNarrowPhaseQuery().CastRay(ray, result)) {
					JPH::Vec3 hitNormal;
					const JPH::BodyLockRead lock(jolt_physics->GetBodyLockInterface(), result.mBodyID);
					if (lock.Succeeded()) {
						const JPH::Body& hitBody = lock.GetBody();
						const JPH::Shape* hitShape = hitBody.GetShape();

						JPH::RVec3 hitPos = ray.mOrigin + ray.mDirection * result.mFraction;

						JPH::Vec3 localPos = hitBody.GetInverseCenterOfMassTransform() * hitPos;

						hitNormal = hitShape->GetSurfaceNormal(
							result.mSubShapeID2,
							localPos
						);

						hitNormal = hitBody.GetRotation() * hitNormal;
					}

					if (hitNormal.GetY() > 0.7f) {
						return true;
					}
					else {
						PN_CORE_INFO("Grounded = FALSE (normal too steep)");
					}
				}
			}

			return false;
		}


		glm::vec3 System::getNormal(entt::entity e) const {

			glm::vec3 n{};

			if (!body_interface) return { 0.f, 0.f, 0.f };

			auto svc = services.lock();
			if (!svc) return { 0.f, 0.f, 0.f };

			auto ecs = svc->get<ECS::Controller>();
			if (!ecs) return { 0.f, 0.f, 0.f };

			auto& registry = ecs->getRegistry();

			if (!registry.valid(e) || !registry.all_of<Physics::RigidBody3D>(e))
				return { 0.f, 0.f, 0.f };

			auto& rb = registry.get<Physics::RigidBody3D>(e);
			if (rb.bodyID.IsInvalid()) return { 0.f, 0.f, 0.f };

			// Calculate normal here
			/*JPH::Vec3 vel = body_interface->GetLinearVelocity(rb.bodyID);

			return glm::vec3(vel.GetX(), vel.GetY(), vel.GetZ());*/

			return { 0.f, 0.f, 0.f }; // Placeholder
		}

		bool System::getWallNormal(JPH::BodyID body_id, const glm::vec3& direction, float checkDistance, glm::vec3& outNormal) {
			if (!jolt_physics || body_id.IsInvalid())
				return false;

			JPH::RVec3 bodyPosition;

			{
				const JPH::BodyLockRead lock(jolt_physics->GetBodyLockInterface(), body_id);
				if (lock.Succeeded()) {
					const JPH::Body& body = lock.GetBody();
					bodyPosition = body.GetPosition();
				}
				else {
					return false;
				}
			}

			// Normalize horizontal direction
			JPH::Vec3 horizontalDir(direction.x, 0.0f, direction.z);
			float len = horizontalDir.Length();
			if (len < 0.001f) return false;
			horizontalDir = horizontalDir / len;

			JPH::RayCastResult result;
			JPH::RRayCast ray;
			ray.mOrigin = bodyPosition;
			ray.mDirection = horizontalDir * checkDistance;

			bool hit = jolt_physics->GetNarrowPhaseQuery().CastRay(ray, result);

			if (hit) {
				// Get the surface normal at hit point
				const JPH::BodyLockRead lock(jolt_physics->GetBodyLockInterface(), result.mBodyID);
				if (lock.Succeeded()) {
					const JPH::Body& hitBody = lock.GetBody();
					const JPH::Shape* hitShape = hitBody.GetShape();

					JPH::RVec3 hitPos = ray.mOrigin + JPH::Vec3(ray.mDirection * result.mFraction);
					JPH::Vec3 normal = hitShape->GetSurfaceNormal(result.mSubShapeID2,
						hitPos - hitBody.GetPosition());

					// Only consider vertical walls (not ground or ceiling)
					if (abs(normal.GetY()) > 0.5f) {
						return false;  // Not a wall
					}

					// Convert to glm
					outNormal = glm::vec3(normal.GetX(), normal.GetY(), normal.GetZ());
					return true;
				}
			}

			return false;
		}

		void System::disablePhysics(entt::entity e) {
			auto svc = services.lock();
			if (!svc) return;

			auto ecs = svc->get<ECS::Controller>();
			if (!ecs) return;

			auto& registry = ecs->getRegistry();
			if (!registry.valid(e) || !registry.all_of<Physics::RigidBody3D>(e)) return;

			auto& rb = registry.get<Physics::RigidBody3D>(e);

			if (!rb.bodyID.IsInvalid() && body_interface) {
				body_interface->RemoveBody(rb.bodyID);
				body_interface->DestroyBody(rb.bodyID);
				rb.bodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);

				PN_CORE_TRACE("[Physics] Disabled physics for entity {}", (uint32_t)e);
			}

			rb.physics_enabled = false;
		}

		void System::enablePhysics(entt::entity e) {
			auto svc = services.lock();
			if (!svc) return;

			auto ecs = svc->get<ECS::Controller>();
			if (!ecs) return;

			auto& registry = ecs->getRegistry();
			if (!registry.valid(e) || !registry.all_of<Physics::RigidBody3D>(e)) return;

			auto& rb = registry.get<Physics::RigidBody3D>(e);

			// Body will be recreated in syncNewBodies on next physics update
			// Just invalidate it to trigger recreation
			rb.bodyID = JPH::BodyID(JPH::BodyID::cInvalidBodyID);

			rb.physics_enabled = true;

			PN_CORE_TRACE("[Physics] Enabled physics for entity {}", (uint32_t)e);
		}
	} // namespace Physics
} // namespace PAIN
