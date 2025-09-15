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
        m_cubePosition = glm::vec3(-5.0f, 0.0f, -2.0f);
        m_cameraPosition = glm::vec3(0.0f, 0.0f, 5.0f);

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

        // Animate the cube's position in a rectangle
        static int pathSegment = 0;
        static float progress = 0.0f;
        float speed = 0.02f;

        glm::vec3 points[4] = {
            {-5.0f, 0.0f, -2.0f}, // Top-left
            { 5.0f, 0.0f, -2.0f}, // Top-right
            { 5.0f, 0.0f,  2.0f}, // Bottom-right
            {-5.0f, 0.0f,  2.0f}  // Bottom-left
        };

        progress += speed;
        if (progress >= 1.0f) {
            progress = 0.0f;
            pathSegment = (pathSegment + 1) % 4;
        }

        glm::vec3 startPoint = points[pathSegment];
        glm::vec3 endPoint = points[(pathSegment + 1) % 4];
        m_cubePosition = glm::mix(startPoint, endPoint, progress);

        // Update the FMOD listener's position and orientation to match the camera
        glm::vec3 cameraVelocity = { 0.0f, 0.0f, 0.0f };
        glm::vec3 cameraForward = glm::normalize(glm::vec3(0, 0, -1));
        glm::vec3 cameraUp = { 0.0f, 1.0f, 0.0f };
        audio.SetListener(m_cameraPosition, cameraVelocity, cameraForward, cameraUp);

        // Play a random footstep sound from the playlist periodically
        static int frameCount = 0;
        if (++frameCount % 100 == 0) // Changed from 60 to 100 for a ~0.7s delay
        {
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