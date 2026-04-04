#pragma once

#include "pch.h"
#include <cmath>
#include <algorithm>
#include "ECS/Components/cParticleSystem.h"

namespace PAIN {

    // ============================================
    // Individual Particle Data
    // ============================================
    struct Particle {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        glm::vec4 initialColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        float size = 1.0f;
        float initialSize = 1.0f;
        float rotation = 0.0f;          // radians
        float angularVelocity = 0.0f;   // radians / sec
        float lifetime = 1.0f;    // Total lifetime
        float age = 0.0f;        // Current age
        bool alive = false;
    };

    // ============================================
    // Particle Pool - Object Pooling
    // ============================================
    class ParticlePool {
    public:
        ParticlePool() = default;
        
        void Initialize(int maxParticles) {
            m_MaxParticles = maxParticles;
            m_Particles.resize(maxParticles);
            m_AliveIndices.reserve(maxParticles);
            m_DeadIndices.reserve(maxParticles);
            
            // Initialize all particles as dead
            for (int i = 0; i < maxParticles; ++i) {
                m_DeadIndices.push_back(i);
            }
        }
        
        void Shutdown() {
            m_Particles.clear();
            m_AliveIndices.clear();
            m_DeadIndices.clear();
        }
        
        // Spawn a particle - returns index or -1 if pool is full
        int Spawn() {
            if (m_DeadIndices.empty()) {
                return -1; // Pool exhausted
            }
            
            int idx = m_DeadIndices.back();
            m_DeadIndices.pop_back();
            m_AliveIndices.push_back(idx);
            
            // Reset particle
            auto& p = m_Particles[idx];
            p.alive = true;
            p.age = 0.0f;
            
            return idx;
        }
        
        // Kill a particle
        void Kill(int index) {
            if (index < 0 || index >= m_MaxParticles) return;
            
            auto& p = m_Particles[index];
            if (!p.alive) return;
            
            p.alive = false;
            
            // Remove from alive list (swap with last)
            for (size_t i = 0; i < m_AliveIndices.size(); ++i) {
                if (m_AliveIndices[i] == index) {
                    std::swap(m_AliveIndices[i], m_AliveIndices.back());
                    m_AliveIndices.pop_back();
                    break;
                }
            }
            
            m_DeadIndices.push_back(index);
        }
        
        // Get particle reference
        Particle& GetParticle(int index) {
            return m_Particles[index];
        }
        
        // Update all alive particles - kill those that have exceeded lifetime
        void Update(float deltaTime) {
            std::vector<int> toKill;
            
            for (int idx : m_AliveIndices) {
                auto& p = m_Particles[idx];
                p.age += deltaTime;
                
                if (p.age >= p.lifetime) {
                    toKill.push_back(idx);
                }
            }
            
            for (int idx : toKill) {
                Kill(idx);
            }
        }
        
        // Get count of alive particles
        int GetAliveCount() const { return static_cast<int>(m_AliveIndices.size()); }
        
        // Get count of dead particles
        int GetDeadCount() const { return static_cast<int>(m_DeadIndices.size()); }
        
        // Check if pool is full
        bool IsFull() const { return m_DeadIndices.empty(); }
        
        // Get const access to alive particle indices
        const std::vector<int>& GetAliveIndices() const { return m_AliveIndices; }
        
        // Get raw particle array for GPU rendering
        const Particle* GetParticles() const { return m_Particles.data(); }
        
    private:
        int m_MaxParticles = 0;
        std::vector<Particle> m_Particles;
        std::vector<int> m_AliveIndices;
        std::vector<int> m_DeadIndices;
    };

    // ============================================
    // Particle System Instance - Handles one particle system
    // ============================================
    class ParticleSystemInstance {
    public:
        ParticleSystemInstance() = default;
        
        void Initialize(const ParticleSystemComponent& config) {
            CopyAuthoringFields(config);
            m_Config.state = ParticleSystemState::Stopped;
            m_Config.currentPlayTime = 0.0f;
            m_Config.activeParticleCount = 0;
            m_Pool.Initialize(config.maxParticles);
        }
        
