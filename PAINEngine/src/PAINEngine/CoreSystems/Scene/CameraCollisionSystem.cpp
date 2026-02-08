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
            m_initialized = (static_cast<JPH::PhysicsSystem*>(m_physicsSystem) != nullptr);
            
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
            bool collides = performCollisionQuery(position, up, radius, height, useCapsule, normal, depth);
            
            // DEBUG: Log when no collision detected
            static int noHitCount = 0;
            if (!collides) {
                noHitCount++;
                if (noHitCount % 120 == 0) { // Every ~2 seconds
                    JPH::PhysicsSystem* physics = static_cast<JPH::PhysicsSystem*>(m_physicsSystem);
                    uint32_t bodyCount = physics->GetNumBodies();
                    PN_CORE_INFO("[CameraCollision] No collision detected. Physics bodies in world: {}", bodyCount);
                }
            }
            
            return collides;
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
            // Ensure depth is positive - negative depth means we're within separation distance but not overlapping
            float penetrationDepth = glm::max(depth, 0.0f);
            
            PN_CORE_INFO("[CameraCollision] Collision at proposed pos (raw depth: {:.3f}, penetration: {:.3f}), attempting to slide...", 
                depth, penetrationDepth);
            
            // The normal from Jolt points from the collided body toward our camera shape
            // This is the direction we need to move to resolve the collision
            // No need to flip - Jolt's mPenetrationAxis already points from shape1 (world) to shape2 (camera)
            
            // Calculate how much we need to push back
            float pushBackDist = penetrationDepth + offset;
            
            // First try: Push back from proposed position along normal
            glm::vec3 pushedBackPos = proposedPos + normal * pushBackDist;
            
            glm::vec3 pushNormal;
            float pushDepth;
            if (!performCollisionQuery(pushedBackPos, up, radius, height, useCapsule, pushNormal, pushDepth)) {
                PN_CORE_INFO("[CameraCollision] Push back successful! Moved by {:.3f}", pushBackDist);
                return pushedBackPos;
            }
            
            // Second try: Project movement onto collision normal plane (sliding)
            float normalDotMovement = glm::dot(normal, movementDir);
            
            // Only slide if we're actually moving toward the surface
            if (normalDotMovement > 0.0f) {
                glm::vec3 slideDir = movementDir - normal * normalDotMovement;
                float slideDirLen = glm::length(slideDir);
                
                if (slideDirLen > 0.0001f) {
                    slideDir = slideDir / slideDirLen;
                    // Use full movement distance but push out by normal first
                    glm::vec3 slidePos = currentPos + slideDir * movementDist + normal * (offset + 0.05f);
                    
                    glm::vec3 slideNormal;
                    float slideDepth;
                    if (!performCollisionQuery(slidePos, up, radius, height, useCapsule, slideNormal, slideDepth)) {
                        PN_CORE_INFO("[CameraCollision] Sliding successful!");
                        return slidePos;
                    }
                }
            }
            
            // Third try: Binary search for valid position along movement path
            PN_CORE_INFO("[CameraCollision] Slide failed, trying binary search...");
            float tMin = 0.0f;
            float tMax = 1.0f;
            glm::vec3 bestPos = currentPos;
            
            for (int i = 0; i < 6; ++i) { // 6 iterations = ~1.5% precision
                float t = (tMin + tMax) * 0.5f;
                glm::vec3 testPos = currentPos + movementDir * (movementDist * t);
                
                // Always push out slightly by normal
                testPos += normal * offset;
                
                glm::vec3 testNormal;
                float testDepth;
                if (performCollisionQuery(testPos, up, radius, height, useCapsule, testNormal, testDepth)) {
                    // Collision, reduce max
                    tMax = t;
                } else {
                    // No collision, can move here, increase min
                    tMin = t;
                    bestPos = testPos;
                }
            }
            
            if (tMin > 0.01f) {
                PN_CORE_INFO("[CameraCollision] Binary search found valid position at t={:.2f}", tMin);
                return bestPos;
            }
            
            // All positions blocked, stay at current position
            PN_CORE_WARN("[CameraCollision] All positions blocked! Staying at current position.");
            
            // If currently in collision, push out
            glm::vec3 currentNormal;
            float currentDepth;
            if (performCollisionQuery(currentPos, up, radius, height, useCapsule, currentNormal, currentDepth)) {
                float currentPenetration = glm::max(currentDepth, 0.0f);
                if (currentPenetration > 0.0f) {
                    PN_CORE_WARN("[CameraCollision] Pushing out of geometry by {:.3f}", currentPenetration + offset);
                    return currentPos + currentNormal * (currentPenetration + offset);
                }
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
                PN_CORE_INFO("[CameraCollision] Query #{} at pos({:.2f}, {:.2f}, {:.2f}) radius={:.2f} useCapsule={}", 
                    queryCount, position.x, position.y, position.z, radius, useCapsule);
            }
            
            // Create collision shape using proper Jolt API
            JPH::Ref<JPH::Shape> shape;
            
            if (useCapsule) {
                // Capsule: half height of the cylinder part (excluding hemispheres)
                float halfHeight = (height - 2.0f * radius) * 0.5f;
                if (halfHeight < 0.0f) halfHeight = 0.0f;
                
                JPH::CapsuleShapeSettings capsuleSettings(halfHeight, radius);
                JPH::ShapeSettings::ShapeResult result = capsuleSettings.Create();
                if (result.IsValid()) {
                    shape = result.Get();
                } else {
                    PN_CORE_ERROR("[CameraCollision] Failed to create capsule shape!");
                    return false;
                }
            } else {
                // Sphere - camera is at center
                JPH::SphereShapeSettings sphereSettings(radius);
                JPH::ShapeSettings::ShapeResult result = sphereSettings.Create();
                if (result.IsValid()) {
                    shape = result.Get();
                } else {
                    PN_CORE_ERROR("[CameraCollision] Failed to create sphere shape!");
                    return false;
                }
            }
            
            if (!shape) {
                PN_CORE_ERROR("[CameraCollision] Shape is null after creation!");
                return false;
            }
            
            // Create transform for the shape at camera position
            // Use RMat44 (real/double precision) to match Jolt's CollideShape API
            JPH::RMat44 transform = JPH::RMat44::sTranslation(JPH::RVec3(position.x, position.y, position.z));
            
            // Collision query settings
            JPH::CollideShapeSettings settings;
            // IMPORTANT: Set separation distance to 0 to only detect actual overlaps
            // Non-zero values cause collisions to be reported before shapes actually touch
            settings.mMaxSeparationDistance = 0.0f;
            settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;
            // Enable active edge detection for better normal calculation  
            settings.mActiveEdgeMode = JPH::EActiveEdgeMode::CollideOnlyWithActive;
            
            // Perform collision check
            class CollisionCollector : public JPH::CollideShapeCollector {
            public:
                bool hasCollision = false;
                JPH::Vec3 deepestNormal;
                float deepestDepth = 0.0f;
                int hitCount = 0;
                JPH::BodyID hitBodyID;
                
                void AddHit(const JPH::CollideShapeResult& result) override {
                    hasCollision = true;
                    hitCount++;
                    // mPenetrationDepth is positive when shapes overlap
                    if (result.mPenetrationDepth > deepestDepth) {
                        deepestDepth = result.mPenetrationDepth;
                        // mPenetrationAxis points from shape 2 to shape 1 (from world geometry to our shape)
                        deepestNormal = result.mPenetrationAxis;
                        hitBodyID = result.mBodyID2; // Store the body we hit
                        
                        // DEBUG: Log collision details with body ID
                        PN_CORE_ERROR("[CameraCollision] HIT BodyID={}: Depth={:.3f} Normal=({:.2f}, {:.2f}, {:.2f}) Contact1=({:.2f}, {:.2f}, {:.2f})",
                            result.mBodyID2.GetIndex(),
                            result.mPenetrationDepth,
                            result.mPenetrationAxis.GetX(), result.mPenetrationAxis.GetY(), result.mPenetrationAxis.GetZ(),
                            result.mContactPointOn1.GetX(), result.mContactPointOn1.GetY(), result.mContactPointOn1.GetZ());
                    }
                }
            };
            
            CollisionCollector collector;
            
            // DEBUG: Log layer filter info
            static int layerCheckCount = 0;
            if (layerCheckCount % 60 == 0) {
                // Log all bodies and their layers
                JPH::BodyIDVector bodyIDs;
                physics->GetBodies(bodyIDs);
                PN_CORE_INFO("[CameraCollision] Total bodies in world: {}", bodyIDs.size());
                for (const auto& id : bodyIDs) {
                    auto& lockInterface = physics->GetBodyLockInterface();
                    JPH::BodyLockRead lock(lockInterface, id);
                    if (lock.Succeeded()) {
                        const JPH::Body& body = lock.GetBody();
                        JPH::ObjectLayer layer = body.GetObjectLayer();
                        JPH::RVec3 pos = body.GetPosition();
                        PN_CORE_INFO("[CameraCollision] Body {}: Layer={}, Pos=({:.2f}, {:.2f}, {:.2f})", 
                            id.GetIndex(), layer, (float)pos.GetX(), (float)pos.GetY(), (float)pos.GetZ());
                    }
                }
            }
            layerCheckCount++;
            
            // Create layer filters to collide with NON_MOVING (static) and MOVING (dynamic) objects
            // This matches the layer setup in your physics system
            class CameraCollisionLayerFilter : public JPH::BroadPhaseLayerFilter {
            public:
                virtual bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override {
                    return true; // Allow all broadphase layers for camera collision
                }
            };
            
            class CameraObjectLayerFilter : public JPH::ObjectLayerFilter {
            public:
                virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override {
                    // DEBUG: Log what layers are being checked
                    static int filterCount = 0;
                    if (filterCount % 300 == 0) {
                        PN_CORE_INFO("[CameraCollision] ObjectLayerFilter checking layer: {}", inLayer);
                    }
                    filterCount++;
                    // Collide with all object layers except UNUSED ones
                    // Layer::NON_MOVING = 4, Layer::MOVING = 5, Layer::DEBRIS = 6, Layer::SENSOR = 7
                    return inLayer >= 4 && inLayer <= 7;
                }
            };
            
            CameraCollisionLayerFilter bpFilter;
            CameraObjectLayerFilter objFilter;
            
            // DEBUG: Critical position logging
            PN_CORE_ERROR("[CameraCollision] QUERY: Pos({:.3f}, {:.3f}, {:.3f}) Radius={:.3f}", 
                position.x, position.y, position.z, radius);
            
            // DEBUG: Log all bodies in physics world (once per second)
            static int bodyLogCounter = 0;
            if (bodyLogCounter % 60 == 0) {
                JPH::BodyIDVector allBodies;
                physics->GetBodies(allBodies);
                PN_CORE_INFO("[CameraCollision] Total bodies in world: {}", allBodies.size());
                for (const auto& bodyID : allBodies) {
                    JPH::BodyLockRead lock(physics->GetBodyLockInterface(), bodyID);
                    if (lock.Succeeded()) {
                        const JPH::Body& body = lock.GetBody();
                        JPH::ObjectLayer layer = body.GetObjectLayer();
                        JPH::RVec3 pos = body.GetPosition();
                        PN_CORE_INFO("[CameraCollision]   Body {}: Layer={} Pos=({:.2f}, {:.2f}, {:.2f}) Type={}",
                            bodyID.GetIndex(), layer, 
                            (float)pos.GetX(), (float)pos.GetY(), (float)pos.GetZ(),
                            body.GetMotionType() == JPH::EMotionType::Static ? "Static" : "Dynamic");
                    }
                }
            }
            bodyLogCounter++;
            
            // Create a body filter to exclude specific bodies if needed
            class CameraBodyFilter : public JPH::BodyFilter {
            public:
                virtual bool ShouldCollideLocked(const JPH::Body& inBody) const override {
                    // Allow collision with all bodies for now
                    // You can add body ID exclusions here if needed
                    return true;
                }
                
                virtual bool ShouldCollide(const JPH::BodyID &inBodyID) const override {
                    return true;
                }
            };
            
            CameraBodyFilter bodyFilter;
            
            // Query collision against all bodies with proper layer filters
            // Pass references (not pointers) to the filters
            // Use RVec3 for base offset to match transform precision
            physics->GetNarrowPhaseQuery().CollideShape(
                shape,
                JPH::Vec3::sReplicate(1.0f), // scale - uniform scaling
                transform,
                settings,
                JPH::RVec3(0, 0, 0), // base offset - centered at camera position
                collector,
                bpFilter, // broad phase layer filter (reference)
                objFilter, // object layer filter (reference)
                bodyFilter // body filter (reference)
            );
            
            if (collector.hasCollision) {
                // Ensure normal is normalized and points in correct direction
                JPH::Vec3 n = collector.deepestNormal;
                float len = n.Length();
                if (len > 0.0001f) {
                    n = n / len;
                } else {
                    // Default normal pointing up if we can't determine direction
                    n = JPH::Vec3(0, 1, 0);
                }
                
                outNormal = glm::vec3(n.GetX(), n.GetY(), n.GetZ());
                outDepth = collector.deepestDepth;
                
                // DEBUG: Log collision detection with more detail
                PN_CORE_WARN("[CameraCollision] COLLISION DETECTED! Hits: {} Depth: {:.3f} Normal: ({:.2f}, {:.2f}, {:.2f}) at Pos({:.2f}, {:.2f}, {:.2f})",
                    collector.hitCount, outDepth, outNormal.x, outNormal.y, outNormal.z, position.x, position.y, position.z);
                
                // Additional debug: if depth is negative, we're within separation distance but not actually colliding
                if (outDepth < 0.0f) {
                    PN_CORE_INFO("[CameraCollision] Note: Negative depth ({:.3f}) means within separation distance but not overlapping", outDepth);
                }
                
                return true;
            }
            
            // DEBUG: Log occasional "no collision" at higher frequency for debugging
            static int noCollisionCount = 0;
            noCollisionCount++;
            if (noCollisionCount % 300 == 0) { // Every 5 seconds at 60fps
                PN_CORE_INFO("[CameraCollision] No collision at Pos({:.2f}, {:.2f}, {:.2f}) Radius={:.2f}",
                    position.x, position.y, position.z, radius);
            }
            
            return false;
        }

    }
}
