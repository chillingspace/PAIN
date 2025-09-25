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
		PN_CORE_INFO("Shader program ID: {}", m_shader->GetRendererID());

		if (!createBuffers()) {
			PN_CORE_ERROR("Failed to create buffers");
		}
	}

	bool WindowsRenderer::createBuffers() {
		float vertices[] = {
			 0.0f,  0.5f, 0.0f,     1.0f, 0.0f, 0.0f,
			-0.5f, -0.5f, 0.0f,     0.0f, 1.0f, 0.0f,
			 0.5f, -0.5f, 0.0f,     0.0f, 0.0f, 1.0f
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
		glClear(GL_COLOR_BUFFER_BIT);

		// Bind shader and VAO, then draw triangle
		m_shader->Bind();
		glBindVertexArray(vao);
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

		m_shader.reset();
	}
}

#endif // PN_PLATFORM_WINDOWS