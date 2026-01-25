#pragma once

#ifndef LOADING_SCREEN_HPP
#define LOADING_SCREEN_HPP

#include <string>
#include <atomic>
#include <mutex>
#include <chrono>

namespace PAIN {
    namespace Scene {

        /**
         * @brief Simple loading screen renderer for async scene loading
         *
         * Displays a progress bar and status text while scene assets are loading
         * on a worker thread. All rendering happens on the main thread, but progress
         * updates are thread-safe via atomic operations.
         */
        class LoadingScreen {
        public:
            LoadingScreen() = default;
            ~LoadingScreen() {
                cleanup();
            }

            /**
             * @brief Initialize OpenGL resources (VAO, VBO, shader)
             * Must be called on main thread with active OpenGL context
             */
            void init(std::shared_ptr<Services> serv);

            /**
             * @brief Render the loading screen (progress bar + status text)
             * Must be called on main thread
             * This also swaps buffers and polls events
             */
            void render();

            /**
             * @brief Finish the loading screen rendered into frame buffer
             * Must be called on main thread
             * This also swaps buffers and polls events
             */
            void finish();

            /**
             * @brief Update progress value (thread-safe)
             * @param progress Progress value from 0.0 to 1.0
             * Can be called from worker thread
             */
            void setProgress(float progress);

            /**
             * @brief Update status text (thread-safe)
             * @param status New status message to display
             * Can be called from worker thread
             */
            void setStatus(const std::string& status);

            /**
             * @brief Set background texture to display (optional)
             * @param textureGUID GUID of texture asset to use as background
             * Can be called before init()
             */
            void setBackgroundTexture(const Assets::GUID& textureGUID);

            /**
             * @brief Get background texture to display (optional)
             */
            Assets::GUID getBackgroundTexture();

            // ============================================================
            // Runtime Configuration - Progress Bar
            // ============================================================
            
            /**
             * @brief Set progress bar position in screen space (world coordinates)
             * @param x X position in pixels from left edge of screen
             * @param y Y position in pixels from top edge of screen
             */
            void setProgressBarPosition(float x, float y);
            
            /**
             * @brief Set progress bar size in screen space
             * @param width Width in pixels
             * @param height Height in pixels
             */
            void setProgressBarSize(float width, float height);
            
            /**
             * @brief Get progress bar position in screen space
             * @return glm::vec2 Position (x, y) in pixels
             */
            glm::vec2 getProgressBarPosition() const;
            
            /**
             * @brief Get progress bar size in screen space
             * @return glm::vec2 Size (width, height) in pixels
             */
            glm::vec2 getProgressBarSize() const;

            // ============================================================
            // Runtime Configuration - Status Text
            // ============================================================
            
            /**
             * @brief Set status text position in screen space (world coordinates)
             * @param x X position in pixels from left edge of screen
             * @param y Y position in pixels from top edge of screen
             */
            void setStatusTextPosition(float x, float y);
            
            /**
             * @brief Set status text scale factor
             * @param scale Scale multiplier for font size (default 1.0)
             */
            void setStatusTextScale(float scale);
            
            /**
             * @brief Get status text position in screen space
             * @return glm::vec2 Position (x, y) in pixels
             */
            glm::vec2 getStatusTextPosition() const;
            
            /**
             * @brief Get status text scale factor
             * @return float Scale multiplier
             */
            float getStatusTextScale() const;

            // ============================================================
            // Spritesheet Animation Support
            // ============================================================
            
            /**
             * @brief Configure spritesheet animation for background texture
             * @param frameCount Number of frames in the spritesheet
             * @param framesPerRow Number of frames per row in the spritesheet
             * @param frameTime Time per frame in seconds
             */
            void setSpritesheetAnimation(int frameCount, int framesPerRow, float frameTime);
            
            /**
             * @brief Enable/disable spritesheet animation
             * @param enabled True to enable animation, false to disable
             */
            void setAnimationEnabled(bool enabled);
            
