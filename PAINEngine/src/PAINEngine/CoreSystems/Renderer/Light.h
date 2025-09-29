#pragma once

#ifdef PN_PLATFORM_WINDOWS
#include "pch.h"

namespace PAIN {

	struct Light {
		glm::vec3 position;
		glm::vec3 L_intensity;

		enum MOVE_MODES {
			FREE,
			ORBIT_ORIGIN,
			NUM_MOVE_MODES,
		};

		MOVE_MODES move_mode = ORBIT_ORIGIN;
	};

}


#endif