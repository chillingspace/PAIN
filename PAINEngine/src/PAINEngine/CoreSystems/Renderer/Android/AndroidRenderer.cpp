#ifdef PN_PLATFORM_ANDROID

#include "AndroidRenderer.h"

#include <cstring>

namespace PAIN {

    AndroidRenderer::AndroidRenderer() {
        clearColor[0] = 0.5f;
        clearColor[1] = 0.5f;
        clearColor[2] = 0.5f;
    }

    AndroidRenderer::~AndroidRenderer() {
        Cleanup();
    }

    bool AndroidRenderer::Init() {
        if (!createShaders()) {
            PN_CORE_ERROR("Failed to create shaders");
            return false;
        }

        if (!createBuffers()) {
            PN_CORE_ERROR("Failed to create buffers");
            return false;
        }

        return true;
    }

    bool AndroidRenderer::createShaders() {
        // Simple vertex and fragment shader code
        const std::string vertexSrc = R"(#version 300 es
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        out vec3 ourColor;
        void main() {
            gl_Position = vec4(aPos, 1.0);
            ourColor = aColor;
        }
    )";

        const std::string fragmentSrc = R"(#version 300 es
        precision mediump float;
        in vec3 ourColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(ourColor, 1.0);
        }
    )";

        m_shaders = std::make_unique<Shader>(vertexSrc, fragmentSrc);

        if (!m_shaders) {
            return false;
        }

        program = m_shaders->GetRendererID();
        return true;
    }

    bool AndroidRenderer::createBuffers() {
        float vertices[] = {
             0.0f,  0.5f, 0.0f,     1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.0f,     0.0f, 1.0f, 0.0f,
             0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f
        };

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // color
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return true;
    }

    void AndroidRenderer::Render() {
        // Clear color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear depth and color
        glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.0f);

        // Use Shader Program
        glUseProgram(program);

        const char* version = (const char*)glGetString(GL_VERSION);
        bool useVAO = (version && strstr(version, "OpenGL ES 3"));

        if (useVAO) {
            glBindVertexArray(vao);
        } else {
            // For OpenGL ES 2.0, manually bind attributes
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); // position
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); // color
            glEnableVertexAttribArray(1);
        }

        // Draw triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);


    }

    void AndroidRenderer::Cleanup() {
        if (vbo != 0) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (vao != 0) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        if (program != 0) {
            glDeleteProgram(program);
            program = 0;
        }
    }

}

#endif