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
		m_shader = Shader::LoadShaders("base.vert", "base.frag");

		if (!m_shader || m_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		glEnable(GL_DEPTH_TEST);

		if (!createBuffers()) {
			PN_CORE_ERROR("Failed to create buffers");
		}
	}

	bool WindowsRenderer::createBuffers() {

		float vertices[] = {
			// positions           // colors
			-0.5f,-0.5f,-0.5f,   1,0,0,    0.5f,-0.5f,-0.5f,   0,1,0,    0.5f, 0.5f,-0.5f,  0,0,1,
			 0.5f, 0.5f,-0.5f,   0,0,1,   -0.5f, 0.5f,-0.5f,   1,1,0,   -0.5f,-0.5f,-0.5f, 1,0,0,

			-0.5f,-0.5f, 0.5f,   1,0,1,    0.5f,-0.5f, 0.5f,   0,1,1,    0.5f, 0.5f, 0.5f,  1,1,1,
			 0.5f, 0.5f, 0.5f,   1,1,1,   -0.5f, 0.5f, 0.5f,   0,0,0,   -0.5f,-0.5f, 0.5f, 1,0,1,

			-0.5f, 0.5f, 0.5f,   1,0,0,   -0.5f, 0.5f,-0.5f,   0,1,0,   -0.5f,-0.5f,-0.5f,0,0,1,
			-0.5f,-0.5f,-0.5f,   0,0,1,   -0.5f,-0.5f, 0.5f,   1,1,0,   -0.5f, 0.5f, 0.5f, 1,0,0,

			 0.5f, 0.5f, 0.5f,   1,0,1,    0.5f, 0.5f,-0.5f,  0,1,1,    0.5f,-0.5f,-0.5f,0,0,0,
			 0.5f,-0.5f,-0.5f,   0,0,0,    0.5f,-0.5f, 0.5f,  1,1,1,    0.5f, 0.5f, 0.5f, 1,0,1,

			-0.5f,0.5f,-0.5f,    1,1,0,    0.5f,0.5f,-0.5f,   0,1,1,    0.5f,0.5f, 0.5f,  1,0,1,
			 0.5f,0.5f,0.5f,     1,0,1,   -0.5f,0.5f, 0.5f,   0,1,0,   -0.5f,0.5f,-0.5f, 1,1,0,

			-0.5f,-0.5f,-0.5f,   1,0,0,    0.5f,-0.5f,-0.5f,  0,1,0,    0.5f,-0.5f,0.5f,   0,0,1,
			 0.5f,-0.5f,0.5f,    0,0,1,   -0.5f,-0.5f,0.5f,   1,1,0,   -0.5f,-0.5f,-0.5f,1,0,0
		};

		m_shader->Bind();

		// Generate and bind VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate and bind VBO
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);

		// Color attribute
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		// Unbind VAO
		glBindVertexArray(0);

		PN_CORE_INFO("Buffers created successfully");
		return true;
	}

	void WindowsRenderer::Render() {
		// Clear screen
		glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Bind shader and VAO, then draw triangle
		m_shader->Bind();

		// Rotation mtx
		float angle = static_cast<float>(glfwGetTime());
		glm::mat4 model = glm::rotate(glm::mat4(1.f), angle, glm::vec3(.5f, 1.f, 0.f));
		glm::mat4 view = glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -3.f));
		glm::mat4 proj = glm::perspective(glm::radians(45.f), 1280.f / 720.f, .1f, 100.f);
		glm::mat4 mvp = proj * view * model;

		GLuint loc = glGetUniformLocation(m_shader->GetRendererID(), "u_MVP");
		glUniformMatrix4fv(loc, 1, GL_FALSE, &mvp[0][0]);

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
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

		if (m_shader) {
			m_shader.reset();
		}

	}
}

#endif // PN_PLATFORM_WINDOWS