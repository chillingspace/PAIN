#include "pch.h"
#include "LoadingScreen.h"
#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Assets/sAssets.h"
#include "ECS/Components/cUIComps.h"

#include "CoreSystems/Windows/Window.h"
#include "LayeredSystems/LevelEditor/Editor.h"

namespace PAIN {
    namespace Scene {

        void LoadingScreen::init(std::shared_ptr<Services> serv) {
            PN_CORE_INFO("[LoadingScreen] Initializing...");

            // Set window ptr
            services = serv;

            // Compile shaders
            m_shader = compileShader();
            m_progressBarShader = compileProgressBarShader();
            m_overlayShader = compileOverlayShader();

            //Set default digipen screen for texture rendering
#ifdef PN_PLATFORM_WINDOWS
            std::filesystem::path tex_path = "engine/textures/DigiPen_BLACK.png";
#else
            std::filesystem::path tex_path = "engine\\textures\\DigiPen_BLACK.png";
#endif
            m_backgroundTextureGUID = services.lock()->get<Assets::Manager>()->findGUID(tex_path);

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
            auto last_time = std::chrono::steady_clock::now();

            //Update delta time
            auto now = std::chrono::steady_clock::now();
            float real_dt = std::chrono::duration<float>(now - last_time).count();
            last_time = now;

            // Store Unscaled Time (Always ticking even when paused)
            timing.unscaled_dt = real_dt;
            timing.dt = real_dt;
            
            // Update animation time
           m_animationTime += timing.dt;
            
            // Update animated background frames if enabled
            if (m_animationEnabled && !m_backgroundFrames.empty()) {
                m_currentFrameTime += timing.dt;
                if (m_currentFrameTime >= m_frameTime) {
                    m_currentFrameTime = 0.0f;
                    m_currentFrame = (m_currentFrame + 1) % m_backgroundFrames.size();
                    m_backgroundTextureGUID = m_backgroundFrames[m_currentFrame];
                }
            }
            
            // Clear screen with dark background
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            // Render in Z-order (back to front):
            // Layer 1: Background texture (if set)
            renderBackgroundTexture();
            
            // Layer 2: Animated gradient overlay
            renderBackgroundOverlay();
            
            // Layer 3: Progress bar
            renderProgressBar();

            // Layer 4: Status text
            renderStatusText();

            //Unbind frame buffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            // Swap buffers and poll events
            auto win = services.lock()->get<Window::Window>();
            if (win) {
                win->swapBuffers();
                win->pollEvents();
            }
        }

        void LoadingScreen::finish() {
#ifdef _DEBUG
            auto editor = services.lock()->get<Editor::Editor>();
            bool editor_visible = editor && editor->isVisible();
            int editor_debug_mode = editor ? editor->getDebugMode() : 0;

#else
            bool editor_visible = false;
            int editor_debug_mode = 0;
#endif

            GLenum err = glGetError();
            if (err != GL_NO_ERROR) {
                PN_CORE_ERROR("OpenGL err on update loop begin: {}", err);
            }

            if (editor_visible) {
                glBindFramebuffer(GL_FRAMEBUFFER, services.lock()->get<sRenderer>()->getFinalFbo());
                // glViewport(0, 0, fbWidth, fbHeight);
            }
            else {
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                // Match viewport to window size
                auto window = services.lock()->get<Window::Window>();
                auto frame_buffer = window->getFrameBuffer();
                glViewport(0, 0, frame_buffer.x, frame_buffer.y);
            }

            //Render to buffer
            render();
        }

        void LoadingScreen::renderProgressBar() {
            if (!m_vao) return;
            
            // Use custom shader if available, otherwise fall back to basic shader
            GLuint shaderToUse = m_progressBarShader ? m_progressBarShader : m_shader;
            if (!shaderToUse) return;
            
            glUseProgram(shaderToUse);
            glBindVertexArray(m_vao);
            
            float progress = m_progress.load();
            
            // Set GL state
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);
            
