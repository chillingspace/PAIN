#pragma once

#include "pch.h"
#include <cmath>
#include "ECS/Components/cParticleSystem.h"

namespace PAIN {

    // ============================================
    // Individual Particle Data
    // ============================================
    struct Particle {
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
        glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        float size = 1.0f;
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
        
        void Initialize(const ParticleSystem& config) {
            m_Config = config;
            m_Pool.Initialize(config.maxParticles);
            m_Config.activeParticleCount = 0;
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
            
            // Kill all particles
            for (int idx : m_Pool.GetAliveIndices()) {
                m_Pool.Kill(idx);
            }
        }
        
        void Restart() {
            Stop();
            Play();
        }
        
        // Update the particle system
        void Update(float deltaTime, const glm::vec3& emitterPosition) {
            if (m_Config.state != ParticleSystemState::Playing) {
                return;
            }
            
            // Check play duration
            m_Config.currentPlayTime += deltaTime;
            if (m_Config.playDuration > 0.0f && 
                m_Config.currentPlayTime >= m_Config.playDuration) {
                if (m_Config.looping) {
                    Restart();
                } else {
                    Stop();
                    return;
                }
            }
            
            // Emit new particles
            Emit(deltaTime, emitterPosition);
            
            // Update existing particles
            UpdateParticles(deltaTime);
            
            m_Config.activeParticleCount = m_Pool.GetAliveCount();
        }
        
        // Get current config (for runtime display)
        ParticleSystem& GetConfig() { return m_Config; }
        
    private:
        ParticleSystem m_Config;
        ParticlePool m_Pool;
        
        // Emission accumulation for fractional particles
        float m_EmissionAccumulator = 0.0f;
        
        void Emit(float deltaTime, const glm::vec3& emitterPosition) {
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
                p.lifetime = m_Config.lifetime + m_Config.lifetimeVariance * (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
                p.age = 0.0f;
                p.alive = true;
                
                // Set initial position based on emission shape
                p.position = GetEmissionPosition(emitterPosition);
                
                // Set initial velocity based on direction and speed
                float speedVariance = m_Config.speedVariance * (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
                float actualSpeed = m_Config.speed + speedVariance;
                
                // Calculate direction with spread
                glm::vec3 direction = GetEmissionDirection();
                p.velocity = direction * actualSpeed;
                
                // Set initial size with variance
                float sizeVariance = m_Config.startSizeVariance * (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
                p.size = m_Config.startSize + sizeVariance;
                
                // Set initial color with variance
                p.color = m_Config.startColor;
                p.color.r += m_Config.startColorVariance.r * (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
                p.color.g += m_Config.startColorVariance.g * (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
                p.color.b += m_Config.startColorVariance.b * (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
                p.color.a += m_Config.startColorVariance.a * (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f);
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
                        float r = m_Config.shapeParams.sphereRadius * std::cbrt(static_cast<float>(rand()) / RAND_MAX);
                        return emitterPosition + randomDir * r;
                    } else {
                        // On surface
                        return emitterPosition + randomDir * m_Config.shapeParams.sphereRadius;
                    }
                }
                    
                case EmissionShape::Box: {
                    // Random point inside box
                    glm::vec3 randomPoint;
                    randomPoint.x = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * m_Config.shapeParams.boxHalfExtents.x;
                    randomPoint.y = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * m_Config.shapeParams.boxHalfExtents.y;
                    randomPoint.z = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * m_Config.shapeParams.boxHalfExtents.z;
                    return emitterPosition + randomPoint;
                }
                    
                case EmissionShape::Circle: {
                    // Random point in circle (on XZ plane)
                    float angle = static_cast<float>(rand()) / RAND_MAX * glm::radians(m_Config.shapeParams.circleArc);
                    float r = std::sqrt(static_cast<float>(rand()) / RAND_MAX) * m_Config.shapeParams.circleRadius;
                    return emitterPosition + glm::vec3(std::cos(angle) * r, 0.0f, std::sin(angle) * r);
                }
                    
                case EmissionShape::Cone: {
                    // Random point in cone
                    float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
                    float h = static_cast<float>(rand()) / RAND_MAX;
                    float r = h * std::tan(glm::radians(m_Config.shapeParams.coneAngle));
                    return emitterPosition + glm::vec3(std::cos(angle) * r, h, std::sin(angle) * r);
                }
                    
                default:
                    return emitterPosition;
            }
        }
        
        glm::vec3 GetEmissionDirection() {
            // Start with emission direction
            glm::vec3 dir = glm::normalize(m_Config.emissionDirection);
            
            // Apply spread
            if (m_Config.emissionSpread > 0.0f) {
                // Generate random offset within spread angle
                float spreadRad = glm::radians(m_Config.emissionSpread);
                glm::vec3 randomOffset = RandomUnitVector() * static_cast<float>(rand()) / RAND_MAX * spreadRad;
                dir = glm::normalize(dir + randomOffset);
            }
            
            return dir;
        }
        
        glm::vec3 RandomUnitVector() {
            float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159f;
            float phi = std::acos(2.0f * static_cast<float>(rand()) / RAND_MAX - 1.0f);
            
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
                
                // Apply gravity (simple -Y acceleration)
                // Could add more force fields here
                p.velocity.y -= 9.81f * deltaTime; // Simple gravity
                
                // Update size based on lifetime curve
                float normalizedAge = p.age / p.lifetime;
                p.size = EvaluateSizeCurve(normalizedAge) * m_Config.sizeOverLifetimeMultiplier;
                
                // Update color based on lifetime curve
                p.color = EvaluateColorCurve(normalizedAge);
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
