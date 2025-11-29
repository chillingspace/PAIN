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
#include "PAINEngine/CoreSystems/Scripting/luaManager.h"
#include "PAINEngine/CoreSystems/Scripting/LuaAIBindings.h"
#include "PAINEngine/Systems/Scripting/GameScriptingSystem.h"
#include "PAINEngine/ECS/Controller.h"
//#include <entt/entt.hpp>

namespace PAIN {

	class Services; 
	namespace Physics { class System; }

	namespace AI {

        class PerceptionSystem {
        public:
            explicit PerceptionSystem(Physics::System* physics);
            void onUpdate(float dt, entt::registry& reg);
            void setPhysics(Physics::System* physics) { physics_ = physics; }

        private:
            Physics::System* physics_ = nullptr;  
            bool canSee(entt::entity self, entt::entity other, entt::registry& reg,
                        float fovDeg, float range, bool requireLOS);
        };

        class BehaviorRuntimeSystem {
        public:
            BehaviorRuntimeSystem() = default;
            BehaviorRuntimeSystem(IEngineAPI* api, LuaManager* luaMgr)
                : api_(api), luaMgr_(luaMgr) {
            }

            void setDependencies(IEngineAPI* api, LuaManager* luaMgr) {
                api_ = api;
                luaMgr_ = luaMgr;
            }

            void onUpdate(float dt, entt::registry& reg);

        private:
            IEngineAPI* api_ = nullptr;
            LuaManager* luaMgr_ = nullptr;

            void tickEntity(float dt, entt::entity e, entt::registry& reg);
            bool lua_decide(entt::entity e, entt::registry& reg, float dt);
        };

        class NavigationSystem {
        public:
            explicit NavigationSystem(Physics::System* physics);
            void onUpdate(float dt, entt::registry& reg);
            void setPhysics(Physics::System* physics) { physics_ = physics; }

        private:
            Physics::System* physics_ = nullptr;
            void startOrUpdatePath(entt::entity e, entt::registry& reg, const glm::vec3& goal);
            void advanceAlongPath(float dt,
                entt::entity e,
                entt::registry& reg);
        };

        class SteeringSystem {
        public:
            explicit SteeringSystem(IEngineAPI* api);
            void onUpdate(float dt, entt::registry& reg);
            void setAPI(IEngineAPI* api) { api_ = api; }

        private:
            IEngineAPI* api_ = nullptr;
            void applyMotion(entt::entity e,
                entt::registry& reg,
                const glm::vec3& vel,
                float dt);
        };

        class AICommandFlushSystem {
        public:
            explicit AICommandFlushSystem(IEngineAPI* api);
            void onUpdate(float dt, entt::registry& reg);
            void setAPI(IEngineAPI* api) { api_ = api; }

        private:
            IEngineAPI* api_ = nullptr;
            void execute(entt::entity e, entt::registry& reg);
        };

        class System : public ECS::System::ISystem {
        public:
            explicit System(std::shared_ptr<Services> svc);

            std::string getSysName() const override { return "AI System"; }

            void onUpdate(AppTiming timing, entt::registry& reg) override;

            void enableAI(bool enable) { b_ai_enabled = enable; }
            bool isAIEnabled() const { return b_ai_enabled; }

		private:
			Physics::System* physics_ = nullptr; // non-owning
			IEngineAPI* engineApi_ = nullptr; // non-owning
            LuaManager* luaMgr_ = nullptr;

			PerceptionSystem      perception_;
			BehaviorRuntimeSystem behavior_;
			NavigationSystem      navigation_;
			SteeringSystem        steering_;
			AICommandFlushSystem  commandFlush_;

			bool b_ai_enabled = true;

            void refreshDependencies(entt::registry& reg);
		};
	}

}

#endif
