#pragma once

#ifndef LOADING_SCREEN_HPP
#define LOADING_SCREEN_HPP

#include <string>
#include <atomic>
#include <mutex>

#include "CoreSystems/Windows/Window.h"

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

            // Helper methods
            void renderProgressBar();
            void renderStatusText();
            unsigned int compileShader();
        };

    }
}

#endif
