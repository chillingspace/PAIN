/*****************************************************************//**
 * \file   CameraCollisionSystem.cpp
 * \brief  Camera collision detection using Jolt Physics
 *
 * \author PAIN Engine
 * \date   2025
 *********************************************************************/

#include "pch.h"
#include "CameraCollisionSystem.h"

// Jolt Physics includes
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyLock.h>

namespace PAIN {
    namespace Scene {

        CameraCollisionSystem::CameraCollisionSystem()
            : m_physicsSystem(nullptr), m_initialized(false) {
        }

        CameraCollisionSystem::~CameraCollisionSystem() {
        }

        void CameraCollisionSystem::init(void* physicsSystem) {
            m_physicsSystem = physicsSystem;
            m_initialized = (physicsSystem != nullptr);
            
            if (m_initialized) {
                PN_CORE_INFO("[CameraCollisionSystem] Initialized with Jolt Physics");
            } else {
                PN_CORE_ERROR("[CameraCollisionSystem] FAILED TO INITIALIZE - Physics system is null!");
            }
        }

        bool CameraCollisionSystem::wouldCollide(const glm::vec3& position,
                                                const glm::vec3& up,
                                                float radius,
                                                float height,
                                                bool useCapsule) {
            if (!m_initialized) return false;
            
            glm::vec3 normal;
            float depth;
            return performCollisionQuery(position, up, radius, height, useCapsule, normal, depth);
        }

        glm::vec3 CameraCollisionSystem::resolveCollision(const glm::vec3& proposedPos,
                                                         const glm::vec3& currentPos,
                                                         const glm::vec3& up,
                                                         float radius,
                                                         float height,
                                                         float offset,
                                                         bool useCapsule) {
            if (!m_initialized) {
                PN_CORE_ERROR("[CameraCollision] resolveCollision called but system not initialized!");
                return proposedPos;
            }
            
            glm::vec3 movementDir = proposedPos - currentPos;
            float movementDist = glm::length(movementDir);
            
            // DEBUG: Log movement attempt
            static int moveCount = 0;
            moveCount++;
            if (moveCount % 60 == 0) {
                PN_CORE_INFO("[CameraCollision] Movement #{} dist={:.3f} from({:.2f}, {:.2f}, {:.2f}) to({:.2f}, {:.2f}, {:.2f})",
                    moveCount, movementDist, 
                    currentPos.x, currentPos.y, currentPos.z,
                    proposedPos.x, proposedPos.y, proposedPos.z);
            }
            
            if (movementDist < 0.0001f) {
                // No movement, just check if current position is valid
                glm::vec3 normal;
                float depth;
                if (performCollisionQuery(currentPos, up, radius, height, useCapsule, normal, depth)) {
                    // Push out of collision
                    PN_CORE_WARN("[CameraCollision] Currently inside geometry! Pushing out by {:.3f}", depth + offset);
                    return currentPos + normal * (depth + offset);
                }
                return currentPos;
            }
            
            movementDir = movementDir / movementDist; // Normalize
            
            // First try: Check if proposed position is clear
            glm::vec3 normal;
            float depth;
            if (!performCollisionQuery(proposedPos, up, radius, height, useCapsule, normal, depth)) {
                // No collision, can move freely
                return proposedPos;
            }
            
            // Collision detected - calculate slide vector
            PN_CORE_INFO("[CameraCollision] Collision at proposed pos, attempting to slide...");
            
            // Project movement onto collision normal plane
            float normalDotMovement = glm::dot(normal, movementDir);
            if (normalDotMovement > 0.0f) {
                // Moving into surface, need to slide
                glm::vec3 slideDir = movementDir - normal * normalDotMovement;
                float slideDist = movementDist * glm::length(slideDir);
                
                if (slideDist > 0.0001f) {
                    slideDir = glm::normalize(slideDir);
                    glm::vec3 slidePos = currentPos + slideDir * slideDist;
                    
                    // Check if slide position is clear
                    if (!performCollisionQuery(slidePos, up, radius, height, useCapsule, normal, depth)) {
                        PN_CORE_INFO("[CameraCollision] Sliding successful!");
                        return slidePos;
                    }
                }
            }
            
            // Try intermediate positions along the path
            PN_CORE_INFO("[CameraCollision] Slide failed, trying intermediate positions...");
            const int steps = 5;
            for (int i = steps - 1; i >= 0; --i) {
                float t = static_cast<float>(i) / static_cast<float>(steps);
                glm::vec3 testPos = currentPos + movementDir * (movementDist * t);
                
                if (!performCollisionQuery(testPos, up, radius, height, useCapsule, normal, depth)) {
                    PN_CORE_INFO("[CameraCollision] Found valid position at step {}/{} (t={:.2f})", i, steps, t);
                    return testPos;
                }
            }
            
            // All positions blocked, stay at current position but push out if inside geometry
            PN_CORE_WARN("[CameraCollision] All positions blocked! Staying at current position.");
            if (performCollisionQuery(currentPos, up, radius, height, useCapsule, normal, depth)) {
                return currentPos + normal * (depth + offset);
            }
            
            return currentPos;
        }

