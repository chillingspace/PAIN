#include "pch.h"
#include "Systems/Particle/sysParticleSystem.h"
#include "CoreSystems/Scene/Scene.h"

namespace PAIN {

    namespace ParticleSystem {

        System::System(std::shared_ptr<Services> svc) : ISystem(svc) {
            PN_CORE_INFO("[ParticleSystem] Initialized");
        }

        System::~System() {
            // Clean up all particle systems
            m_ParticleSystems.clear();
        }

        void System::onUpdate(AppTiming timing, entt::registry& registry) {
            auto sceneManager = services.lock()->get<Scene::SceneManager>();
            if (!sceneManager) return;

            auto view = registry.view<ParticleSystemComponent, LocalTransform>();
            
            // Track which entities still have ParticleSystem components
            std::unordered_set<entt::entity> activeEntities;
            
            for (auto [entity, particleComp, transform] : view.each()) {
                activeEntities.insert(entity);
                
                // Get or create particle system instance
                ParticleSystemInstance* psInstance = GetParticleSystem(entity);
                if (!psInstance) continue;
                
                // Check if component was modified (need to reinitialize)
                // For now, just update
                
                // Update particle system
                psInstance->Update(timing.dt, transform.position);
                
                // Sync runtime state back to component for display
                particleComp.state = psInstance->GetConfig().state;
                particleComp.currentPlayTime = psInstance->GetConfig().currentPlayTime;
                particleComp.activeParticleCount = psInstance->GetConfig().activeParticleCount;
            }
            
            // Clean up systems for entities that no longer have ParticleSystem
            CleanupRemovedSystems(registry);
        }

        void System::onFixedUpdate(AppTiming timing, entt::registry& registry) {
            // Particle systems can use regular update for now
        }

        void System::onEvent(Event::Event& e) {
            // Handle particle system events if needed
        }

        void System::InitializeParticleSystem(entt::entity entity, const ParticleSystemComponent& config) {
            auto it = m_ParticleSystems.find(entity);
            if (it != m_ParticleSystems.end()) {
                // Already exists, just reinitialize
                it->second->Shutdown();
                it->second->Initialize(config);
                return;
            }
            
            // Create new particle system
            auto ps = std::make_unique<ParticleSystemInstance>();
            ps->Initialize(config);
            
            // Auto-play if playOnAwake is true
            if (config.playOnAwake) {
                ps->Play();
            }
            
            m_ParticleSystems[entity] = std::move(ps);
        }

        ParticleSystemInstance* System::GetParticleSystem(entt::entity entity) {
            auto it = m_ParticleSystems.find(entity);
            if (it != m_ParticleSystems.end()) {
                return it->second.get();
            }
            return nullptr;
        }

        void System::CleanupRemovedSystems(entt::registry& registry) {
            std::vector<entt::entity> toRemove;
            
            for (auto& [entity, _] : m_ParticleSystems) {
                if (!registry.all_of<ParticleSystemComponent>(entity)) {
                    toRemove.push_back(entity);
                }
            }
            
            for (auto entity : toRemove) {
                m_ParticleSystems.erase(entity);
            }
        }

    } // namespace ParticleSystem

} // namespace PAIN
