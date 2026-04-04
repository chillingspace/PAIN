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

            /**
             * @brief Set background color
             */
            void setBackgroundColor(glm::vec3 const& bg_color);

            /**
             * @brief Get background color
             */
            glm::vec3 getBackgroundColor();

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
            
            void setSpritesheetTexture(const Assets::GUID& textureGUID);
            Assets::GUID getSpritesheetTexture() const;
            void setSpritesheetScale(float scale);
            float getSpritesheetScale() const;
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

            // ============================================================
            // Style Customization - Progress Bar Colors
            // ============================================================
            
            /**
             * @brief Set progress bar fill color
             * @param color RGB color for filled portion
             */
            void setProgressBarFillColor(const glm::vec3& color);
            
            /**
             * @brief Set progress bar glow color
             * @param color RGB color for glow effect
             */
            void setProgressBarGlowColor(const glm::vec3& color);
            
            /**
             * @brief Set glow intensity
             * @param intensity Glow intensity multiplier (0.0 - 2.0)
             */
            void setProgressBarGlowIntensity(float intensity);
            
            /**
             * @brief Get progress bar colors and style
             * @return Tuple of (fillColor, glowColor, glowIntensity)
             */
            std::tuple<glm::vec3, glm::vec3, float> getProgressBarStyle() const;

            /**
             * @brief Set progress bar and status text to default
             */
            void defaultSetup();

            /**
             * @brief Get boolean show Background
             */
            bool getShowBG() const;

            /**
             * @brief Set boolean show Background
             */
            void setShowBG(bool show);

            /**
             * @brief Get boolean show Overlay
             */
            bool getShowOverlay() const;

            /**
             * @brief Set boolean show Overlay
             */
            void setShowOverlay(bool show);

            /**
             * @brief Get boolean show Progress Bar
             */
            bool getShowProgressBar() const;

            /**
             * @brief Set boolean show Progress Bar
             */
            void setShowProgressBar(bool show);

            /**
             * @brief Get boolean show Status Text
             */
            bool getShowStatusText() const;

            /**
             * @brief Set boolean show Status Text
             */
            void setShowStatusText(bool show);

            /**
            * @brief Get background texture scale
            */
            float getBGScale() const;

            /**
             * @brief Set background texture scale
             */
            void setBGScale(float scale);

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

            // Procedural animated background (Apollonian fractal 2D)
            const char* proceduralFragmentShaderSource = R"(
#version 330 core
uniform float iTime;
uniform vec2 iResolution;
out vec4 FragColor;

#define RESOLUTION  iResolution
#define TIME        iTime
#define MAX_MARCHES 30
#define TOLERANCE   0.0001
#define ROT(a)      mat2(cos(a), sin(a), -sin(a), cos(a))
#define PI          3.141592654
#define TAU         (2.0*PI)

const mat2 rot0 = ROT(0.0);
mat2 g_rot0 = rot0;
mat2 g_rot1 = rot0;

float sRGB(float t) { return mix(1.055*pow(t, 1./2.4) - 0.055, 12.92*t, step(t, 0.0031308)); }
vec3 sRGB(in vec3 c) { return vec3(sRGB(c.x), sRGB(c.y), sRGB(c.z)); }

const vec4 hsv2rgb_K = vec4(1.0, 2.0/3.0, 1.0/3.0, 3.0);
vec3 hsv2rgb(vec3 c) {
  vec3 p = abs(fract(c.xxx + hsv2rgb_K.xyz) * 6.0 - hsv2rgb_K.www);
  return c.z * mix(hsv2rgb_K.xxx, clamp(p - hsv2rgb_K.xxx, 0.0, 1.0), c.y);
}

float apolloian(vec3 p, float s, out float h) {
  float scale = 1.0;
  for(int i=0; i < 5; ++i) {
    p = -1.0 + 2.0*fract(0.5*p+0.5);
    float r2 = dot(p,p);
    float k  = s/r2;
    p       *= k;
    scale   *= k;
  }
  vec3 ap = abs(p/scale);
  float d = length(ap.xy);
  d = min(d, ap.z);
  h = (d == ap.z) ? 0.5 : 0.0;
  return d;
}

float df(vec2 p, out float h) {
  const float fz = 1.0;
  float z = 1.55*fz;
  p /= z;
  vec3 p3 = vec3(p, 0.1);
  p3.xz *= g_rot0;
  p3.yz *= g_rot1;
  float d = apolloian(p3, 1.0/fz, h);
  return d * z;
}

float shadow(vec2 lp, vec2 ld, float mint, float maxt) {
  const float ds   = 0.6;
  const float soff = 0.05;
  const float smul = 1.5;
  float t  = mint;
  float nd = 1e6;
  float h;
  for (int i=0; i < MAX_MARCHES; ++i) {
    float d = df(lp + ld*t, h);
    if (d < TOLERANCE || t >= maxt) {
      float sd = 1.0 - exp(-smul*max(t/maxt-soff, 0.0));
      return t >= maxt ? mix(sd, 1.0, smoothstep(0.0, 0.025, nd)) : sd;
    }
    nd = min(nd, d);
    t += ds*d;
  }
  return 1.0 - exp(-smul*max(t/maxt-soff, 0.0));
}