            // If using custom shader, set all uniforms
            if (m_progressBarShader) {
                // Get window dimensions for screen-space to NDC conversion
                auto serv = services.lock();
                if (!serv) return;
                auto win = serv->get<Window::Window>();
                if (!win) return;
                
                auto framebuffer = win->getFrameBuffer();
                float screenWidth = framebuffer.x;
                float screenHeight = framebuffer.y;
                
                GLint progressLoc = glGetUniformLocation(m_progressBarShader, "progress");
                GLint animTimeLoc = glGetUniformLocation(m_progressBarShader, "animationTime");
                GLint fillColorLoc = glGetUniformLocation(m_progressBarShader, "fillColor");
                GLint glowColorLoc = glGetUniformLocation(m_progressBarShader, "glowColor");
                GLint glowIntensityLoc = glGetUniformLocation(m_progressBarShader, "glowIntensity");
                
                glUniform1f(progressLoc, progress);
                glUniform1f(animTimeLoc, m_animationTime);
                glUniform3fv(fillColorLoc, 1, &m_fillColor[0]);
                glUniform3fv(glowColorLoc, 1, &m_glowColor[0]);
                glUniform1f(glowIntensityLoc, m_glowIntensity);
                
                // Calculate position and size in screen space
                float barWidth, barHeight, barX, barY;
                
                if (m_useCustomProgressBarSize) {
                    barWidth = m_progressBarSize.x;
                    barHeight = m_progressBarSize.y;
                } else {
                    // Default: 60% of screen width, 40 pixels height
                    barWidth = screenWidth * 0.6f;
                    barHeight = 40.0f;
                }
                
                if (m_useCustomProgressBarPos) {
                    barX = m_progressBarPosition.x;
                    barY = m_progressBarPosition.y;
                } else {
                    // Default: centered horizontally, 70% down from top
                    barX = screenWidth / 2.0f;
                    barY = screenHeight * 0.7f;
                }
                
                // Convert screen space to NDC
                float ndcX = (barX / screenWidth) * 2.0f - 1.0f;
                float ndcY = 1.0f - (barY / screenHeight) * 2.0f;  // Flip Y (screen Y is top-down, NDC is bottom-up)
                float ndcWidth = (barWidth / screenWidth) * 2.0f;
                float ndcHeight = (barHeight / screenHeight) * 2.0f;
                
                // Create progress bar quad in NDC
                float progressVertices[] = {
                    ndcX - ndcWidth / 2, ndcY + ndcHeight / 2,  0.0f, 1.0f,
                    ndcX - ndcWidth / 2, ndcY - ndcHeight / 2,  0.0f, 0.0f,
                    ndcX + ndcWidth / 2, ndcY - ndcHeight / 2,  1.0f, 0.0f,
                    
                    ndcX - ndcWidth / 2, ndcY + ndcHeight / 2,  0.0f, 1.0f,
                    ndcX + ndcWidth / 2, ndcY - ndcHeight / 2,  1.0f, 0.0f,
                    ndcX + ndcWidth / 2, ndcY + ndcHeight / 2,  1.0f, 1.0f
                };
                
                glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(progressVertices), progressVertices);
                
