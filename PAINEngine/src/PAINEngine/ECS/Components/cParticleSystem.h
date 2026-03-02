#pragma once

#include "pch.h"
#include "LayeredSystems/LevelEditor/EditorAttributes.h"
#include "AssetTypes.h"

namespace PAIN {

    // ============================================
    // Emission Shape Types
    // ============================================
    enum class EmissionShape {
        Point = 0,
        Sphere,
        Box,
        Circle,
        Cone
    };

    // ============================================
    // Playback State
    // ============================================
    enum class ParticleSystemState {
        Stopped = 0,
        Playing,
        Paused
    };

    enum class ParticleRenderShape {
        Square = 0,
        Circle,
        SoftCircle
    };

    enum class ParticleSimulationSpace {
        World = 0,
        Local
    };

    enum class ParticleBlendMode {
        Alpha = 0,
        Additive,
        Premultiplied
    };

    enum class ParticleSortMode {
        None = 0,
        BackToFront
    };

    enum class ParticleDirectionMode {
        Custom = 0,
        ShapeNormal,
        Random
    };

    enum class ParticleDirectionSpace {
        World = 0,
        Local
    };

    // ============================================
    // Keyframe for curves
    // ============================================
    struct FloatKeyframe {
        float time = 0.0f;      // 0.0 to 1.0
        float value = 1.0f;
        
        // Serialization
        static constexpr bool ShouldSerialize = true;
    };

    struct ColorKeyframe {
        float time = 0.0f;      // 0.0 to 1.0
        glm::vec4 color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        
        // Serialization
        static constexpr bool ShouldSerialize = true;
    };

    // ============================================
    // Shape Parameters
    // ============================================
    struct EmissionShapeParams {
        // Sphere
        float sphereRadius = 0.5f;
        
        // Box
        glm::vec3 boxHalfExtents = glm::vec3(0.5f, 0.5f, 0.5f);
        
        // Circle/Cone
        float circleRadius = 0.5f;
        float circleArc = 360.0f;  // degrees
        
        // Cone
        float coneAngle = 25.0f;   // degrees
        
        // Common
        bool volumeEmission = false;  // Emit from volume vs surface
        
        // Serialization
        static constexpr bool ShouldSerialize = true;
    };

    // ============================================
    // Particle System Component
    // ============================================
    struct ParticleSystemComponent {

        //Serialization flag
        static constexpr bool ShouldSerialize = true;

        // ========================================
        // Main Settings
        // ========================================
        float playDuration = 0.0f;        // 0 = infinite
        float lifetime = 1.0f;           // seconds
        float lifetimeVariance = 0.0f;   // +/- seconds
        bool looping = true;
        bool playOnAwake = true;

        // ========================================
        // Visual Settings
        // ========================================
        Assets::GUID particleTexture;     // Texture asset
        ParticleRenderShape renderShape = ParticleRenderShape::SoftCircle;
        float softEdge = 0.2f;            // Used by SoftCircle shape
        
        float startSize = 1.0f;
        float startSizeVariance = 0.0f;
        
        glm::vec4 startColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);  // RGBA
        glm::vec4 startColorVariance = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        // ========================================
        // Emission Settings
        // ========================================
        float emissionRate = 10.0f;      // particles per second
        int maxParticles = 1000;
        
        float speed = 1.0f;              // units per second
        float speedVariance = 0.0f;
        bool velocityOverLifetimeEnabled = false;
        glm::vec3 velocityOverLifetime = glm::vec3(0.0f);
        float gravityMultiplier = 1.0f;
        float drag = 0.0f;

        float startRotation = 0.0f;      // degrees
        float startRotationVariance = 0.0f;
        float angularVelocity = 0.0f;    // degrees / sec
        float angularVelocityVariance = 0.0f;
        
        EmissionShape emissionShape = EmissionShape::Point;
        EmissionShapeParams shapeParams;
        glm::vec3 emissionDirection = glm::vec3(0.0f, 1.0f, 0.0f);  // Up by default
        float emissionSpread = 0.0f;      // degrees
        ParticleDirectionMode directionMode = ParticleDirectionMode::Custom;
        ParticleDirectionSpace directionSpace = ParticleDirectionSpace::World;

        // ========================================
        // Over Lifetime Curves
        // ========================================
        std::vector<FloatKeyframe> sizeOverLifetime;
        float sizeOverLifetimeMultiplier = 1.0f;
        
        std::vector<ColorKeyframe> colorOverLifetime;

        // ========================================
        // Rendering / Simulation Modules
        // ========================================
        ParticleSimulationSpace simulationSpace = ParticleSimulationSpace::World;
        ParticleBlendMode blendMode = ParticleBlendMode::Alpha;
        ParticleSortMode sortMode = ParticleSortMode::None;
        
        // ========================================
        // Runtime State (DO NOT SERIALIZE)
        // ========================================
        ParticleSystemState state = ParticleSystemState::Stopped;
        float currentPlayTime = 0.0f; // Time since system started playing
        int activeParticleCount = 0;

        // Runtime editor commands (DO NOT SERIALIZE)
        bool requestPlay = false;
        bool requestPause = false;
        bool requestStop = false;
        bool requestRestart = false;

        // Constructors
        ParticleSystemComponent() = default;
    };

} // namespace PAIN

