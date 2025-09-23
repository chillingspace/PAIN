#include "pch.h"
#include "RendererLayer.h"

namespace PAIN {
	RendererLayer::RendererLayer() : program(0), 
									 vertexShader(0), 
									 fragmentShader(0), 
									 vbo(0), 
									 vao(0),
									 shader(nullptr)
#ifdef PLATFORM_WINDOWS
									 , 
									 window(nullptr)
#endif
	{	
		// default colour is black
		defaultColor[0] = 0.f;
		defaultColor[1] = 0.f;
		defaultColor[2] = 0.f;
	}

	bool RendererLayer::createShaders() {
		return false;
	}

	bool RendererLayer::createBuffers() {
		float vertices[] = {
			// positions        // colors
			-0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f, // Bottom left (red)
			 0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f, // Bottom right (green)
			 0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f  // Top (blue)
		};

        // Generate and configure VAO and VBO
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
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

        shader = std::make_unique<Shader>(vertexShaderSrc, fragmentShaderSrc);
	}

	// rendering loop
    void RendererLayer::onUpdate() {
        
        glClearColor(defaultColor[0],
                     defaultColor[1],
                     defaultColor[2],
                     1.f);

		glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);

        shader->Bind();
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        shader->UnBind();
    }
}