                // Draw single quad - shader handles empty vs filled portions
                glDrawArrays(GL_TRIANGLES, 0, 6);
            } else {
                // Fallback: Use basic shader with old dual-pass approach
                // Draw background bar (dark gray)
                {
                    float barWidth = 0.6f;
                    float barHeight = 0.05f;
                    float barX = 0.0f;
                    float barY = -0.3f;
                    
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
                    
                    GLint colorLoc = glGetUniformLocation(m_shader, "color");
                    GLint alphaLoc = glGetUniformLocation(m_shader, "alpha");
                    glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
                    glUniform1f(alphaLoc, 1.0f);
                    
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
                
                // Draw progress bar (bright color)
                if (progress > 0.0f) {
                    float barWidth = 0.6f * progress;
                    float barHeight = 0.05f;
                    float barX = -0.3f + (0.6f * progress) / 2;
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
                    
                    GLint colorLoc = glGetUniformLocation(m_shader, "color");
                    GLint alphaLoc = glGetUniformLocation(m_shader, "alpha");
                    glUniform3f(colorLoc, 0.2f, 0.8f, 0.9f);
                    glUniform1f(alphaLoc, 1.0f);
                    
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
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
            std::string currentStatus = "Progress ( ";
            float progress = m_progress.load();
            int percentage = static_cast<int>(progress * 100.0f);
            currentStatus += std::to_string(percentage) + "% ): ";
            {
                std::lock_guard<std::mutex> lock(m_statusMutex);
                currentStatus += m_statusText;
            }
            
            if (currentStatus.empty()) return;
            
            // Create UIText component for status
            UIText statusTextComp;
            statusTextComp.display_text = currentStatus;
            statusTextComp.color = glm::vec3(0.85f, 0.85f, 0.85f);
            statusTextComp.alignment = TextAlignment::Center;
            
            // Calculate position based on custom settings or defaults
            float textX, textY;
            if (m_useCustomStatusTextPos) {
                textX = m_statusTextPosition.x;
                textY = m_statusTextPosition.y;
            } else {
                // Default: centered horizontally, below progress bar
                textX = screenWidth / 2.0f;
                
                // Calculate default progress bar Y position
                float defaultBarY = screenHeight * 0.7f;
                if (m_useCustomProgressBarPos) {
                    defaultBarY = m_progressBarPosition.y;
                }
                
                // Position text below progress bar (add 60 pixels)
                textY = defaultBarY + 60.0f;
            }
            
            statusTextComp.text_pos = glm::vec2(textX, textY);
            statusTextComp.scale_factor = m_statusTextScale;
            
            // Set word wrap and font
            statusTextComp.word_wrap = false;
#ifdef PN_PLATFORM_WINDOWS
            std::filesystem::path font_path = "engine/fonts/OpenSans-Regular.ttf";
#else
            std::filesystem::path font_path = "engine\\fonts\\OpenSans-Regular.ttf";
#endif
            statusTextComp.font_guid = services.lock()->get<Assets::Manager>()->findGUID(font_path);
            
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
            
            #ifdef PN_PLATFORM_WINDOWS
                std::filesystem::path shader_path = "engine/shaders/loading_progress.vert";
            #else
                std::filesystem::path shader_path = "engine\\shaders\\android_loading_progress.vert";
            #endif
            
            auto assets_loader = services.lock()->get<Assets::Manager>();
            auto shader_opt = assets_loader->getAsset<Assets::Shader>(shader_path);
            auto shader = shader_opt.has_value() ? shader_opt.value() : nullptr;
            
            if (!shader || shader->GetRendererID() == 0) {
                PN_CORE_ERROR("[LoadingScreen] Failed to load progress bar shader");
                return 0;
            }
            
            return shader->GetRendererID();
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

        Assets::GUID LoadingScreen::getBackgroundTexture() {
            return m_backgroundTextureGUID;
        }

        void LoadingScreen::renderBackgroundTexture() {
            // Check if background texture is set
            if (!m_backgroundTextureGUID.IsValid()) return;
            
            // Get services
            auto serv = services.lock();
            if (!serv) return;
            
            // Get renderer
            auto renderer = serv->get<sRenderer>();
            if (!renderer || !renderer->w_renderer) return;
            
            // Get window for dimensions
            auto win = serv->get<Window::Window>();
            if (!win) return;
            
            auto framebuffer = win->getFrameBuffer();
            float screenWidth = framebuffer.x;
            float screenHeight = framebuffer.y;
            
            // Get texture asset
            auto assetMgr = serv->get<Assets::Manager>();
            auto texOpt = assetMgr->getAsset<Assets::Texture>(m_backgroundTextureGUID);
            if (!texOpt.has_value()) return;
            if (!texOpt.value()->gl_texture) services.lock()->get<sRenderer>()->uploadTexture(texOpt.value());
            
            GLuint texID = texOpt.value()->gl_texture;
            
            // Render fullscreen background texture
            glm::vec2 pos(0.0f, 0.0f);
            glm::vec2 scale(1, 1);
            glm::vec4 uvTransform(1.0f, 1.0f, 0.0f, 0.0f);  // Full texture
            
            renderer->w_renderer->Render2DTexture(texID, pos, scale, uvTransform);
        }

        void LoadingScreen::renderBackgroundOverlay() {
            // Only render if overlay shader is loaded
            if (!m_overlayShader || !m_vao) return;
            
            glUseProgram(m_overlayShader);
            glBindVertexArray(m_vao);
            
            // Set GL state for overlay (with blending)
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDisable(GL_DEPTH_TEST);
            
            // Set shader uniforms
            GLint animTimeLoc = glGetUniformLocation(m_overlayShader, "animationTime");
            GLint color1Loc = glGetUniformLocation(m_overlayShader, "color1");
            GLint color2Loc = glGetUniformLocation(m_overlayShader, "color2");
            GLint strengthLoc = glGetUniformLocation(m_overlayShader, "overlayStrength");
            
            glUniform1f(animTimeLoc, m_animationTime);
            glUniform3fv(color1Loc, 1, &m_overlayColor1[0]);
            glUniform3fv(color2Loc, 1, &m_overlayColor2[0]);
            glUniform1f(strengthLoc, m_overlayStrength);
            
            // Draw fullscreen quad
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
           glBindVertexArray(0);
            glUseProgram(0);
        }

        // ============================================================
        // Runtime Configuration Implementations
        // ============================================================

        void LoadingScreen::setProgressBarPosition(float x, float y) {
            m_progressBarPosition = glm::vec2(x, y);
            m_useCustomProgressBarPos = true;
        }

        void LoadingScreen::setProgressBarSize(float width, float height) {
            m_progressBarSize = glm::vec2(width, height);
            m_useCustomProgressBarSize = true;
        }

        glm::vec2 LoadingScreen::getProgressBarPosition() const {
            return m_progressBarPosition;
        }

        glm::vec2 LoadingScreen::getProgressBarSize() const {
            return m_progressBarSize;
        }

        void LoadingScreen::setStatusTextPosition(float x, float y) {
            m_statusTextPosition = glm::vec2(x, y);
            m_useCustomStatusTextPos = true;
        }

        void LoadingScreen::setStatusTextScale(float scale) {
            m_statusTextScale = scale;
        }

        glm::vec2 LoadingScreen::getStatusTextPosition() const {
            return m_statusTextPosition;
        }

        float LoadingScreen::getStatusTextScale() const {
            return m_statusTextScale;
        }

        void LoadingScreen::setAnimatedBackground(const std::vector<Assets::GUID>& textureGUIDs, float frameTime) {
            m_backgroundFrames = textureGUIDs;
            m_frameTime = frameTime;
            m_currentFrame = 0;
            m_currentFrameTime = 0.0f;
            
            // Set first frame as background if available
            if (!m_backgroundFrames.empty()) {
                m_backgroundTextureGUID = m_backgroundFrames[0];
                m_animationEnabled = true;
            }
        }

        void LoadingScreen::setBackgroundAnimationEnabled(bool enabled) {
            m_animationEnabled = enabled && !m_backgroundFrames.empty();
        }

    }
}

