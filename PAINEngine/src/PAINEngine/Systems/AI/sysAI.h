/*****************************************************************//**
 * \file   sysAI.h
 * \brief  Declaration of AI system states
 *
 * \author Nicole Esther Lee, 2301544, lee.n@digipen.edu (100%)
 * \co-author
 * \date   5 October 2025
 * All content © 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifndef SYS_AI_H
#define SYS_AI_H

#include "pch.h"
#include "ECS/System/ISystem.h"
#include "PAINEngine/Systems/Physics/sysPhysics.h" 
#include "PAINEngine/CoreSystems/Scripting/IEngineAPI.h"
//#include <entt/entt.hpp>

namespace PAIN {

	class Services; 
	namespace Physics { class System; }

	namespace AI {
		class PerceptionSystem {
		public:
			explicit PerceptionSystem(std::shared_ptr<Services> services);
			void onUpdate(float dt, entt::registry& reg);
		private:
			std::shared_ptr<Services> services_;
			std::shared_ptr<Physics::System> phys_;
			bool ensureCache();
			bool canSee(entt::entity self, entt::entity other, entt::registry& reg, float fovDeg, float range, bool requireLOS);
		};

		class BehaviorRuntimeSystem {
		public:
			BehaviorRuntimeSystem(std::shared_ptr<Services> services, IEngineAPI* api);
			void onUpdate(float dt, entt::registry& reg);
		private:
			std::shared_ptr<Services> services_;
			IEngineAPI* api_; // used to enqueue safe engine commands
			void tickEntity(float dt, entt::entity e, entt::registry& reg);
			// hooks: call into LuaManager (registered elsewhere) through narrow API
			bool lua_decide(entt::entity e, entt::registry& reg); // reads/writes blackboard, pushes commands
		};

		class NavigationSystem {
		public:
			explicit NavigationSystem(std::shared_ptr<Services> services);
			void onUpdate(float dt, entt::registry& reg);
		private:
			std::shared_ptr<Services> services_;
			void startOrUpdatePath(entt::entity e, entt::registry& reg, const glm::vec3& goal);
			void advanceAlongPath(float dt, entt::entity e, entt::registry& reg);
		};

		class SteeringSystem {
		public:
			explicit SteeringSystem(std::shared_ptr<Services> services, IEngineAPI* api);
			void onUpdate(float dt, entt::registry& reg);
		private:
			std::shared_ptr<Services> services_;
			IEngineAPI* api_;
			void applyMotion(entt::entity e, entt::registry& reg, const glm::vec3& vel, float dt);
		};

		class AICommandFlushSystem {
		public:
			explicit AICommandFlushSystem(std::shared_ptr<Services> services, IEngineAPI* api);
			void onUpdate(float dt, entt::registry& reg);
		private:
			std::shared_ptr<Services> services_;
			IEngineAPI* api_;
			void execute(entt::entity e, entt::registry& reg);
		};

		class System : public ECS::System::ISystem {
		public:
			explicit System(std::shared_ptr<Services> svc);

			std::string getSysName() const override { return "AI System"; }

			void onUpdate(AppTiming timing, entt::registry& reg) override;
			//void onFixedUpdate(AppTiming, entt::registry&) override {}
			//void onEvent(Event::Event& e) override {}                 
			void enableAI(bool enable) { b_ai_enabled = enable; }
			bool isAIEnabled() const { return b_ai_enabled; }

		private:
			std::shared_ptr<Services> services_; // local strong ref

			PerceptionSystem      perception_;
			BehaviorRuntimeSystem behavior_;
			NavigationSystem      navigation_;
			SteeringSystem        steering_;
			AICommandFlushSystem  commandFlush_;

			bool b_ai_enabled = true;
		};
	}

}

#endif
