#include "pch.h"
#include "LoadingScreen.h"

namespace PAIN {
    namespace Scene {

        // Simple vertex shader for fullscreen quad
        static const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

        // Simple fragment shader for solid colors
        static const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 color;
uniform float alpha;

void main() {
    FragColor = vec4(color, alpha);
}
)";

        void LoadingScreen::init(std::shared_ptr<Window::Window> win) {
            PN_CORE_INFO("[LoadingScreen] Initializing...");

            // Set window ptr
            win_ptr = win;

            // Compile shader
            m_shader = compileShader();

            // Create fullscreen quad for background and progress bar
            float quadVertices[] = {
                // positions   // texCoords
                -1.0f,  1.0f,  0.0f, 1.0f,
                -1.0f, -1.0f,  0.0f, 0.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,

                -1.0f,  1.0f,  0.0f, 1.0f,
                 1.0f, -1.0f,  1.0f, 0.0f,
                 1.0f,  1.0f,  1.0f, 1.0f
            };

            glGenVertexArrays(1, &m_vao);
            glGenBuffers(1, &m_vbo);

            glBindVertexArray(m_vao);
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

            // Position attribute
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
            glEnableVertexAttribArray(0);

            // TexCoord attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
            glEnableVertexAttribArray(1);

            glBindVertexArray(0);

            PN_CORE_INFO("[LoadingScreen] Initialization complete");
        }

        void LoadingScreen::cleanup() {
            PN_CORE_INFO("[LoadingScreen] Cleaning up...");

            if (m_vao) {
                glDeleteVertexArrays(1, &m_vao);
                m_vao = 0;
            }

            if (m_vbo) {
                glDeleteBuffers(1, &m_vbo);
                m_vbo = 0;
            }

            if (m_shader) {
                glDeleteProgram(m_shader);
                m_shader = 0;
            }
        }

        void LoadingScreen::render() {
            // Clear screen with dark background
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);

            // Render progress bar
            renderProgressBar();

            // Render status text (simplified - just log for now)
            renderStatusText();

            // Swap buffers and poll events
            auto win = win_ptr.lock();
            if (win) {
                win->swapBuffers();
                win->pollEvents();
            }
        }

        void LoadingScreen::renderProgressBar() {
            if (!m_shader || !m_vao) return;

            glUseProgram(m_shader);
            glBindVertexArray(m_vao);

            float progress = m_progress.load();

            // Draw background bar (dark gray)
            {
                // Bar dimensions in NDC (-1 to 1)
                float barWidth = 0.6f;   // 60% of screen width
                float barHeight = 0.05f; // 5% of screen height
                float barX = 0.0f;       // Centered
                float barY = -0.3f;      // Below center

                // Create vertices for background bar
                float bgVertices[] = {
                    barX - barWidth / 2, barY + barHeight / 2,  0.0f, 1.0f,
                    barX - barWidth / 2, barY - barHeight / 2,  0.0f, 0.0f,
                    barX + barWidth / 2, barY - barHeight / 2,  1.0f, 0.0f,

                    barX - barWidth / 2, barY + barHeight / 2,  0.0f, 1.0f,
                    barX + barWidth / 2, barY - barHeight / 2,  1.0f, 0.0f,
                    barX + barWidth / 2, barY + barHeight / 2,  1.0f, 1.0f
                };

                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgVertices), bgVertices);

                // Set color uniform (dark gray: 0.2, 0.2, 0.2)
                GLint colorLoc = glGetUniformLocation(m_shader, "color");
                GLint alphaLoc = glGetUniformLocation(m_shader, "alpha");
                glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
                glUniform1f(alphaLoc, 1.0f);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            // Draw progress bar (bright color)
            if (progress > 0.0f) {
                float barWidth = 0.6f * progress; // Scale by progress
                float barHeight = 0.05f;
                float barX = -0.3f + (0.6f * progress) / 2; // Offset to start from left
                float barY = -0.3f;

                float progressVertices[] = {
                    barX - barWidth / 2, barY + barHeight / 2,  0.0f, 1.0f,
                    barX - barWidth / 2, barY - barHeight / 2,  0.0f, 0.0f,
                    barX + barWidth / 2, barY - barHeight / 2,  1.0f, 0.0f,

                    barX - barWidth / 2, barY + barHeight / 2,  0.0f, 1.0f,
                    barX + barWidth / 2, barY - barHeight / 2,  1.0f, 0.0f,
                    barX + barWidth / 2, barY + barHeight / 2,  1.0f, 1.0f
                };

                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(progressVertices), progressVertices);

                // Set color uniform (bright cyan: 0.2, 0.8, 0.9)
                GLint colorLoc = glGetUniformLocation(m_shader, "color");
                GLint alphaLoc = glGetUniformLocation(m_shader, "alpha");
                glUniform3f(colorLoc, 0.2f, 0.8f, 0.9f);
                glUniform1f(alphaLoc, 1.0f);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }

            glBindVertexArray(0);
            glUseProgram(0);
        }

        void LoadingScreen::renderStatusText() {
            // For now, just log the status text
            // In a full implementation, you would render text using your text rendering system
            // TODO: Integrate with existing text rendering system (CoreSystems/Renderer/text.h)
        }

        void LoadingScreen::setProgress(float progress) {
            // Clamp to [0.0, 1.0]
            progress = std::max(0.0f, std::min(1.0f, progress));
            m_progress.store(progress);
        }

        void LoadingScreen::setStatus(const std::string& status) {
            std::lock_guard<std::mutex> lock(m_statusMutex);
            m_statusText = status;
            PN_CORE_INFO("[LoadingScreen] {}", status);
        }

        unsigned int LoadingScreen::compileShader() {
            // Compile vertex shader
            unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
            glCompileShader(vertexShader);

            // Check for vertex shader compile errors
            int success;
            char infoLog[512];
            glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
                PN_CORE_ERROR("[LoadingScreen] Vertex shader compilation failed: {}", infoLog);
            }

            // Compile fragment shader
            unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
            glCompileShader(fragmentShader);

            // Check for fragment shader compile errors
            glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
                PN_CORE_ERROR("[LoadingScreen] Fragment shader compilation failed: {}", infoLog);
            }

            // Link shaders
            unsigned int shaderProgram = glCreateProgram();
            glAttachShader(shaderProgram, vertexShader);
            glAttachShader(shaderProgram, fragmentShader);
            glLinkProgram(shaderProgram);

            // Check for linking errors
            glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
                PN_CORE_ERROR("[LoadingScreen] Shader program linking failed: {}", infoLog);
            }

            // Delete shaders (they're linked into the program now
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            return shaderProgram;
        }

    }
}

