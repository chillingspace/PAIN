#pragma once

#ifdef PN_PLATFORM_WINDOWS
#include "pch.h"

namespace PAIN {
    struct Material {
        float rough;
        float metal;
        glm::vec3 color;
    };


}




#endif