#pragma once

#ifndef LOADING_SCREEN_HPP
#define LOADING_SCREEN_HPP

#include <string>
#include <atomic>
#include <mutex>
#include <chrono>

#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Renderer/sRenderer.h"

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
            ~LoadingScreen() = default;

            /**
             * @brief Initialize OpenGL resources (VAO, VBO, shader)
             * Must be called on main thread with active OpenGL context
             */
            void init(std::shared_ptr<Window::Window> win);

            /**
             * @brief Cleanup OpenGL resources
             * Must be called on main thread with active OpenGL context
             */
            void cleanup();

            /**
             * @brief Render the loading screen (progress bar + status text)
             * Must be called on main thread
             * This also swaps buffers and polls events
             */
            void render();

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

        private:
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
            std::weak_ptr<Window::Window> win_ptr;

            // Renderer access for 2D texture rendering
            std::weak_ptr<sRenderer> renderer_ptr;

            // Optional background texture
            Assets::GUID m_backgroundTextureGUID;

            // Custom shader programs for loading screen effects
            unsigned int m_progressBarShader{ 0 };
            unsigned int m_overlayShader{ 0 };

            // Animation timing
            float m_animationTime{ 0.0f };
            std::chrono::steady_clock::time_point m_lastFrameTime;

            // Layout configuration (in screen space -1 to 1)
            float m_progressBarWidth{ 0.6f };   // 60% of screen width
            float m_progressBarHeight{ 0.05f }; // 5% of screen height
            float m_progressBarY{ -0.3f };      // Y position (below center)

            // Color scheme
            glm::vec3 m_fillColor{ 0.2f, 0.8f, 0.9f };      // Cyan
            glm::vec3 m_glowColor{ 0.4f, 0.9f, 1.0f };      // Light cyan
            float m_glowIntensity{ 0.5f };
            glm::vec3 m_overlayColor1{ 0.05f, 0.05f, 0.1f }; // Dark blue
            glm::vec3 m_overlayColor2{ 0.02f, 0.02f, 0.05f }; // Darker blue
            float m_overlayStrength{ 0.8f };

            // Helper methods
            void renderBackgroundTexture();
            void renderBackgroundOverlay();
            void renderProgressBar();
            void renderTitle();
            void renderStatusText();
            void renderPercentage();
            
            unsigned int compileShader();
            unsigned int compileProgressBarShader();
            unsigned int compileOverlayShader();
        };

    }
}

#endif
