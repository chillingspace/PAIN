/*****************************************************************//**
 * \file   sysAI.cpp
 * \brief  Definition of AI system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sysAI.h"


//static IEngineAPI* GetEngineAPI(std::shared_ptr<PAIN::Services> svc) {
//    if (!svc) return nullptr;
//    auto api = svc->get<IEngineAPI>();
//    return api ? api.get() : nullptr;
//}

static PAIN::Physics::System* GetPhysicsSystem(std::shared_ptr<PAIN::Services> svc) {
	if (!svc) return nullptr;

	// get ECS controller from Services
	auto ecs = svc->get<PAIN::ECS::Controller>();
	if (!ecs) return nullptr;

	// ask the controller for Physics::System
	auto physSys = ecs->getSystem<PAIN::Physics::System>();
	return physSys ? physSys.get() : nullptr;
}

namespace PAIN {
	namespace AI {
		/*======================== Perception ========================*/
		PerceptionSystem::PerceptionSystem(Physics::System* physics)
			: physics_(physics)
		{
			 if (!physics_) PN_CORE_WARN("PerceptionSystem created with null Physics::System");
		}

		static glm::vec3 forward_of(entt::registry& reg, entt::entity e) {
			//auto& t = reg.get<PAIN::LocalTransform>(e);
			//return t.forward();
			return glm::vec3(0);
		}

		bool PerceptionSystem::canSee(entt::entity self, entt::entity other, entt::registry& reg,
			float fovDeg, float range, bool requireLOS)
		{
			auto& ts = reg.get<PAIN::LocalTransform>(self);
			auto& to = reg.get<PAIN::LocalTransform>(other);
			glm::vec3 delta = to.position - ts.position;
			float dist2 = glm::dot(delta, delta);

			if (dist2 > range * range) return false;

			glm::vec3 fwd = forward_of(reg, self);
			float cosang = glm::dot(glm::normalize(delta), glm::normalize(fwd));
			float fovCos = std::cos(glm::radians(fovDeg * 0.5f));
			if (cosang < fovCos) return false;

			if (requireLOS && physics_) {
				// Raycast: if hit something before 'other', LOS blocked
				// Pseudocode: phys_->raycast(ts.position, to.position, mask);
				// Return true if only hits 'other' collider or nothing blocks.
			}
			return true;
		}

		void PerceptionSystem::onUpdate(float dt, entt::registry& reg) {
			(void)dt;

			auto sensorView = reg.view<Sensors, PAIN::LocalTransform>();
			auto transformView = reg.view<PAIN::LocalTransform>();

			// !TODO: naive N^2 sample; replace with spatial query later
			
			// Iterate all entities that have Sensors + Transform
			sensorView.each([&](entt::entity e, Sensors& sens, PAIN::LocalTransform& ts) {
				sens.visible_targets.clear();

				// Iterate all entities that have Transform
				transformView.each([&](entt::entity other, PAIN::LocalTransform& to) {
					if (other == e) return; // skip self

					if (canSee(e, other, reg,
						sens.cfg.sight_fov_deg,
						sens.cfg.sight_range,
						sens.cfg.require_los)) {
						sens.visible_targets.push_back(other);
					}
					});

				// write summary facts into Blackboard
				if (reg.any_of<Blackboard>(e)) {
					auto& bb = reg.get<Blackboard>(e);
					bb.set<bool>("hasTargets", !sens.visible_targets.empty());
					if (!sens.visible_targets.empty()) {
						bb.set<std::uint32_t>("targetId",
							static_cast<std::uint32_t>(sens.visible_targets.front()));
					}
				}
				});
		}

		/*======================== Behavior Runtime ========================*/
		BehaviorRuntimeSystem::BehaviorRuntimeSystem(IEngineAPI* api)
			: api_(api)
		{
			 //if (!api_) PN_CORE_WARN("BehaviorRuntimeSystem created with null IEngineAPI");
		}

		void BehaviorRuntimeSystem::onUpdate(float dt, entt::registry& reg) {
			auto view = reg.view<Controller>();
			for (auto e : view) {
				auto& ctrl = view.get<Controller>(e);
				if (!ctrl.enabled) continue;
				ctrl.accum_dt += dt;
				if (ctrl.accum_dt < ctrl.tick_interval) continue;
				ctrl.accum_dt = 0.0f;
				tickEntity(dt, e, reg);
			}
		}

		bool BehaviorRuntimeSystem::lua_decide(entt::entity e, entt::registry& reg) {
			// This is a narrow junction point to your LuaManager.
			// Example idea:
			// auto lua = services_->get<LuaManager>();
			// return lua->tick_ai_behavior(e, reg);
			// For now, do a trivial “patrol or chase” decision using Blackboard facts.
			auto& bb = reg.get<Blackboard>(e);
			bool hasTargets = bb.get_bool("hasTargets", false);
			if (hasTargets) {
				// enqueue a chase command
				auto& cq = reg.get_or_emplace<CommandQueue>(e);
				// Ask C++ to replan towards current target each tick; NavigationSystem will handle dedup.
				if (auto tid = bb.get<std::uint32_t>("targetId")) {
					// In a real build you'd expose a safe API to query target position; here we just set RequestPath and MoveTarget.
					cq.push({ CommandType::RequestPath });
				}
				return true;
			}
			else {
				// patrol: set/keep a waypoint if not existing
				auto& nav = reg.get_or_emplace<NavAgent>(e);
				if (!nav.move_target.has_value()) {
					// Pick a dummy local waypoint; plug patrol system here.
					auto& t = reg.get<PAIN::LocalTransform>(e);
					nav.move_target = t.position + glm::vec3{ 3.0f, 0.0f, 0.0f };
					auto& cq = reg.get_or_emplace<CommandQueue>(e);
					cq.push({ CommandType::SetMoveTarget, *nav.move_target });
				}
				return true;
			}
		}

		void BehaviorRuntimeSystem::tickEntity(float dt, entt::entity e, entt::registry& reg) {
			(void)dt;
			// Pre-conditions: ensure components
			if (!reg.any_of<Blackboard>(e)) {
				reg.emplace<Blackboard>(e);
			}
			if (!reg.any_of<CommandQueue>(e)) {
				reg.emplace<CommandQueue>(e);
			}
			if (!reg.any_of<NavAgent>(e)) {
				reg.emplace<NavAgent>(e);
			}
			// Decide (Lua or trivial fallback)
			lua_decide(e, reg);
		}

		/*======================== Navigation ========================*/
		NavigationSystem::NavigationSystem(Physics::System* physics)
			: physics_(physics)
		{
			if (!physics_) PN_CORE_WARN("NavigationSystem created with null Physics::System");
			// might use physics_ later (for navmesh queries, raycasts or something idk)
		}


		void NavigationSystem::startOrUpdatePath(entt::entity e, entt::registry& reg, const glm::vec3& goal) {
			auto& agent = reg.get<NavAgent>(e);
			if (agent.has_request_in_flight) return;

			// !TODO: Integrate navmesh/path service. For now, synthesize a tiny path:
			agent.path.clear();
			auto& t = reg.get<PAIN::LocalTransform>(e);
			agent.path.push_back(goal);
			agent.path_index = 0;
			agent.arrived = false;
			agent.has_request_in_flight = false;
		}

		void NavigationSystem::advanceAlongPath(float dt, entt::entity e, entt::registry& reg) {
			auto& agent = reg.get<NavAgent>(e);
			if (agent.path.empty()) return;

			auto& t = reg.get<PAIN::LocalTransform>(e);
			glm::vec3 target = agent.path[agent.path_index];
			glm::vec3 delta = target - t.position;
			float dist = glm::length(delta);
			if (dist <= agent.arrival_radius) {
				agent.path_index++;
				if (agent.path_index >= agent.path.size()) {
					agent.arrived = true;
					agent.path.clear();
					return;
				}
				return;
			}
			// write desired velocity for Steering to consume
			auto& steer = reg.get_or_emplace<Steering>(e);
			steer.desired_velocity = (dist > 0.0001f ? (delta / dist) : glm::vec3{}) * agent.speed;
		}

		void NavigationSystem::onUpdate(float dt, entt::registry& reg) {
			auto navView = reg.view<NavAgent, PAIN::LocalTransform>();

			navView.each([&](entt::entity e, NavAgent& agent, PAIN::LocalTransform&) {

				if (!agent.move_target.has_value())
					return;

				agent.replan_timer -= dt;

				if (agent.path.empty() || agent.replan_timer <= 0.0f) {
					startOrUpdatePath(e, reg, *agent.move_target);
					agent.replan_timer = agent.replan_cooldown;
				}

				advanceAlongPath(dt, e, reg);
				});
		}

		/*======================== Steering / Motion ========================*/
		SteeringSystem::SteeringSystem(IEngineAPI* api)
			: api_(api)
		{
			//if (!api_) PN_CORE_WARN("SteeringSystem created with null IEngineAPI");
		}


		void SteeringSystem::applyMotion(entt::entity e, entt::registry& reg, const glm::vec3& vel, float dt) {
			// Option A: kinematic integration + physics teleport (keeps Transform/Physics in sync):
			auto& t = reg.get<PAIN::LocalTransform>(e);
			t.position += vel * dt;

			// If entity has RigidBody3D, use your physics sync helper from EngineAPIAdapter:
			if (reg.any_of<PAIN::Physics::RigidBody3D>(e) && api_) {
				api_->SetPosition(e, t.position); // adapter will call phys->teleportBodyToTransform(...)
			}
		}

		void SteeringSystem::onUpdate(float dt, entt::registry& reg) {
			auto view = reg.view<Steering>();
			for (auto e : view) {
				auto& s = view.get<Steering>(e);
				if (glm::dot(s.desired_velocity, s.desired_velocity) > 0.0f) {
					applyMotion(e, reg, s.desired_velocity, dt);
				}
				// clear after use (Navigation will refill next frame)
				s.desired_velocity = {};
			}
		}

		/*======================== Command Flush ========================*/
		AICommandFlushSystem::AICommandFlushSystem(IEngineAPI* api)
			: api_(api)
		{
			//if (!api_) PN_CORE_WARN("AICommandFlushSystem created with null IEngineAPI");
		}


		void AICommandFlushSystem::execute(entt::entity e, entt::registry& reg) {
			auto& q = reg.get<CommandQueue>(e);
			auto& agent = reg.get_or_emplace<NavAgent>(e);

			for (const auto& cmd : q.pending) {
				switch (cmd.type) {
				case CommandType::SetMoveTarget:
					agent.move_target = cmd.v3;
					agent.arrived = false;
					break;
				case CommandType::ClearMoveTarget:
					agent.move_target.reset();
					agent.arrived = true;
					agent.path.clear();
					break;
				case CommandType::RequestPath:
					if (agent.move_target.has_value()) {
						// NavigationSystem will detect and (re)plan on next tick.
					}
					break;
				case CommandType::PlayAnimation:
					// !TODO: The below is commented because I don't think animation is ready yet
					//if (api_) api_->PlayAnimation(e, cmd.str.c_str()); // expose this in your adapter
					break;
				case CommandType::FaceEntity:
					// Optional: set a blackboard/steering facing directive
					break;
				default: break;
				}
			}
			q.clear();
		}

		void AICommandFlushSystem::onUpdate(float dt, entt::registry& reg) {
			(void)dt;
			auto view = reg.view<CommandQueue>();
			for (auto e : view) execute(e, reg);
		}

		void System::refreshDependencies() {
			if (auto svc = services.lock()) {

				// Try to find Physics system
				if (!physics_) {
					if (auto ecs = svc->get<ECS::Controller>()) {
						if (auto physSys = ecs->getSystem<Physics::System>()) {
							physics_ = physSys.get();
							PN_CORE_TRACE("AI::System bound Physics::System");
						}
					}
				}

				// Try to find Engine API
				// !TODO: Fix IEngineAPI not registering
				if (!engineApi_) {
					//if (auto api = svc->get<IEngineAPI>()) {
					//	engineApi_ = api.get();
					//	PN_CORE_TRACE("AI::System bound IEngineAPI");
					//}
				}
			}

			// Update sub-systems with the latest pointers
			perception_ = PerceptionSystem{ physics_ };
			behavior_ = BehaviorRuntimeSystem{ engineApi_ };
			navigation_ = NavigationSystem{ physics_ };
			steering_ = SteeringSystem{ engineApi_ };
			commandFlush_ = AICommandFlushSystem{ engineApi_ };
		}


		System::System(std::shared_ptr<Services> svc)
			: ECS::System::ISystem(svc)
			, physics_(nullptr)
			, engineApi_(nullptr)
			, perception_(nullptr)
			, behavior_(nullptr)
			, navigation_(nullptr)
			, steering_(nullptr)
			, commandFlush_(nullptr)
		{
			refreshDependencies();

			PN_CORE_TRACE("AI::System constructed. physics_={} engineApi_={}",
				physics_ ? "ok" : "null",
				engineApi_ ? "ok" : "null");
		}


		void System::onUpdate(AppTiming timing, entt::registry& reg) {
			if (!b_ai_enabled)
				return;

			// bind dependencies if they were null at startup
			refreshDependencies();

			float dt = timing.dt;

			perception_.onUpdate(dt, reg);
			behavior_.onUpdate(dt, reg);
			navigation_.onUpdate(dt, reg);
			steering_.onUpdate(dt, reg);
			commandFlush_.onUpdate(dt, reg);
		}

	}
}