        void Shutdown() {
            m_Pool.Shutdown();
        }
        
        // Playback controls
        void Play() {
            m_Config.state = ParticleSystemState::Playing;
            m_Config.currentPlayTime = 0.0f;
        }
        
        void Pause() {
            m_Config.state = ParticleSystemState::Paused;
        }
        
        void Resume() {
            if (m_Config.state == ParticleSystemState::Paused) {
                m_Config.state = ParticleSystemState::Playing;
            }
        }
        
        void Stop() {
            m_Config.state = ParticleSystemState::Stopped;
            m_Config.currentPlayTime = 0.0f;
            m_EmissionAccumulator = 0.0f;
            
            // Kill all particles
            while (m_Pool.GetAliveCount() > 0) {
                const auto& alive = m_Pool.GetAliveIndices();
                m_Pool.Kill(alive.back());
            }

            m_Config.activeParticleCount = 0;
        }
        
        void Restart() {
            Stop();
            Play();
        }

        void ApplyConfig(const ParticleSystemComponent& config) {
            const bool poolSizeChanged = (config.maxParticles != m_Config.maxParticles);
            const auto previousState = m_Config.state;
            const float previousPlayTime = m_Config.currentPlayTime;

            CopyAuthoringFields(config);

            if (poolSizeChanged) {
                m_Pool.Shutdown();
                m_Pool.Initialize(m_Config.maxParticles);
                m_EmissionAccumulator = 0.0f;
            }

            m_Config.state = previousState;
            m_Config.currentPlayTime = previousPlayTime;
            m_Config.activeParticleCount = m_Pool.GetAliveCount();
        }
        
        // Update the particle system
        void Update(float deltaTime, const glm::vec3& emitterPosition, const glm::quat& emitterRotation) {
            if (!m_HasPreviousEmitterPosition) {
                m_PreviousEmitterPosition = emitterPosition;
                m_PreviousEmitterRotation = emitterRotation;
                m_HasPreviousEmitterPosition = true;
            }

            if (m_Config.simulationSpace == ParticleSimulationSpace::Local) {
                const glm::vec3 delta = emitterPosition - m_PreviousEmitterPosition;
                const glm::quat currentEmitterRotation = glm::normalize(emitterRotation);
                const glm::quat previousEmitterRotation = glm::normalize(m_PreviousEmitterRotation);
                const glm::quat deltaRotation = glm::normalize(currentEmitterRotation * glm::inverse(previousEmitterRotation));
                const bool hasTranslationDelta = glm::dot(delta, delta) > 0.0000001f;
                const bool hasRotationDelta = glm::abs(deltaRotation.w - 1.0f) > 0.00001f;

                if (hasTranslationDelta || hasRotationDelta) {
                    for (int idx : m_Pool.GetAliveIndices()) {
                        Particle& p = m_Pool.GetParticle(idx);
                        const glm::vec3 localOffset = p.position - m_PreviousEmitterPosition;
                        const glm::vec3 rotatedOffset = hasRotationDelta ? (deltaRotation * localOffset) : localOffset;
                        p.position = emitterPosition + rotatedOffset;
                    }
                }
            }
            m_PreviousEmitterPosition = emitterPosition;
            m_PreviousEmitterRotation = emitterRotation;

            if (m_Config.state == ParticleSystemState::Playing) {
                // Check play duration
                m_Config.currentPlayTime += deltaTime;
                if (m_Config.playDuration > 0.0f &&
                    m_Config.currentPlayTime >= m_Config.playDuration) {
                    if (m_Config.looping) {
                        Restart();
                    } else {
                        m_Config.state = ParticleSystemState::Stopped;
                        m_Config.currentPlayTime = 0.0f;
                        m_EmissionAccumulator = 0.0f;
                    }
                }

                if (m_Config.state == ParticleSystemState::Playing) {
                    Emit(deltaTime, emitterPosition, emitterRotation);
                }
            }

            // Burst-emitted particles must continue aging and simulating even while the system is stopped.
            UpdateParticles(deltaTime);
            m_Config.activeParticleCount = m_Pool.GetAliveCount();
        }
        
