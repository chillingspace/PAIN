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

            // Initialize default position and size for progress bar and status text
            defaultSetup();

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
            //Update delta time using member variable to track time between frames
            auto now = std::chrono::steady_clock::now();
            float real_dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
            m_lastFrameTime = now;

            // Store Unscaled Time (Always ticking even when paused)
            timing.unscaled_dt = real_dt;
            timing.dt = real_dt;
            
            // Update animation time
           m_animationTime += timing.dt;
            
            // Update spritesheet animation frame if enabled
            if (m_animationEnabled && m_frameCount > 1) {
                m_currentFrameTime += timing.dt;
                if (m_currentFrameTime >= m_frameTime) {
                    m_currentFrameTime = 0.0f;
                    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_frameCount;
                }
            }
            
            // Clear screen with dark background
            glClearColor(m_backGroundColor.r, m_backGroundColor.g, m_backGroundColor.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            
            // Render in Z-order (back to front):
            // Layer 1: Background texture (if set)
            if (showBg) renderBackgroundTexture();

            // Layer 2: Animated gradient overlay
            if (showOverlay) renderBackgroundOverlay();

            // Layer 3: Progress bar
            if (showProgress) renderProgressBar();

            // Layer 4: Status text
            if (showStatus) renderStatusText();

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
                
                // Vertices are pre-built by buildProgressBarVertices()
                //Just draw from the buffer
                glDrawArrays(GL_TRIANGLES, 0, 6);
            } else {
                // Get window dimensions for screen-space to NDC conversion
                auto serv = services.lock();
                if (!serv) return;
                auto win = serv->get<Window::Window>();
                if (!win) return;

                auto framebuffer = win->getFrameBuffer();
                float screenWidth = framebuffer.x;
                float screenHeight = framebuffer.y;

                // Fallback: Use basic shader with old dual-pass approach
                // Draw background bar (dark gray)
                {                  
                    // Calculate position and size in screen space
                    float barWidth = m_progressBarSize.x;
                    float barHeight = m_progressBarSize.y;

                    float barX = m_progressBarPosition.x;
                    float barY = m_progressBarPosition.y;

                    // Convert screen space to NDC
                    float ndcX = (barX / screenWidth) * 2.0f - 1.0f;
                    float ndcY = (barY / screenHeight) * 2.0f - 1.0f;
                    float ndcWidth = (barWidth / screenWidth) * 2.0f;
                    float ndcHeight = (barHeight / screenHeight) * 2.0f;

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

                    GLint colorLoc = glGetUniformLocation(m_shader, "color");
                    GLint alphaLoc = glGetUniformLocation(m_shader, "alpha");
                    glUniform3f(colorLoc, 0.2f, 0.2f, 0.2f);
                    glUniform1f(alphaLoc, 1.0f);
                    
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }

                // Draw progress bar (bright color)
                if (progress > 0.0f) {
                    float barWidth = m_progressBarSize.x * progress;
                    float barHeight = m_progressBarSize.y;
                    float barX = m_progressBarPosition.x;
                    float barY = m_progressBarPosition.y;

                    // Convert screen space to NDC
                    float ndcX = (barX / screenWidth) * 2.0f - 1.0f;
                    float ndcY = (barY / screenHeight) * 2.0f - 1.0f;
                    float ndcWidth = (barWidth / screenWidth) * 2.0f;
                    float ndcHeight = (barHeight / screenHeight) * 2.0f;

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
            float textX = m_statusTextPosition.x;
            float textY = m_statusTextPosition.y;

            
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

        void LoadingScreen::setBackgroundColor(glm::vec3 const& bg_color) {
            m_backGroundColor = bg_color;
        }

        glm::vec3 LoadingScreen::getBackgroundColor() {
            return m_backGroundColor;
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
            
            // Calculate UV coordinates for spritesheet animation
            glm::vec4 uvTransform;
            
            if (m_animationEnabled && m_frameCount > 1) {
                // Calculate UV coordinates for current frame in spritesheet
                float frameWidth = 1.0f / static_cast<float>(m_framesPerRow);
                int framesPerColumn = (m_frameCount + m_framesPerRow - 1) / m_framesPerRow;  // Ceil division
                float frameHeight = 1.0f / static_cast<float>(framesPerColumn);
                
                // Calculate current frame's row and column
                int frameRow = m_currentFrameIndex / m_framesPerRow;
                int frameCol = m_currentFrameIndex % m_framesPerRow;
                
                // UV transform: (scaleU, scaleV, offsetU, offsetV)
                uvTransform = glm::vec4(
                    frameWidth,                              // Scale U
                    frameHeight,                             // Scale V
                    frameCol * frameWidth,                   // Offset U
                    frameRow * frameHeight                   // Offset V
                );
            } else {
                // No animation: use full texture
                uvTransform = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
            }
            
            // Render fullscreen background texture with UV transform
            // Use normalized scale (1,1) for fullscreen, renderer handles actual screen dimensions
            glm::vec2 pos(0.0f, 0.0f);
            glm::vec2 scale(texOpt.value()->width, texOpt.value()->height);
            glm::vec2 normscale = glm::normalize(scale) * bgScale;
            
            renderer->w_renderer->Render2DTexture(texID, pos, normscale, uvTransform);
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
            buildProgressBarVertices();
        }

        void LoadingScreen::setProgressBarSize(float width, float height) {
            m_progressBarSize = glm::vec2(width, height);
            buildProgressBarVertices();
        }

        glm::vec2 LoadingScreen::getProgressBarPosition() const {
            return m_progressBarPosition;
        }

        glm::vec2 LoadingScreen::getProgressBarSize() const {
            return m_progressBarSize;
        }

        void LoadingScreen::setStatusTextPosition(float x, float y) {
            m_statusTextPosition = glm::vec2(x, y);
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

        void LoadingScreen::defaultSetup() {
            //Set default digipen screen for texture rendering
#ifdef PN_PLATFORM_WINDOWS
            std::filesystem::path tex_path = "engine/textures/DigiPen_BLACK.png";
#else
            std::filesystem::path tex_path = "engine\\textures\\DigiPen_BLACK.png";
#endif
            m_backgroundTextureGUID = services.lock()->get<Assets::Manager>()->findGUID(tex_path);
            m_backGroundColor = { 0.1f, 0.1f, 0.1f };

            //Default BG Scale
            bgScale = 1.0f;

            //Default booleans
            showBg = true;
            showOverlay = false;
            showProgress = false;
            showStatus = false;

            //Setup other variables
            auto win = services.lock()->get<Window::Window>();
            if (win) {
                auto framebuffer = win->getFrameBuffer();
                float screenWidth = framebuffer.x;
                float screenHeight = framebuffer.y;

                // Set default progress bar position and size (if not already set by user)
                m_progressBarPosition = glm::vec2(screenWidth / 2.0f, screenHeight * 0.15f);
                m_progressBarSize = glm::vec2(screenWidth * 0.6f, 40.0f);

                // Set default progress bar color and overlay color
                m_fillColor = { 0.2f, 0.8f, 0.9f };
                m_glowColor = { 0.4f, 0.9f, 1.0f };
                m_glowIntensity = 0.5f;
                m_overlayColor1 = {0.05f, 0.05f, 0.1f };
                m_overlayColor2 = { 0.02f, 0.02f, 0.05f };
                m_overlayStrength = 0.8f;

                // Set default status text position (if not already set by user)
                m_statusTextPosition = glm::vec2(screenWidth / 2.0f, m_progressBarPosition.y - 70.0f);
                m_statusTextScale = 0.03f;
            }

            buildProgressBarVertices();
        }

        bool LoadingScreen::getShowBG() const {
            return showBg;
        }

        void LoadingScreen::setShowBG(bool show) {
            showBg = show;
        }

        bool LoadingScreen::getShowOverlay() const {
            return showOverlay;
        }

        void LoadingScreen::setShowOverlay(bool show) {
            showOverlay = show;
        }

        bool LoadingScreen::getShowProgressBar() const {
            return showProgress;
        }

        void LoadingScreen::setShowProgressBar(bool show) {
            showProgress = show;
        }

        bool LoadingScreen::getShowStatusText() const {
            return showStatus;
        }

        void LoadingScreen::setShowStatusText(bool show) {
            showStatus = show;
        }

        float LoadingScreen::getBGScale() const {
            return bgScale;
        }

        void LoadingScreen::setBGScale(float scale) {
            bgScale = scale;
        }

        // ============================================================
        // Spritesheet Animation Implementation
        // ============================================================

        void LoadingScreen::setSpritesheetAnimation(int frameCount, int framesPerRow, float frameTime) {
            m_frameCount = std::max(1, frameCount);
            m_framesPerRow = std::max(1, framesPerRow);
            m_frameTime = std::max(0.01f, frameTime);
            m_currentFrameIndex = 0;
            m_currentFrameTime = 0.0f;
        }

        void LoadingScreen::setAnimationEnabled(bool enabled) {
            m_animationEnabled = enabled && (m_frameCount > 1);
        }

        std::tuple<int, int, float, bool> LoadingScreen::getSpritesheetSettings() const {
            return std::make_tuple(m_frameCount, m_framesPerRow, m_frameTime, m_animationEnabled);
        }

        // ============================================================
        // Style Customization Implementations
        // ============================================================

        void LoadingScreen::setProgressBarFillColor(const glm::vec3& color) {
            m_fillColor = color;
        }

        void LoadingScreen::setProgressBarGlowColor(const glm::vec3& color) {
            m_glowColor = color;
        }

        void LoadingScreen::setProgressBarGlowIntensity(float intensity) {
            m_glowIntensity = glm::clamp(intensity, 0.0f, 2.0f);
        }

        std::tuple<glm::vec3, glm::vec3, float> LoadingScreen::getProgressBarStyle() const {
            return std::make_tuple(m_fillColor, m_glowColor, m_glowIntensity);
        }

        // ============================================================
        // Helper Method - Build Progress Bar Vertices
        // ============================================================

        void LoadingScreen::buildProgressBarVertices() {
            auto serv = services.lock();
            if (!serv) return;
            auto win = serv->get<Window::Window>();
            if (!win) return;

            auto framebuffer = win->getFrameBuffer();
            float screenWidth = framebuffer.x;
            float screenHeight = framebuffer.y;

            // Calculate position and size in screen space
            float barWidth = m_progressBarSize.x;
            float barHeight = m_progressBarSize.y;
            float barX = m_progressBarPosition.x;
            float barY = m_progressBarPosition.y;

            // Convert screen space to NDC
            // X: left edge (0) -> -1, right edge (screenWidth) -> +1
            // Y: top edge (0) -> +1, bottom edge (screenHeight) -> -1
            float ndcX = (barX / screenWidth) * 2.0f - 1.0f;
            float ndcY = (barY / screenHeight) * 2.0f - 1.0f;
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
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }

        // ============================================================
        // Preview Mode Implementation
        // ============================================================

        void LoadingScreen::renderPreview(float progress, const std::string& status) {
            //Update delta time using member variable to track time between frames
            auto now = std::chrono::steady_clock::now();
            float real_dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
            m_lastFrameTime = now;

            // Store Unscaled Time (Always ticking even when paused)
            timing.unscaled_dt = real_dt;
            timing.dt = real_dt;

            // Update animation time
            m_animationTime += timing.dt;

            // Update spritesheet animation frame if enabled
            if (m_animationEnabled && m_frameCount > 1) {
                m_currentFrameTime += timing.dt;
                if (m_currentFrameTime >= m_frameTime) {
                    m_currentFrameTime = 0.0f;
                    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_frameCount;
                }
            }

            // Temporarily override progress and status
            float oldProgress = m_progress.load();
            std::string oldStatus;
            {
                std::lock_guard<std::mutex> lock(m_statusMutex);
                oldStatus = m_statusText;
                m_statusText = status;
            }
            m_progress.store(progress);
            
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
            // Clear screen with dark background
            glClearColor(m_backGroundColor.r, m_backGroundColor.g, m_backGroundColor.b, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Render in Z-order (back to front):
            // Layer 1: Background texture (if set)
            if (showBg) renderBackgroundTexture();

            // Layer 2: Animated gradient overlay
            if (showOverlay) renderBackgroundOverlay();

            // Layer 3: Progress bar
            if (showProgress) renderProgressBar();

            // Layer 4: Status text
            if (showStatus) renderStatusText();

            ////Unbind frame buffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            
            // Restore original values
            m_progress.store(oldProgress);
            {
                std::lock_guard<std::mutex> lock(m_statusMutex);
                m_statusText = oldStatus;
            }
        }
    }
}

