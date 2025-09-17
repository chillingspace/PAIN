#include "pch.h"
#include "TestTriangleLayer.h"
#include "PAINEngine/Applications/Application.h"

namespace PAIN {

    TestTriangleLayer::TestTriangleLayer() {
        // Vertex data for the triangle
        float vertices[] = {
            // positions        // colors
            -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f
        };

        // Generate and configure VAO and VBO
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

        // Basic vertex and fragment shaders
        const char* vertexShaderSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        out vec3 vColor;
        void main() {
            gl_Position = vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

        const char* fragmentShaderSrc = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

        m_Shader = std::make_unique<Shader>(vertexShaderSrc, fragmentShaderSrc);
    }

    TestTriangleLayer::~TestTriangleLayer() {
        glDeleteBuffers(1, &m_VBO);
        glDeleteVertexArrays(1, &m_VAO);
    }

    void TestTriangleLayer::onAttach() {
        // Initialize 3D positions for the demo
        m_cubePosition = glm::vec3(-12.0f, 0.0f, -4.0f); // Start at the new top-left
        m_cameraPosition = glm::vec3(0.0f, 0.0f, 0.0f);   // Camera is at the center

        // Get a reference to the audio manager
        AudioManager& audio = Application::Get().GetAudioManager();

        // Load a looping 3D music stream
        audio.LoadSound("assets/audio/Music/Boss_Music.wav", true, true, true);

        // Define and load the footstep sound effect playlist
        std::vector<std::string> grassFootsteps = {
            "assets/audio/SFX/MovingSFX/Footstep_Grass_01.wav",
            "assets/audio/SFX/MovingSFX/Footstep_Grass_02.wav",
            "assets/audio/SFX/MovingSFX/Footstep_Grass_03.wav",
            "assets/audio/SFX/MovingSFX/Footstep_Grass_04.wav",
            "assets/audio/SFX/MovingSFX/Footstep_Grass_05.wav",
            "assets/audio/SFX/MovingSFX/Footstep_Grass_06.wav",
            "assets/audio/SFX/MovingSFX/Footstep_Grass_07.wav",
            "assets/audio/SFX/MovingSFX/Footstep_Grass_08.wav"
        };
        audio.LoadPlaylist("FootstepsGrass", grassFootsteps);

        // Start the music at the cube's initial position with reduced volume
        audio.PlaySound("assets/audio/Music/Boss_Music.wav", m_cubePosition, -10.0f);
    }

    void TestTriangleLayer::onUpdate() {
        AudioManager& audio = Application::Get().GetAudioManager();

        // Animate the cube's position in a larger and wider rectangle
        static int pathSegment = 0;
        static float progress = 0.0f;
        float speed = 0.005f; // Slower speed for a longer traversal time

        glm::vec3 points[4] = {
            {-12.0f, 0.0f, -4.0f}, // Top-left (NW)
            { 12.0f, 0.0f, -4.0f}, // Top-right (NE)
            { 12.0f, 0.0f,  4.0f}, // Bottom-right (SE)
            {-12.0f, 0.0f,  4.0f}  // Bottom-left (SW)
        };

        progress += speed;
        if (progress >= 1.0f) {
            progress = 0.0f;
            pathSegment = (pathSegment + 1) % 4;

            // Log the new movement direction
            switch (pathSegment) {
            case 0: PN_CORE_INFO("Sound source moving from back-left to front-left."); break;
            case 1: PN_CORE_INFO("Sound source moving from front-left to front-right."); break;
            case 2: PN_CORE_INFO("Sound source moving from front-right to back-right."); break;
            case 3: PN_CORE_INFO("Sound source moving from back-right to back-left."); break;
            }
        }

        glm::vec3 startPoint = points[pathSegment];
        glm::vec3 endPoint = points[(pathSegment + 1) % 4];
        m_cubePosition = glm::mix(startPoint, endPoint, progress);

        // Update the FMOD listener's position and orientation. The listener is static.
        glm::vec3 cameraVelocity = { 0.0f, 0.0f, 0.0f };
        glm::vec3 cameraForward = { 0.0f, 0.0f, -1.0f }; // Camera faces forward along -Z axis
        glm::vec3 cameraUp = { 0.0f, 1.0f, 0.0f };
        audio.SetListener(m_cameraPosition, cameraVelocity, cameraForward, cameraUp);

        // Play a random footstep sound from the playlist periodically
        static int frameCount = 0;
        if (++frameCount % 100 == 0)
        {
            //PN_CORE_INFO("Playing footstep sound...");
            audio.PlayRandomFromPlaylist("FootstepsGrass", m_cubePosition, 0.0f);
        }

        // Standard rendering logic
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_Shader->Bind();
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_Shader->UnBind();
    }
}