        // Get current config (for runtime display)
        ParticleSystemComponent& GetConfig() { return m_Config; }
        
        // Get particle pool for rendering
        ParticlePool& GetPool() { return m_Pool; }
        
        // Emit a burst of particles immediately (bypasses emission rate)
        void EmitBurst(int count, const glm::vec3& emitterPosition, const glm::quat& emitterRotation, const glm::vec3& positionOffset = glm::vec3(0.0f)) {
            glm::vec3 adjustedPosition = emitterPosition + positionOffset;
            for (int i = 0; i < count; ++i) {
                if (m_Pool.IsFull()) break;
                
                int idx = m_Pool.Spawn();
                if (idx < 0) break;
                
                Particle& p = m_Pool.GetParticle(idx);
                
                p.lifetime = m_Config.lifetime + m_Config.lifetimeVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.age = 0.0f;
                p.alive = true;
                
                p.position = GetEmissionPosition(adjustedPosition);
                
                float speedVariance = m_Config.speedVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                float actualSpeed = m_Config.speed + speedVariance;
                
                glm::vec3 direction = GetEmissionDirection(p.position, adjustedPosition, emitterRotation);
                p.velocity = direction * actualSpeed;
                
                float sizeVariance = m_Config.startSizeVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialSize = glm::max(0.0f, m_Config.startSize + sizeVariance);
                p.size = p.initialSize;
                
                float startRotVariance = m_Config.startRotationVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.rotation = glm::radians(m_Config.startRotation + startRotVariance);
                
                float angularVelVariance = m_Config.angularVelocityVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.angularVelocity = glm::radians(m_Config.angularVelocity + angularVelVariance);
                
                p.initialColor = m_Config.startColor;
                p.initialColor.r += m_Config.startColorVariance.r * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialColor.g += m_Config.startColorVariance.g * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialColor.b += m_Config.startColorVariance.b * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialColor.a += m_Config.startColorVariance.a * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialColor = glm::clamp(p.initialColor, glm::vec4(0.0f), glm::vec4(1.0f));
                p.color = p.initialColor;
            }
        }
        
    private:
        ParticleSystemComponent m_Config;
        ParticlePool m_Pool;
        
        // Emission accumulation for fractional particles
        float m_EmissionAccumulator = 0.0f;
        glm::vec3 m_PreviousEmitterPosition = glm::vec3(0.0f);
        glm::quat m_PreviousEmitterRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        bool m_HasPreviousEmitterPosition = false;

        static void SortCurves(ParticleSystemComponent& config) {
            auto byTimeFloat = [](const FloatKeyframe& a, const FloatKeyframe& b) { return a.time < b.time; };
            auto byTimeColor = [](const ColorKeyframe& a, const ColorKeyframe& b) { return a.time < b.time; };
            std::sort(config.sizeOverLifetime.begin(), config.sizeOverLifetime.end(), byTimeFloat);
            std::sort(config.colorOverLifetime.begin(), config.colorOverLifetime.end(), byTimeColor);
        }

