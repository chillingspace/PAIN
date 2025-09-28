#pragma once

#ifdef PN_PLATFORM_WINDOWS
#include "pch.h"

namespace PAIN {

	struct Light {
		glm::vec3 position = { 0.f, 0.f, 0.f };
		glm::vec3 L_intensity = { 0.1f, 0.1f, 0.1f };
	};

}


#endif