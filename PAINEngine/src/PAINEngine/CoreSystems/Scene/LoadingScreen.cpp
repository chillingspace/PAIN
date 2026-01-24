#include "pch.h"
#include "LoadingScreen.h"
#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Assets/sAssets.h"
#include "ECS/Components/cUIComps.h"

#include "CoreSystems/Windows/Window.h"

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

        void LoadingScreen::init(std::shared_ptr<Services> serv) {
            PN_CORE_INFO("[LoadingScreen] Initializing...");

            // Set window ptr
            services = serv;

            // Compile shaders
            m_shader = compileShader();
            m_progressBarShader = compileProgressBarShader();
            m_overlayShader = compileOverlayShader();

            // Initialize animation timing
            m_animationTime = 0.0f;
            m_lastFrameTime = std::chrono::steady_clock::now();

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

            if (m_progressBarShader) {
                glDeleteProgram(m_progressBarShader);
                m_progressBarShader = 0;
            }

            if (m_overlayShader) {
                glDeleteProgram(m_overlayShader);
                m_overlayShader = 0;
            }
        }

        void LoadingScreen::render() {
            // Calculate delta time for animations
            auto currentTime = std::chrono::steady_clock::now();
            std::chrono::duration<float> deltaTime = currentTime - m_lastFrameTime;
            m_lastFrameTime = currentTime;
            
            // Update animation time
            m_animationTime += deltaTime.count();
            
            // Clear screen with dark background
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            
            // Render in Z-order (back to front):
            // Layer 1: Background texture (if set)
            renderBackgroundTexture();
            
            // Layer 2: Animated gradient overlay
            renderBackgroundOverlay();
            
            // Layer 3: Progress bar
            renderProgressBar();
            
            // Layer 4: Title text
            renderTitle();
            
            // Layer 5: Status text
            renderStatusText();
            
            // Layer 6: Percentage text
            renderPercentage();

            // Swap buffers and poll events
            auto win = services.lock()->get<Window::Window>();
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
            // Get window dimensions for positioning
            auto win = services.lock()->get<Window::Window>();
            if (!win) return;
            
            auto framebuffer = win->getFrameBuffer();
            float screenWidth = framebuffer.x;
            float screenHeight = framebuffer.y;
            
            // Lock mutex to safely read status text
            std::string currentStatus;
            {
                std::lock_guard<std::mutex> lock(m_statusMutex);
                currentStatus = m_statusText;
            }
            
            if (currentStatus.empty()) return;
            
            // Create UIText component for status
            UIText statusTextComp;
            statusTextComp.display_text = currentStatus;
            statusTextComp.font_size = 20.0f;
            statusTextComp.color = glm::vec3(0.85f, 0.85f, 0.85f);  // Light gray
            statusTextComp.alignment = TextAlignment::Center;
            
            // Position below progress bar
            // Progress bar Y is -0.3 in NDC, convert to screen space
            float progressBarScreenY = screenHeight * (1.0f - m_progressBarY) / 2.0f;
            statusTextComp.text_pos = glm::vec2(screenWidth / 2.0f, progressBarScreenY + 60.0f);
            
            // TODO: Set font GUID - for now TextRenderer will use default font
            // statusTextComp.font_guid = defaultFontGUID;
            
            // Render using TextRenderer static instance
            TextRenderer::get().renderText(statusTextComp);
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

        unsigned int LoadingScreen::compileProgressBarShader() {
            // NOTE: This requires services pointer, which LoadingScreen doesn't currently have
            // For now, return 0 and we'll use the basic shader instead
            // This will be properly implemented when LoadingScreen gets services access
            
            PN_CORE_WARN("[LoadingScreen] Custom progress bar shader not loaded - using basic shader");
            return 0;
            
            /* TODO: Uncomment when services pointer is added to LoadingScreen::init()
            #ifdef PN_PLATFORM_WINDOWS
                std::filesystem::path shader_path = "engine/shaders/loading_progress.vert";
            #else
                std::filesystem::path shader_path = "engine\\shaders\\android_loading_progress.vert";
            #endif
            
            auto assets_loader = services->get<Assets::Manager>();
            auto shader_opt = assets_loader->getAsset<Assets::Shader>(shader_path);
            auto shader = shader_opt.has_value() ? shader_opt.value() : nullptr;
            
            if (!shader || shader->GetRendererID() == 0) {
                PN_CORE_ERROR("[LoadingScreen] Failed to load progress bar shader");
                return 0;
            }
            
            return shader->GetRendererID();
            */
        }

        unsigned int LoadingScreen::compileOverlayShader() {
            // NOTE: Same as above - requires services pointer
            PN_CORE_WARN("[LoadingScreen] Custom overlay shader not loaded - skipping overlay");
            return 0;
            
            #ifdef PN_PLATFORM_WINDOWS
                std::filesystem::path shader_path = "engine/shaders/loading_overlay.vert";
            #else
                std::filesystem::path shader_path = "engine\\shaders\\android_loading_overlay.vert";
            #endif
            
            auto assets_loader = services.lock()->get<Assets::Manager>();
            auto shader_opt = assets_loader->getAsset<Assets::Shader>(shader_path);
            auto shader = shader_opt.has_value() ? shader_opt.value() : nullptr;
            
            if (!shader || shader->GetRendererID() == 0) {
                PN_CORE_WARN("[LoadingScreen] Failed to load overlay shader - skipping");
                return 0;
            }
            
            return shader->GetRendererID();
        }

        void LoadingScreen::setBackgroundTexture(const Assets::GUID& textureGUID) {
            m_backgroundTextureGUID = textureGUID;
        }

        void LoadingScreen::renderBackgroundTexture() {
            // TODO: Implement after renderer_ptr is properly set in Scene.cpp
            // Will use WindowsRenderer->Render2DTexture() for fullscreen background
        }

        void LoadingScreen::renderBackgroundOverlay() {
            // TODO: Implement with overlay shader
            // Renders animated gradient overlay
        }

        void LoadingScreen::renderTitle() {
            // Get window dimensions
            auto win = services.lock()->get<Window::Window>();
            if (!win) return;
            
            auto framebuffer = win->getFrameBuffer();
            float screenWidth = framebuffer.x;
            float screenHeight = framebuffer.y;
            
            // Create UIText component for title
            UIText titleTextComp;
            titleTextComp.display_text = "PAIN Engine";
            titleTextComp.font_size = 48.0f;
            titleTextComp.color = glm::vec3(1.0f, 1.0f, 1.0f);  // Pure white
            titleTextComp.alignment = TextAlignment::Center;
            
            // Position at top of screen
            titleTextComp.text_pos = glm::vec2(screenWidth / 2.0f, 80.0f);
            
            // Optional: Add subtle shadow for depth
            titleTextComp.shadow_offset = glm::vec2(2.0f, 2.0f);
            titleTextComp.shadow_color = glm::vec4(0.0f, 0.0f, 0.0f, 0.5f);
            
            // Render
            TextRenderer::get().renderText(titleTextComp);
        }

        void LoadingScreen::renderPercentage() {
            // Get window dimensions
            auto win = services.lock()->get<Window::Window>();
            if (!win) return;
            
            auto framebuffer = win->getFrameBuffer();
            float screenWidth = framebuffer.x;
            float screenHeight = framebuffer.y;
            
            // Get current progress
            float progress = m_progress.load();
            int percentage = static_cast<int>(progress * 100.0f);
            
            // Create percentage text (e.g., "67%")
            std::string percentageText = std::to_string(percentage) + "%";
            
            // Create UIText component
            UIText percentTextComp;
            percentTextComp.display_text = percentageText;
            percentTextComp.font_size = 32.0f;
            percentTextComp.color = glm::vec3(0.9f, 0.9f, 1.0f);  // Slightly blue-tinted white
            percentTextComp.alignment = TextAlignment::Center;
            
            // Position above progress bar
            float progressBarScreenY = screenHeight * (1.0f - m_progressBarY) / 2.0f;
            percentTextComp.text_pos = glm::vec2(screenWidth / 2.0f, progressBarScreenY - 40.0f);
            
            // Render
            TextRenderer::get().renderText(percentTextComp);
        }

    }
}

