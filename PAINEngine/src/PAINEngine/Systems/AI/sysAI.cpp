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


static IEngineAPI* GetEngineAPI(std::shared_ptr<PAIN::Services> svc) {
	return svc->get<IEngineAPI>().get();
}

namespace PAIN {
	namespace AI {
		/*======================== Perception ========================*/
		PerceptionSystem::PerceptionSystem(std::shared_ptr<PAIN::Services> services)
			: services_(std::move(services)) {
			ensureCache();
		}

		bool PerceptionSystem::ensureCache() {
			if (!phys_) phys_ = services_->get<PAIN::Physics::System>();
			return (bool)phys_;
		}

		static glm::vec3 forward_of(entt::registry& reg, entt::entity e) {
			auto& t = reg.get<PAIN::Transform>(e);
			return t.forward();
		}

		bool PerceptionSystem::canSee(entt::entity self, entt::entity other, entt::registry& reg,
			float fovDeg, float range, bool requireLOS)
		{
			auto& ts = reg.get<PAIN::Transform>(self);
			auto& to = reg.get<PAIN::Transform>(other);
			glm::vec3 delta = to.position - ts.position;
			//float dist2 = glm::length2(delta);
			float dist2 = glm::dot(delta, delta);

			if (dist2 > range * range) return false;

			glm::vec3 fwd = forward_of(reg, self);
			float cosang = glm::dot(glm::normalize(delta), glm::normalize(fwd));
			float fovCos = std::cos(glm::radians(fovDeg * 0.5f));
			if (cosang < fovCos) return false;

			if (requireLOS && phys_) {
				// Raycast: if hit something before 'other', LOS blocked
				// Pseudocode: phys_->raycast(ts.position, to.position, mask);
				// Return true if only hits 'other' collider or nothing blocks.
			}
			return true;
		}

		void PerceptionSystem::onUpdate(float dt, entt::registry& reg) {
			(void)dt;

			auto sensorView = reg.view<Sensors, PAIN::Transform>();
			auto transformView = reg.view<PAIN::Transform>();

			// !TODO: naive N^2 sample; replace with spatial query later
			
			// Iterate all entities that have Sensors + Transform
			sensorView.each([&](entt::entity e, Sensors& sens, PAIN::Transform& ts) {
				sens.visible_targets.clear();

				// Iterate all entities that have Transform
				transformView.each([&](entt::entity other, PAIN::Transform& to) {
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
		BehaviorRuntimeSystem::BehaviorRuntimeSystem(std::shared_ptr<PAIN::Services> services, IEngineAPI* api)
			: services_(std::move(services)), api_(api) {
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
					// Pick a dummy local waypoint; plug your patrol system here.
					auto& t = reg.get<PAIN::Transform>(e);
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
			if (!reg.all_of<Blackboard, CommandQueue, NavAgent>(e)) {
				reg.emplace<Blackboard>(e);
				reg.emplace<CommandQueue>(e);
				reg.emplace<NavAgent>(e);
			}
			// Decide (Lua or trivial fallback)
			lua_decide(e, reg);
		}

		System::System(std::shared_ptr<Services> svc): ECS::System::ISystem(svc)
			, services_(svc)
			, perception_(svc)
			, behavior_(svc, GetEngineAPI(svc))
			//, navigation_(svc)
			//, steering_(svc, GetEngineAPI(svc))
			//, commandFlush_(svc, GetEngineAPI(svc))
		{}


		void System::onUpdate(AppTiming timing, entt::registry& reg) {
			if (!b_ai_enabled)
				return;

			float dt = timing.dt;

			perception_.onUpdate(dt, reg);
			behavior_.onUpdate(dt, reg);
			//navigation_.onUpdate(dt, reg);
			//steering_.onUpdate(dt, reg);
			//commandFlush_.onUpdate(dt, reg);
		}


		// Old Code Below		
		//void System::aiSetup()
		//{
		//	// Initialize AI-specific data structures
		//	// Setup behavior trees, state machines, or decision systems
		//	// Register AI component types if needed

		//	accumulated_time = 0.0f;
		//	b_ai_enabled = true;

		//	PN_CORE_TRACE("AI System setup complete");
		//}

		//System::System(std::shared_ptr<Services> svc) : ISystem(svc), c_max_ai_entities(1024), c_ai_update_interval(0.1f), accumulated_time(0.0f)
		//{
		//	aiSetup();
		//}


		//System::~System()
		//{
		//	// Cleanup AI resources
		//	// Clear behavior trees, navigation data, etc.

		//	PN_CORE_TRACE("AI System destroyed");
		//}

		//void System::onUpdate(AppTiming timing, entt::registry& registry)
		//{
		//	// Skip AI updates if disabled
		//	if (!b_ai_enabled) return;

		//	// Main AI update logic
		//	// Process AI entities based on components
		//	// Execute behavior trees, state machines, pathfinding, etc.

		//	// Implement time-sliced updates for better performance
		//	accumulated_time += timing.dt;

		//	if (accumulated_time >= c_ai_update_interval)
		//	{
		//		// Update AI logic here
		//		// Iterate through entities with AI components
		//		// Execute decision-making, pathfinding, behavior selection

		//		// TODO: Add actual AI entity processing when AI components are registered
		//		// Example:
		//		// for (auto entity : ai_entities) {
		//		//     processAILogic(entity);
		//		// }

		//		accumulated_time -= c_ai_update_interval;
		//	}
		//}
		//void System::onEvent(Event::Event& e)
		//{
		//	// Handle AI-related events
		//	// e.g., target acquired, path blocked, entity spawned

		//	// Example event handling structure:
		//	// Event::EventDispatcher dispatcher(e);
		//	// dispatcher.dispatch<Event::EntitySpawnedEvent>(PN_BIND_EVENT_FN(System::onEntitySpawned));
		//	// dispatcher.dispatch<Event::EntityDestroyedEvent>(PN_BIND_EVENT_FN(System::onEntityDestroyed));
		//}

		//void System::enableAI(bool enable)
		//{
		//	b_ai_enabled = enable;
		//	PN_CORE_TRACE("AI System {0}", enable ? "enabled" : "disabled");
		//}

		//bool System::isAIEnabled() const
		//{
		//	return b_ai_enabled;
		//}

	}
}
