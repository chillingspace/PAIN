#pragma once

#include "pch.h"
#include "ECS/System/ISystem.h"
#include "ECS/Components/cParticleSystem.h"
#include "Systems/Particle/sysParticle.h"

namespace PAIN {

    namespace ParticleSystem {

        class System : public ECS::System::ISystem
        {
        public:
            explicit System(std::shared_ptr<Services> svc);
            ~System();

            // Virtual override methods for system lifecycle
            void onUpdate(AppTiming timing, entt::registry& reg) override;
            void onFixedUpdate(AppTiming timing, entt::registry& reg) override;
            void onEvent(Event::Event& e) override;
            std::string getSysName() const override { return "Particle System"; }
            
            // Get particle system instance for an entity (for rendering)
            ParticleSystemInstance* GetParticleSystem(entt::entity entity);
            
        private:
            // Map entity to particle system instance
            std::unordered_map<entt::entity, std::unique_ptr<PAIN::ParticleSystemInstance>> m_ParticleSystems;
            
            // Initialize particle system for an entity
            void InitializeParticleSystem(entt::entity entity, const ParticleSystemComponent& config);
            
            // Clean up particle system for removed entities
            void CleanupRemovedSystems(entt::registry& reg);
        };

    } // namespace ParticleSystem

} // namespace PAIN
