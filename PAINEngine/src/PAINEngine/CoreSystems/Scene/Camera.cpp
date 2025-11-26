#include "pch.h"
#include "Camera.h"

namespace PAIN {

	glm::mat4 Camera::model() const
	{
		glm::mat4 m = glm::mat4(1.f);
		m = glm::translate(m, glm::vec3(0.f, 1.f, 0.f));
		return m;
	}

	glm::mat4 Camera::view() const {
		return glm::lookAt(pos, pos + forward, up);
	}

	glm::mat4 Camera::projection() const
	{
		return glm::perspective(glm::radians(fov), aspect_ratio, near_plane, far_plane);
	}


}