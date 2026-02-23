#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>
#include "CoreSystems/Collision/BoundingVolume.h"

namespace PAIN {
	struct Plane {
		glm::vec3 normal = { 0.f, 1.f, 0.f };
		float distance = 0.f;

		Plane() = default;
		Plane(const glm::vec3& p1, const glm::vec3& norm)
			: normal(glm::normalize(norm)), distance(glm::dot(normal, p1)) {}

		float getSignedDistanceToPlane(const glm::vec3& point) const {
			return glm::dot(normal, point) - distance;
		}
	};

	struct Frustum {
		Plane topFace;
		Plane bottomFace;
		Plane rightFace;
		Plane leftFace;
		Plane farFace;
		Plane nearFace;
	};

	// Extracts frustum planes from a view-projection matrix (Gribb/Hart method)
	inline Frustum extractFrustum(const glm::mat4& vp) {
		Frustum frustum;

		// Note: The matrix comes in column-major order from GLM, but the extraction
		// formula uses the row-vectors. vp[col][row]

		// Left Plane
		frustum.leftFace.normal.x = vp[0][3] + vp[0][0];
		frustum.leftFace.normal.y = vp[1][3] + vp[1][0];
		frustum.leftFace.normal.z = vp[2][3] + vp[2][0];
		frustum.leftFace.distance = -(vp[3][3] + vp[3][0]);

		// Right Plane
		frustum.rightFace.normal.x = vp[0][3] - vp[0][0];
		frustum.rightFace.normal.y = vp[1][3] - vp[1][0];
		frustum.rightFace.normal.z = vp[2][3] - vp[2][0];
		frustum.rightFace.distance = -(vp[3][3] - vp[3][0]);

		// Bottom Plane
		frustum.bottomFace.normal.x = vp[0][3] + vp[0][1];
		frustum.bottomFace.normal.y = vp[1][3] + vp[1][1];
		frustum.bottomFace.normal.z = vp[2][3] + vp[2][1];
		frustum.bottomFace.distance = -(vp[3][3] + vp[3][1]);

		// Top Plane
		frustum.topFace.normal.x = vp[0][3] - vp[0][1];
		frustum.topFace.normal.y = vp[1][3] - vp[1][1];
		frustum.topFace.normal.z = vp[2][3] - vp[2][1];
		frustum.topFace.distance = -(vp[3][3] - vp[3][1]);

		// Near Plane
		frustum.nearFace.normal.x = vp[0][3] + vp[0][2];
		frustum.nearFace.normal.y = vp[1][3] + vp[1][2];
		frustum.nearFace.normal.z = vp[2][3] + vp[2][2];
		frustum.nearFace.distance = -(vp[3][3] + vp[3][2]);

		// Far Plane
		frustum.farFace.normal.x = vp[0][3] - vp[0][2];
		frustum.farFace.normal.y = vp[1][3] - vp[1][2];
		frustum.farFace.normal.z = vp[2][3] - vp[2][2];
		frustum.farFace.distance = -(vp[3][3] - vp[3][2]);

		// Normalize planes
		auto normalizePlane = [](Plane& p) {
			float length = glm::length(p.normal);
			p.normal /= length;
			p.distance /= length;
		};

		normalizePlane(frustum.leftFace);
		normalizePlane(frustum.rightFace);
		normalizePlane(frustum.bottomFace);
		normalizePlane(frustum.topFace);
		normalizePlane(frustum.nearFace);
		normalizePlane(frustum.farFace);

		return frustum;
	}

	// Fast intersection test
	inline bool isAABBInFrustum(const AABB& aabb, const Frustum& frustum) {
		const Plane* planes[6] = {
			&frustum.leftFace, &frustum.rightFace,
			&frustum.bottomFace, &frustum.topFace,
			&frustum.nearFace, &frustum.farFace
		};

		glm::vec3 center = aabb.getCenter();
		glm::vec3 extents = aabb.getExtents();

		for (int i = 0; i < 6; ++i) {
			const Plane& plane = *planes[i];

			// Compute the projection interval radius of aabb onto L(t) = aabb.center + t * p.normal
			float r = extents.x * std::abs(plane.normal.x) +
				extents.y * std::abs(plane.normal.y) +
				extents.z * std::abs(plane.normal.z);

			float d = plane.getSignedDistanceToPlane(center);

			if (d < -r) {
				return false; // Outside
			}
		}
		return true; // Inside or intersecting
	}

	class Camera {
	private:
	public:
		Camera(const glm::vec3& pos, const glm::vec3& forward, const glm::vec3& up, float fov, float near_pl, float far_pl, float width_ratio, float height_ratio, float speed = 15.f, float sens = 0.1f) :
			pos(pos), forward(forward), up(up), fov(fov), near_plane(near_pl), far_plane(far_pl), width_ratio(width_ratio), height_ratio(height_ratio), speed(speed), sensitivity(sens) {
			aspect_ratio = width_ratio / height_ratio;

			right = glm::normalize(glm::cross(forward, up));
		};
		~Camera() = default;

		enum MOVE_MODES {
			FPS,
			NUM_MOVE_MODES,
		};

		MOVE_MODES move_mode = FPS;

		glm::vec3 pos;
		glm::vec3 forward;
		glm::vec3 up;
		glm::vec3 right;

		float fov;
		float near_plane;		// closest distance camera can see
		float far_plane;		// furthest distance camera can see

		float width_ratio;
		float height_ratio;
		float aspect_ratio;

		float speed;
		float sensitivity;

		// ============================================
		// Camera Collision Settings
		// ============================================
		bool collisionEnabled = true;           // Toggle collision on/off
		float collisionRadius = 0.5f;           // Sphere/capsule radius
		float collisionOffset = 0.1f;           // Minimum distance from surfaces
		float capsuleHeight = 1.8f;             // Height for capsule collision
		bool useCapsuleCollision = false;       // false = sphere, true = capsule
		bool showCollisionGizmo = false;        // Show collision visualization in editor

		// temp
		glm::mat4 model() const;

		glm::mat4 view() const;
		glm::mat4 projection() const;
		Frustum getFrustum() const;

		void debugPrintFOV() const {
			std::cout << "Vertical FOV: " << fov << " degrees" << std::endl;
			std::cout << "Aspect Ratio: " << aspect_ratio << std::endl;
			float hFOV = 2.0f * glm::degrees(glm::atan(glm::tan(glm::radians(fov) / 2.0f) * aspect_ratio));
			std::cout << "Horizontal FOV: " << hFOV << " degrees" << std::endl;
		}
	};
}