            /**
             * @brief Get spritesheet animation settings
             * @return Tuple of (frameCount, framesPerRow, frameTime, enabled)
             */
            std::tuple<int, int, float, bool> getSpritesheetSettings() const;

            // ============================================================
            // Preview Mode
            // ============================================================
            
            /**
             * @brief Render loading screen for preview (editor mode)
             * @param progress Progress value to display (0.0 - 1.0)
             * @param status Status text to display
             */
            void renderPreview(float progress, const std::string& status);

        private:

            // Simple vertex shader for fullscreen quad !!FALLBACK
            const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

            // Simple fragment shader for solid colors !!FALLBACK
            const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 color;
uniform float alpha;

void main() {
    FragColor = vec4(color, alpha);
}
)";

            //Internal blocking timing
            AppTiming timing;

            // Thread-safe progress tracking
            std::atomic<float> m_progress{ 0.0f };

            // Thread-safe status text
            std::string m_statusText;
            std::mutex m_statusMutex;

            // OpenGL resources (created on main thread only)
            unsigned int m_vao{ 0 };
            unsigned int m_vbo{ 0 };
            unsigned int m_shader{ 0 };

            // Window dimensions for aspect ratio calculation
            std::weak_ptr<Services> services;

            // Optional background texture
            Assets::GUID m_backgroundTextureGUID;

            // Custom shader programs for loading screen effects
            unsigned int m_progressBarShader{ 0 };
            unsigned int m_overlayShader{ 0 };

            // Animation timing
            float m_animationTime{ 0.0f };
            std::chrono::steady_clock::time_point m_lastFrameTime;

            // Progress Bar - Screen Space Configuration (in pixels)
            glm::vec2 m_progressBarPosition{ 0.0f, 0.0f };  // Position (x, y) in screen space
            glm::vec2 m_progressBarSize{ 600.0f, 40.0f };    // Size (width, height) in pixels
            bool m_useCustomProgressBarPos{ false };          // True if custom position set
            bool m_useCustomProgressBarSize{ false };         // True if custom size set

            // Status Text - Screen Space Configuration
            glm::vec2 m_statusTextPosition{ 0.0f, 0.0f };    // Position (x, y) in screen space
            float m_statusTextScale{ 0.02f };                  // Scale factor for font size
            bool m_useCustomStatusTextPos{ false };           // True if custom position set

            // Spritesheet Animation Support
            int m_frameCount{ 1 };                            // Number of frames in spritesheet
            int m_framesPerRow{ 1 };                          // Frames per row in spritesheet
            float m_frameTime{ 0.1f };                        // Time per frame in seconds
            float m_currentFrameTime{ 0.0f };                 // Accumulated time for current frame
            int m_currentFrameIndex{ 0 };                     // Current frame index
            bool m_animationEnabled{ false };                 // Animation enabled flag

            // Color scheme
            glm::vec3 m_fillColor{ 0.2f, 0.8f, 0.9f };      // Cyan
            glm::vec3 m_glowColor{ 0.4f, 0.9f, 1.0f };      // Light cyan
            float m_glowIntensity{ 0.5f };
            glm::vec3 m_overlayColor1{ 0.05f, 0.05f, 0.1f }; // Dark blue
            glm::vec3 m_overlayColor2{ 0.02f, 0.02f, 0.05f }; // Darker blue
            float m_overlayStrength{ 0.8f };

            /**
            * @brief Cleanup OpenGL resources
            * Must be called on main thread with active OpenGL context
            */
            void cleanup();

            // Helper methods
            void renderBackgroundTexture();
            void renderBackgroundOverlay();
            void renderProgressBar();
            void renderStatusText();
            
            unsigned int compileShader();
            unsigned int compileProgressBarShader();
            unsigned int compileOverlayShader();
        };

    }
}

#endif