        void CopyAuthoringFields(const ParticleSystemComponent& config) {
            m_Config.playDuration = config.playDuration;
            m_Config.lifetime = config.lifetime;
            m_Config.lifetimeVariance = config.lifetimeVariance;
            m_Config.looping = config.looping;
            m_Config.playOnAwake = config.playOnAwake;
            m_Config.particleTexture = config.particleTexture;
            m_Config.renderShape = config.renderShape;
            m_Config.softEdge = glm::clamp(config.softEdge, 0.0f, 1.0f);
            m_Config.startSize = glm::max(0.01f, config.startSize);
            m_Config.startSizeVariance = glm::max(0.0f, config.startSizeVariance);
            m_Config.startColor = glm::clamp(config.startColor, glm::vec4(0.0f), glm::vec4(1.0f));
            m_Config.startColorVariance = glm::max(config.startColorVariance, glm::vec4(0.0f));
            m_Config.emissionRate = glm::max(0.0f, config.emissionRate);
            m_Config.maxParticles = glm::max(1, config.maxParticles);
            m_Config.speed = glm::max(0.0f, config.speed);
            m_Config.speedVariance = glm::max(0.0f, config.speedVariance);
            m_Config.velocityOverLifetimeEnabled = config.velocityOverLifetimeEnabled;
            m_Config.velocityOverLifetime = config.velocityOverLifetime;
            m_Config.gravityMultiplier = config.gravityMultiplier;
            m_Config.drag = config.drag;
            m_Config.startRotation = config.startRotation;
            m_Config.startRotationVariance = config.startRotationVariance;
            m_Config.angularVelocity = config.angularVelocity;
            m_Config.angularVelocityVariance = config.angularVelocityVariance;
            m_Config.emissionShape = config.emissionShape;
            m_Config.shapeParams = config.shapeParams;
            m_Config.shapeParams.sphereRadius = glm::max(0.0f, m_Config.shapeParams.sphereRadius);
            m_Config.shapeParams.boxHalfExtents = glm::max(m_Config.shapeParams.boxHalfExtents, glm::vec3(0.0f));
            m_Config.shapeParams.circleRadius = glm::max(0.0f, m_Config.shapeParams.circleRadius);
            m_Config.shapeParams.circleArc = glm::clamp(m_Config.shapeParams.circleArc, 0.0f, 360.0f);
            m_Config.shapeParams.coneAngle = glm::clamp(m_Config.shapeParams.coneAngle, 0.0f, 89.0f);
            m_Config.emissionDirection = config.emissionDirection;
            m_Config.emissionSpread = glm::clamp(config.emissionSpread, 0.0f, 180.0f);
            m_Config.directionMode = config.directionMode;
            m_Config.directionSpace = config.directionSpace;
            m_Config.sizeOverLifetime = config.sizeOverLifetime;
            m_Config.sizeOverLifetimeMultiplier = config.sizeOverLifetimeMultiplier;
            m_Config.colorOverLifetime = config.colorOverLifetime;
            m_Config.simulationSpace = config.simulationSpace;
            m_Config.blendMode = config.blendMode;
            m_Config.sortMode = config.sortMode;
            SortCurves(m_Config);
        }
        
        void Emit(float deltaTime, const glm::vec3& emitterPosition, const glm::quat& emitterRotation) {
            // Calculate how many particles to emit this frame
            float particlesToEmit = m_Config.emissionRate * deltaTime + m_EmissionAccumulator;
            int emitCount = static_cast<int>(particlesToEmit);
            m_EmissionAccumulator = particlesToEmit - emitCount;
            
            // Also handle burst emission if needed (future feature)
            
            for (int i = 0; i < emitCount; ++i) {
                if (m_Pool.IsFull()) break;
                
                int idx = m_Pool.Spawn();
                if (idx < 0) break;
                
                Particle& p = m_Pool.GetParticle(idx);
                
                // Set lifetime with variance
                p.lifetime = m_Config.lifetime + m_Config.lifetimeVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.age = 0.0f;
                p.alive = true;
                
                // Set initial position based on emission shape
                p.position = GetEmissionPosition(emitterPosition);
                
                // Set initial velocity based on direction and speed
                float speedVariance = m_Config.speedVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                float actualSpeed = m_Config.speed + speedVariance;
                
                // Calculate direction with spread
                glm::vec3 direction = GetEmissionDirection(p.position, emitterPosition, emitterRotation);
                p.velocity = direction * actualSpeed;
                
                // Set initial size with variance
                float sizeVariance = m_Config.startSizeVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialSize = glm::max(0.0f, m_Config.startSize + sizeVariance);
                p.size = p.initialSize;

                // Set initial rotation and angular velocity
                float startRotVariance = m_Config.startRotationVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.rotation = glm::radians(m_Config.startRotation + startRotVariance);

                float angularVelVariance = m_Config.angularVelocityVariance * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.angularVelocity = glm::radians(m_Config.angularVelocity + angularVelVariance);
                
                // Set initial color with variance
                p.initialColor = m_Config.startColor;
                p.initialColor.r += m_Config.startColorVariance.r * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialColor.g += m_Config.startColorVariance.g * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialColor.b += m_Config.startColorVariance.b * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialColor.a += m_Config.startColorVariance.a * (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f);
                p.initialColor = glm::clamp(p.initialColor, glm::vec4(0.0f), glm::vec4(1.0f));
                p.color = p.initialColor;
            }
        }
        
