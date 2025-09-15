#include "pch.h"
#include "TestTriangleLayer.h"


namespace PAIN {
	 
    TestTriangleLayer::TestTriangleLayer() {

        // Vertex data: position (x,y,z) + color (r,g,b)
        float vertices[] = {
            // positions        // colors
            -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // bottom-left, red
             0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // bottom-right, green
             0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f  // top, blue
        };

        // Generate VAO and VBO
        glGenVertexArrays(1, &m_VAO);
        glGenBuffers(1, &m_VBO);

        glBindVertexArray(m_VAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Vertex position
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

        // Vertex color
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

        // Simple shaders
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

        // Setup Shaders
        m_Shader = std::make_unique<Shader>(vertexShaderSrc, fragmentShaderSrc);

        // JSON test
        //json j;
        //j["hello"] = "world";
        //j["value"] = 123;

        //std::ofstream out("test.json");
        //out << j.dump(4);
    }

    TestTriangleLayer::~TestTriangleLayer() {
        glDeleteBuffers(1, &m_VBO);
        glDeleteVertexArrays(1, &m_VAO);
    }


    void TestTriangleLayer::onUpdate() {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        m_Shader->Bind();
        glBindVertexArray(m_VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        m_Shader->UnBind();
    }
}
