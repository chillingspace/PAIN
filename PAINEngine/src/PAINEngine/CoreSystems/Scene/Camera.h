#pragma once

#include <glm/glm.hpp>

namespace PAIN {
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

		// temp
		glm::mat4 model() const;

		glm::mat4 view() const;
		glm::mat4 projection() const;

		void debugPrintFOV() const {
			std::cout << "Vertical FOV: " << fov << " degrees" << std::endl;
			std::cout << "Aspect Ratio: " << aspect_ratio << std::endl;
			float hFOV = 2.0f * glm::degrees(glm::atan(glm::tan(glm::radians(fov) / 2.0f) * aspect_ratio));
			std::cout << "Horizontal FOV: " << hFOV << " degrees" << std::endl;
		}
	};
}