        glm::vec3 GetEmissionPosition(const glm::vec3& emitterPosition) {
            switch (m_Config.emissionShape) {
                case EmissionShape::Point:
                    return emitterPosition;
                    
                case EmissionShape::Sphere: {
                    // Random point on sphere surface or volume
                    glm::vec3 randomDir = RandomUnitVector();
                    if (m_Config.shapeParams.volumeEmission) {
                        // Inside sphere
                        float r = m_Config.shapeParams.sphereRadius * std::cbrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX));
                        return emitterPosition + randomDir * r;
                    } else {
                        // On surface
                        return emitterPosition + randomDir * m_Config.shapeParams.sphereRadius;
                    }
                }
                    
                case EmissionShape::Box: {
                    // Random point inside box
                    glm::vec3 randomPoint;
                    randomPoint.x = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f) * m_Config.shapeParams.boxHalfExtents.x;
                    randomPoint.y = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f) * m_Config.shapeParams.boxHalfExtents.y;
                    randomPoint.z = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f - 1.0f) * m_Config.shapeParams.boxHalfExtents.z;
                    return emitterPosition + randomPoint;
                }
                    
                case EmissionShape::Circle: {
                    // Random point in circle (on XZ plane)
                    float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * glm::radians(m_Config.shapeParams.circleArc);
                    float r = std::sqrt(static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * m_Config.shapeParams.circleRadius;
                    return emitterPosition + glm::vec3(std::cos(angle) * r, 0.0f, std::sin(angle) * r);
                }
                    
                case EmissionShape::Cone: {
                    // Random point in cone
                    float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159f;
                    float h = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                    const float coneAngle = glm::clamp(m_Config.shapeParams.coneAngle, 0.0f, 89.0f);
                    float r = h * std::tan(glm::radians(coneAngle));
                    return emitterPosition + glm::vec3(std::cos(angle) * r, h, std::sin(angle) * r);
                }
                    
                default:
                    return emitterPosition;
            }
        }
        
        glm::vec3 SafeNormalize(const glm::vec3& v, const glm::vec3& fallback = glm::vec3(0.0f, 1.0f, 0.0f)) {
            const float len2 = glm::dot(v, v);
            if (len2 < 0.000001f) {
                return fallback;
            }
            return glm::normalize(v);
        }

        glm::vec3 GetEmissionDirection(const glm::vec3& spawnPosition, const glm::vec3& emitterPosition, const glm::quat& emitterRotation) {
            glm::vec3 dir(0.0f, 1.0f, 0.0f);

            switch (m_Config.directionMode) {
            case ParticleDirectionMode::ShapeNormal:
                dir = SafeNormalize(spawnPosition - emitterPosition, SafeNormalize(m_Config.emissionDirection));
                break;
            case ParticleDirectionMode::Random:
                dir = RandomUnitVector();
                break;
            case ParticleDirectionMode::Custom:
            default:
                dir = SafeNormalize(m_Config.emissionDirection);
                break;
            }

            if (m_Config.directionSpace == ParticleDirectionSpace::Local) {
                dir = SafeNormalize(emitterRotation * dir, dir);
            }

            // Apply spread
            if (m_Config.emissionSpread > 0.0f) {
                float spreadRad = glm::radians(m_Config.emissionSpread);
                float randomFactor = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
                glm::vec3 randomOffset = RandomUnitVector() * randomFactor * spreadRad;
                dir = SafeNormalize(dir + randomOffset, dir);
            }

            return dir;
        }
        
        glm::vec3 RandomUnitVector() {
            float theta = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159f;
            float phi = std::acos(2.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX) - 1.0f);
            
            return glm::vec3(
                std::sin(phi) * std::cos(theta),
                std::sin(phi) * std::sin(theta),
                std::cos(phi)
            );
        }
        
        void UpdateParticles(float deltaTime) {
            m_Pool.Update(deltaTime);
            
            for (int idx : m_Pool.GetAliveIndices()) {
                Particle& p = m_Pool.GetParticle(idx);
                
                // Update position
                p.position += p.velocity * deltaTime;
                
                // Apply gravity and drag
                p.velocity.y -= 9.81f * m_Config.gravityMultiplier * deltaTime;
                const float dragFactor = glm::max(0.0f, 1.0f - m_Config.drag * deltaTime);
                p.velocity *= dragFactor;

                if (m_Config.velocityOverLifetimeEnabled) {
                    p.velocity += m_Config.velocityOverLifetime * deltaTime;
                }

                p.rotation += p.angularVelocity * deltaTime;
                
                // Update size based on lifetime curve
                float normalizedAge = p.age / p.lifetime;
                const float sizeFactor = EvaluateSizeCurve(normalizedAge) * m_Config.sizeOverLifetimeMultiplier;
                p.size = glm::max(0.0f, p.initialSize * sizeFactor);
                
                // Update color based on lifetime curve
                p.color = glm::clamp(p.initialColor * EvaluateColorCurve(normalizedAge), glm::vec4(0.0f), glm::vec4(1.0f));
            }
        }
        
        float EvaluateSizeCurve(float normalizedAge) {
            if (m_Config.sizeOverLifetime.empty()) {
                return 1.0f;
            }
            
            return EvaluateFloatCurve(m_Config.sizeOverLifetime, normalizedAge);
        }
        
        glm::vec4 EvaluateColorCurve(float normalizedAge) {
            if (m_Config.colorOverLifetime.empty()) {
                return m_Config.startColor;
            }
            
            return EvaluateColorKeyframeCurve(m_Config.colorOverLifetime, normalizedAge);
        }
        
        float EvaluateFloatCurve(const std::vector<FloatKeyframe>& curve, float t) {
            if (curve.size() == 0) return 1.0f;
            if (curve.size() == 1) return curve[0].value;
            
            // Find the two keyframes to interpolate between
            for (size_t i = 0; i < curve.size() - 1; ++i) {
                if (t >= curve[i].time && t <= curve[i + 1].time) {
                    float range = curve[i + 1].time - curve[i].time;
                    if (range <= 0.0f) return curve[i].value;
                    
                    float localT = (t - curve[i].time) / range;
                    return glm::mix(curve[i].value, curve[i + 1].value, localT);
                }
            }
            
            // Extrapolate
            if (t < curve[0].time) return curve[0].value;
            return curve.back().value;
        }
        
        glm::vec4 EvaluateColorKeyframeCurve(const std::vector<ColorKeyframe>& curve, float t) {
            if (curve.size() == 0) return m_Config.startColor;
            if (curve.size() == 1) return curve[0].color;
            
            for (size_t i = 0; i < curve.size() - 1; ++i) {
                if (t >= curve[i].time && t <= curve[i + 1].time) {
                    float range = curve[i + 1].time - curve[i].time;
                    if (range <= 0.0f) return curve[i].color;
                    
                    float localT = (t - curve[i].time) / range;
                    return glm::mix(curve[i].color, curve[i + 1].color, localT);
                }
            }
            
            if (t < curve[0].time) return curve[0].color;
            return curve.back().color;
        }
    };

} // namespace PAIN