// ============================================
// REFLECTION (Editor Integration)
// ============================================
REFL_TYPE(PAIN::ParticleSystemComponent)
REFL_FIELD(playDuration, PAIN::Editor::Attributes::DisplayName("Duration (s)"), PAIN::Editor::Attributes::Range(0.0f, 60.0f))
REFL_FIELD(lifetime, PAIN::Editor::Attributes::DisplayName("Lifetime (s)"), PAIN::Editor::Attributes::Range(0.1f, 30.0f))
REFL_FIELD(lifetimeVariance, PAIN::Editor::Attributes::DisplayName("Lifetime Variance"))
REFL_FIELD(looping, PAIN::Editor::Attributes::DisplayName("Looping"))
REFL_FIELD(playOnAwake, PAIN::Editor::Attributes::DisplayName("Play On Awake"))
REFL_FIELD(particleTexture, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(renderShape, PAIN::Editor::Attributes::DisplayName("Render Shape"))
REFL_FIELD(softEdge, PAIN::Editor::Attributes::DisplayName("Soft Edge"), PAIN::Editor::Attributes::Range(0.0f, 1.0f))
REFL_FIELD(startSize, PAIN::Editor::Attributes::DisplayName("Start Size"), PAIN::Editor::Attributes::Range(0.01f, 10.0f))
REFL_FIELD(startSizeVariance, PAIN::Editor::Attributes::DisplayName("Size Variance"))
REFL_FIELD(startColor, PAIN::Editor::Attributes::DisplayName("Start Color"))
REFL_FIELD(startColorVariance, PAIN::Editor::Attributes::DisplayName("Color Variance"))
REFL_FIELD(emissionRate, PAIN::Editor::Attributes::DisplayName("Emission Rate"), PAIN::Editor::Attributes::Range(0.0f, 1000.0f))
REFL_FIELD(maxParticles, PAIN::Editor::Attributes::DisplayName("Max Particles"), PAIN::Editor::Attributes::Range(1, 10000))
REFL_FIELD(speed, PAIN::Editor::Attributes::DisplayName("Speed"), PAIN::Editor::Attributes::Range(0.0f, 100.0f))
REFL_FIELD(speedVariance, PAIN::Editor::Attributes::DisplayName("Speed Variance"))
REFL_FIELD(velocityOverLifetimeEnabled, PAIN::Editor::Attributes::DisplayName("Velocity Over Lifetime"))
REFL_FIELD(velocityOverLifetime, PAIN::Editor::Attributes::DisplayName("Velocity Delta"))
REFL_FIELD(gravityMultiplier, PAIN::Editor::Attributes::DisplayName("Gravity Multiplier"), PAIN::Editor::Attributes::Range(-5.0f, 5.0f))
REFL_FIELD(drag, PAIN::Editor::Attributes::DisplayName("Drag"), PAIN::Editor::Attributes::Range(0.0f, 10.0f))
REFL_FIELD(startRotation, PAIN::Editor::Attributes::DisplayName("Start Rotation (deg)"))
REFL_FIELD(startRotationVariance, PAIN::Editor::Attributes::DisplayName("Start Rotation Variance"))
REFL_FIELD(angularVelocity, PAIN::Editor::Attributes::DisplayName("Angular Velocity (deg/s)"))
REFL_FIELD(angularVelocityVariance, PAIN::Editor::Attributes::DisplayName("Angular Velocity Variance"))
REFL_FIELD(emissionShape)
REFL_FIELD(shapeParams)
REFL_FIELD(emissionDirection, PAIN::Editor::Attributes::DisplayName("Emission Direction"))
REFL_FIELD(emissionSpread, PAIN::Editor::Attributes::DisplayName("Spread (deg)"), PAIN::Editor::Attributes::Range(0.0f, 180.0f))
REFL_FIELD(directionMode, PAIN::Editor::Attributes::DisplayName("Direction Mode"))
REFL_FIELD(directionSpace, PAIN::Editor::Attributes::DisplayName("Direction Space"))
REFL_FIELD(sizeOverLifetime, PAIN::Editor::Attributes::DisplayName("Size Over Lifetime"))
REFL_FIELD(sizeOverLifetimeMultiplier, PAIN::Editor::Attributes::DisplayName("Size Multiplier"), PAIN::Editor::Attributes::Range(0.0f, 10.0f))
REFL_FIELD(colorOverLifetime, PAIN::Editor::Attributes::DisplayName("Color Over Lifetime"))
REFL_FIELD(simulationSpace, PAIN::Editor::Attributes::DisplayName("Simulation Space"))
REFL_FIELD(blendMode, PAIN::Editor::Attributes::DisplayName("Blend Mode"))
REFL_FIELD(sortMode, PAIN::Editor::Attributes::DisplayName("Sort Mode"))
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::ParticleSystemComponent>);

// REFLECTION for EmissionShapeParams
REFL_TYPE(PAIN::EmissionShapeParams)
REFL_FIELD(sphereRadius)
REFL_FIELD(boxHalfExtents)
REFL_FIELD(circleRadius)
REFL_FIELD(circleArc)
REFL_FIELD(coneAngle)
REFL_FIELD(volumeEmission)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::EmissionShapeParams>);

// REFLECTION for FloatKeyframe
REFL_TYPE(PAIN::FloatKeyframe)
REFL_FIELD(time)
REFL_FIELD(value)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::FloatKeyframe>);

// REFLECTION for ColorKeyframe
REFL_TYPE(PAIN::ColorKeyframe)
REFL_FIELD(time)
REFL_FIELD(color)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::ColorKeyframe>);
