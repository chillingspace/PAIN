#pragma once
#ifdef PN_PLATFORM_ANDROID
#include "pch.h"

#include "Applications/AppSystem.h"
#include "../Shader.h"

namespace PAIN {
    class AndroidRenderer {
    public:
        AndroidRenderer();
        ~AndroidRenderer();

        bool Init(std::shared_ptr<Services> app_services);
        void Render();
        void Cleanup();

        // handle event func?

    private:
        bool createShaders();
        bool createBuffers();

        float clearColor[3];

        unsigned int vao = 0;
        unsigned int vbo = 0;
        unsigned int program = 0;

        std::unique_ptr<Shader> m_shaders;

        std::shared_ptr<Services> services;
    };

}

#endif