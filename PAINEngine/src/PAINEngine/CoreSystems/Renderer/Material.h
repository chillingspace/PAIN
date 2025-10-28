#pragma once

#include "pch.h"

namespace PAIN {
    struct Material {
		float rough{ 0.1f };        // 0.1 -> smooth, 1 -> rough
        float metal{ 0.3f };
        glm::vec3 color{ 1.f,0.f,0.f };
        bool useTex{ false };        // pass in as float
		unsigned int tex{ 0 };      // just for redundanncy
        // sampler2D tex;
		bool alwaysLit{ false };
    };


}

