#pragma once
#include "pch.h"

namespace PAIN {
	class Camera {
	private:
	public:
		Camera(const glm::vec3& pos, const glm::vec3& forward, const glm::vec3& up, float fov, float near_pl, float far_pl, float width_ratio, float height_ratio) :
			pos(pos), forward(forward), up(up), fov(fov), near_plane(near_pl), far_plane(far_pl), width_ratio(width_ratio), height_ratio(height_ratio) {
			aspect_ratio = width_ratio / height_ratio;
		};
		~Camera() = default;

		enum MOVE_MODES {
			FPS,
			ORBIT_ORIGIN,
			NUM_MOVE_MODES,
		};

		MOVE_MODES move_mode = FPS;

		float speed = 15.f;
		float sensitivity = 0.1f;

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

		// temp
		glm::mat4 model() const;

		glm::mat4 view() const;
		glm::mat4 projection() const;

	};
}