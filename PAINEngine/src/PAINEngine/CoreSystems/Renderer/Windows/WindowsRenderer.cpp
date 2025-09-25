#ifdef PN_PLATFORM_WINDOWS

#include "WindowsRenderer.h"
#include <cstring>

namespace PAIN {

	WindowsRenderer::WindowsRenderer() {
		clearColor[0] = 0.2f;
		clearColor[1] = 0.3f;
		clearColor[2] = 0.3f;
	}

	WindowsRenderer::~WindowsRenderer() {
		Cleanup();
	}

	void WindowsRenderer::Init() {
		if (!createShaders()) {
			PN_CORE_ERROR("Failed to create shaders");
		}

		if (!createBuffers()) {
			PN_CORE_ERROR("Failed to create buffers");
		}
	}

	bool WindowsRenderer::createShaders() {
		const char* vertexSrc = R"(
        #version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aColor;
        out vec3 vColor;
        void main() {
            gl_Position = vec4(aPos, 1.0);
            vColor = aColor;
        }
    )";

		const char* fragmentSrc = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

		m_shaders = std::make_unique<Shader>(vertexSrc, fragmentSrc);

		if (!m_shaders) {
			PN_CORE_ERROR("Failed to create shader program");
			return false;
		}

		program = m_shaders->GetRendererID();

		return true;
	}

	bool WindowsRenderer::createBuffers() {
		float vertices[] = {
			 0.0f,  0.5f, 0.0f,     1.0f, 0.0f, 0.0f,
			-0.5f, -0.5f, 0.0f,     0.0f, 1.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f
		};

		const char* version = (const char*)glGetString(GL_VERSION);
		bool useVAO = (version && strstr(version, "OpenGL ES 3"));

		// Create VAO (OpenGL ES 3.0+)
		glGenVertexArrays(1, &vao);
		if (vao == 0) {
			PN_CORE_ERROR("Failed to create VAO");
			return false;
		}
		glBindVertexArray(vao);
		PN_CORE_INFO("Using VAO for vertex attributes");

		PN_CORE_ERROR("VAO not supported, using direct attribute binding");


		// Create VBO
		glGenBuffers(1, &vbo);
		if (vbo == 0) {
			PN_CORE_ERROR("Failed to create VBO");
			return false;
		}
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// Color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// Unbind
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		// For OpenGL ES 2.0, we'll bind attributes in render function
		PN_CORE_INFO("Will bind attributes in render function for OpenGL ES 2.0");


		PN_CORE_INFO("Buffers created successfully");
		return true;
	}

	void WindowsRenderer::Render() {
		// Clear the screen
		glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Use shader program
		glUseProgram(program);

		// Bind VAO and draw (OpenGL ES 3.0+)
		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		// For OpenGL ES 2.0, bind attributes manually
		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		// Position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// Color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	void WindowsRenderer::Cleanup() {
		if (vao != 0) {
			glDeleteVertexArrays(1, &vao);
			vao = 0;
		}

		if (vbo != 0) {
			glDeleteBuffers(1, &vbo);
			vbo = 0;
		}

		if (program != 0) {
			glDeleteProgram(program);
			program = 0;
		}
	}
}

#endif // PN_PLATFORM_WINDOWS