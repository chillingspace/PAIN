#pragma once

#ifndef CAMERA_COLLISION_SYSTEM_HPP
#define CAMERA_COLLISION_SYSTEM_HPP

#include <glm/glm.hpp>
#include <memory>

#include "Camera.h"

namespace PAIN {
    namespace Scene {
        
        class CameraCollisionSystem {
        public:
            CameraCollisionSystem();
            ~CameraCollisionSystem();
            
            // Initialize with physics system reference
            void init(void* physicsSystem);
            
            // Check if proposed position would collide with geometry
            // Returns true if collision detected
            bool wouldCollide(const glm::vec3& position, 
                            const glm::vec3& up,
                            float radius, 
                            float height,
                            float offset,
                            bool useCapsule);
            
            // Get adjusted position with sliding along surfaces
            // Uses currentPos as reference to determine slide direction
            glm::vec3 resolveCollision(const glm::vec3& proposedPos,
                                      const glm::vec3& currentPos,
                                      const glm::vec3& up,
                                      float radius,
                                      float height,
                                      float offset,
                                      bool useCapsule);
            
            // Raycast to find distance to nearest surface in a direction
            // Returns distance or maxDistance if no hit
            float distanceToSurface(const glm::vec3& position,
                                   const glm::vec3& direction,
                                   float maxDistance);
            
            // Check collision for a specific camera (convenience method)
            glm::vec3 checkCameraCollision(Camera* camera, const glm::vec3& proposedPos);
            
            // Raycast-based camera position for third-person camera
            // Casts ray from player to desired position, returns best camera position
            // If blocked: returns hit point - offset (or interpolated closer position)
            // If not blocked: returns desired position
            // smoothFactor: 0 = snap to hit point, 1 = full interpolation
            glm::vec3 getCameraPositionWithRaycast(const glm::vec3& playerPos,
                                                    const glm::vec3& desiredPos,
                                                    float collisionRadius,
                                                    float offset,
                                                    float smoothFactor = 0.0f,
                                                    const glm::vec3& lastValidPos = glm::vec3(0.0f));
            
            // Simple raycast from player to desired camera position
            // Returns distance to hit, or -1 if no hit
            float raycastToPosition(const glm::vec3& from, const glm::vec3& to);
            
        private:
            void* m_physicsSystem;  // JPH::PhysicsSystem*
            bool m_initialized;
            
            // Internal collision shape creation helpers
            void* createSphereShape(float radius);
            void* createCapsuleShape(float radius, float height);
            void destroyShape(void* shape);
            
            // Perform actual collision query
            bool performCollisionQuery(const glm::vec3& position,
                                      const glm::vec3& up,
                                      float radius,
                                      float height,
                                      float offset,
                                      bool useCapsule,
                                      glm::vec3& outNormal,
                                      float& outDepth);
        };
        
    }
}

#endif // CAMERA_COLLISION_SYSTEM_HPP