        float CameraCollisionSystem::distanceToSurface(const glm::vec3& position,
                                                      const glm::vec3& direction,
                                                      float maxDistance) {
            if (!m_initialized) return maxDistance;
            
            JPH::PhysicsSystem* physics = static_cast<JPH::PhysicsSystem*>(m_physicsSystem);
            
            JPH::RRayCast ray;
            ray.mOrigin = JPH::RVec3(position.x, position.y, position.z);
            ray.mDirection = JPH::Vec3(direction.x, direction.y, direction.z) * maxDistance;
            
            JPH::RayCastResult result;
            if (physics->GetNarrowPhaseQuery().CastRay(ray, result)) {
                return result.mFraction * maxDistance;
            }
            
            return maxDistance;
        }

        glm::vec3 CameraCollisionSystem::checkCameraCollision(Camera* camera, const glm::vec3& proposedPos) {
            if (!camera || !camera->collisionEnabled) return proposedPos;
            
            return resolveCollision(
                proposedPos,
                camera->pos,
                camera->up,
                camera->collisionRadius,
                camera->capsuleHeight,
                camera->collisionOffset,
                camera->useCapsuleCollision
            );
        }

        bool CameraCollisionSystem::performCollisionQuery(const glm::vec3& position,
                                                         const glm::vec3& up,
                                                         float radius,
                                                         float height,
                                                         bool useCapsule,
                                                         glm::vec3& outNormal,
                                                         float& outDepth) {
            JPH::PhysicsSystem* physics = static_cast<JPH::PhysicsSystem*>(m_physicsSystem);
            
            // DEBUG: Log query attempt
            static int queryCount = 0;
            queryCount++;
            if (queryCount % 60 == 0) { // Log every 60 queries (about once per second at 60fps)
                PN_CORE_INFO("[CameraCollision] Query #{} at pos({:.2f}, {:.2f}, {:.2f}) radius={:.2f}", 
                    queryCount, position.x, position.y, position.z, radius);
            }
            
            // Create collision shape settings
            JPH::Ref<JPH::Shape> shape;
            JPH::Vec3 shapeOffset(0, 0, 0);
            
            if (useCapsule) {
                // Capsule: half height of the cylinder part (excluding hemispheres)
                float halfHeight = (height - 2.0f * radius) * 0.5f;
                if (halfHeight < 0.0f) halfHeight = 0.0f;
                shape = new JPH::CapsuleShape(halfHeight, radius);
                // Offset so bottom of capsule is at position.y
                shapeOffset = JPH::Vec3(0, height * 0.5f, 0);
            } else {
                // Sphere
                shape = new JPH::SphereShape(radius);
            }
            
            // Create shape cast
            JPH::Vec3 joltPos(position.x, position.y, position.z);
            JPH::Quat joltRot = JPH::Quat::sIdentity();
            
            // Collision query settings
            JPH::CollideShapeSettings settings;
            settings.mMaxSeparationDistance = 0.0f;
            settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
            
            // Perform collision check
            class CollisionCollector : public JPH::CollideShapeCollector {
            public:
                bool hasCollision = false;
                JPH::Vec3 deepestNormal;
                float deepestDepth = 0.0f;
                int hitCount = 0;
                
                void AddHit(const JPH::CollideShapeResult& result) override {
                    hasCollision = true;
                    hitCount++;
                    if (result.mPenetrationDepth > deepestDepth) {
                        deepestDepth = result.mPenetrationDepth;
                        deepestNormal = result.mPenetrationAxis;
                    }
                }
            };
            
            CollisionCollector collector;
            
            // Query collision against all bodies
            physics->GetNarrowPhaseQuery().CollideShape(
                shape,
                JPH::Vec3::sReplicate(1.0f), // scale
                JPH::RMat44::sTranslation(joltPos + shapeOffset),
                settings,
                joltPos + shapeOffset, // base offset
                collector
            );
            
            if (collector.hasCollision) {
                outNormal = glm::vec3(collector.deepestNormal.GetX(), 
                                     collector.deepestNormal.GetY(), 
                                     collector.deepestNormal.GetZ());
                outDepth = collector.deepestDepth;
                
                // DEBUG: Log collision detection
                PN_CORE_WARN("[CameraCollision] COLLISION DETECTED! Hits: {} Depth: {:.3f} Normal: ({:.2f}, {:.2f}, {:.2f})",
                    collector.hitCount, outDepth, outNormal.x, outNormal.y, outNormal.z);
                return true;
            }
            
            return false;
        }

    }
}
