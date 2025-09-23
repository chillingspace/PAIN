#pragma once
#include "pch.h"
#include "../Shader.h"

namespace PAIN {
    class AndroidRenderer {
    public:
        AndroidRenderer();
        ~AndroidRenderer();

        bool Init();
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
    };

}