vec3 effect(vec2 p, vec2 q) {
  float a  = 0.1*TIME;
  g_rot0   = ROT(0.5*a);
  g_rot1   = ROT(sqrt(0.5)*a);

  vec2  lightPos  = vec2(0.0, 1.0) * g_rot1;
  vec2  lightDiff = lightPos - p;
  float lightLen  = length(lightDiff);
  vec2  lightDir  = lightDiff / lightLen;
  vec3  lightPos3 = vec3(lightPos, 0.0);
  vec3  p3        = vec3(p, -1.0);
  float lightLen3 = distance(lightPos3, p3);
  vec3  lightDir3 = normalize(lightPos3 - p3);
  float diff      = max(dot(lightDir3, vec3(0.0, 0.0, 1.0)), 0.0);

  float h;
  float d  = df(p, h);
  float ss = shadow(p, lightDir, 0.005, lightLen);
  vec3 bcol = hsv2rgb(vec3(fract(h - 0.2*length(p) + 0.25*TIME), 0.666, 1.0));

  vec3 col = vec3(0.0);
  col += mix(0.0, 1.0, diff)*0.5*mix(0.1, 1.0, ss)/(lightLen3*lightLen3);
  col += exp(-300.0*abs(d))*sqrt(bcol);
  col += exp(-40.0*max(lightLen - 0.02, 0.0));
  return col;
}

void main() {
  vec2 q = gl_FragCoord.xy / RESOLUTION.xy;
  vec2 p = -1.0 + 2.0*q;
  p.x *= RESOLUTION.x / RESOLUTION.y;

  vec3 col = effect(p, q);
  col *= mix(0.0, 1.0, smoothstep(0.0, 4.0, TIME));
  col  = sRGB(col);
  FragColor = vec4(col, 1.0);
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

            // Custom shader programs for loading screen effects
            unsigned int m_progressBarShader{ 0 };
            unsigned int m_overlayShader{ 0 };
            unsigned int m_proceduralShader{ 0 };

            // Animation timing
            float m_animationTime{ 0.0f };
            std::chrono::steady_clock::time_point m_lastFrameTime;

            // Optional background texture
            Assets::GUID m_backgroundTextureGUID;
            glm::vec3 m_backGroundColor{ 0.1f, 0.1f, 0.1f };
            float bgScale = 1.0f;

            // Spritesheet overlay (use existing animation settings)
            Assets::GUID m_spritesheetTextureGUID;
            float spritesheetScale = 1.0f;

            // Progress Bar - Screen Space Configuration (in pixels)
            glm::vec2 m_progressBarPosition{ 0.0f, 0.0f };  // Position (x, y) in screen space
            glm::vec2 m_progressBarSize{ 600.0f, 40.0f };    // Size (width, height) in pixels

            // Status Text - Screen Space Configuration
            glm::vec2 m_statusTextPosition{ 0.0f, 0.0f };    // Position (x, y) in screen space
            float m_statusTextScale{ 0.03f };                  // Scale factor for font size

            // Spritesheet Animation Support
            int m_frameCount{ 5 };                            // Number of frames in spritesheet
            int m_framesPerRow{ 5 };                          // Frames per row in spritesheet
            float m_frameTime{ 0.1f };                        // Time per frame in seconds
            float m_currentFrameTime{ 0.0f };                 // Accumulated time for current frame
            int m_currentFrameIndex{ 0 };                     // Current frame index
            bool m_animationEnabled{ true };                 // Animation enabled flag

            // Color scheme
            glm::vec3 m_fillColor{ 0.2f, 0.8f, 0.9f };      // Cyan
            glm::vec3 m_glowColor{ 0.4f, 0.9f, 1.0f };      // Light cyan
            float m_glowIntensity{ 0.5f };
            glm::vec3 m_overlayColor1{ 0.05f, 0.05f, 0.1f }; // Dark blue
            glm::vec3 m_overlayColor2{ 0.02f, 0.02f, 0.05f }; // Darker blue
            float m_overlayStrength{ 0.8f };

            //Internal boolean to render BG / Progress / Status
            bool showBg = true;
            bool showOverlay = false;
            bool showProgress = false;
            bool showStatus = false;

            /**
            * @brief Cleanup OpenGL resources
            * Must be called on main thread with active OpenGL context
            */
            void cleanup();

            // Helper methods
            void renderBackgroundTexture();
            void renderSpritesheetLayer();
            void renderBackgroundOverlay();
            void renderProgressBar();
            void renderStatusText();
            
            unsigned int compileShader();
            unsigned int compileProgressBarShader();
            unsigned int compileOverlayShader();
            unsigned int compileProceduralShader();

            void renderProceduralBackground();
            void buildProgressBarVertices();
        };

    }
}

#endif
