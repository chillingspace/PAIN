/**
 * @file WindowsRenderer.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-09-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */


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
		glEnable(GL_CULL_FACE); 
		glCullFace(GL_BACK);

		if (!createBuffers()) {
			PN_CORE_ERROR("Failed to create buffers");
		}
	}

	bool WindowsRenderer::createBuffers() {
		m_shader->Bind();

		// Generate and bind VAO
		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);

		// Generate and bind VBO
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		// Generate and bind EBO (index buffer)
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


		// Position attribute, layout(location = 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);

		// Color attribute, layout(location = 1)
		//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));
		//glEnableVertexAttribArray(1);

		// Normal attribute, layout(location = 1)
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
		glEnableVertexAttribArray(1);

		// Unbind VAO
		glBindVertexArray(0);

		PN_CORE_INFO("Buffers created successfully");
		return true;
	}

	void WindowsRenderer::Render() {
		// update
		light.position = Camera::get().pos;
		


		// render
		// Clear screen
		glClearColor(clearColor[0], clearColor[1], clearColor[2], 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Bind shader and VAO, then draw triangle
		if (!m_shader) {
			PN_CORE_ERROR("Unable to find m_shaders");
			return;
		}
		m_shader->Bind();

		// Rotation mtx
		float angle = static_cast<float>(glfwGetTime());
		glm::mat4 model = glm::rotate(glm::mat4(1.f), angle, glm::vec3(1.f, -1.f, -1.f));
		glm::mat4 mvp = Camera::get().projection() * Camera::get().view() * model;

		glUniformMatrix4fv(glGetUniformLocation(m_shader->GetRendererID(), "u_Model"), 1, GL_FALSE, &model[0][0]);
		//glUniformMatrix4fv(glGetUniformLocation(m_shader->GetRendererID(), "u_MVP"), 1, GL_FALSE, &mvp[0][0]);

		glUniformMatrix4fv(glGetUniformLocation(m_shader->GetRendererID(), "u_M"), 1, GL_FALSE, &model[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(m_shader->GetRendererID(), "u_V"), 1, GL_FALSE, &Camera::get().view()[0][0]);
		glUniformMatrix4fv(glGetUniformLocation(m_shader->GetRendererID(), "u_P"), 1, GL_FALSE, &Camera::get().projection()[0][0]);

		glUniform3f(glGetUniformLocation(m_shader->GetRendererID(), "u_LightDir"), 0.f, 0.0f, -1.f);
		glUniform3f(glGetUniformLocation(m_shader->GetRendererID(), "u_LightColor"), 1.0f, 1.0f, 1.0f);

		glUniform1f(glGetUniformLocation(m_shader->GetRendererID(), "material.rough"), material.rough);
		glUniform1f(glGetUniformLocation(m_shader->GetRendererID(), "material.metal"), material.metal);
		glUniform3f(glGetUniformLocation(m_shader->GetRendererID(), "material.color"), material.color.r, material.color.g, material.color.b);

		glUniform3f(glGetUniformLocation(m_shader->GetRendererID(), "light[0].position"), light.position.x, light.position.y, light.position.z);
		glUniform3f(glGetUniformLocation(m_shader->GetRendererID(), "light[0].L"), light.L_intensity.x, light.L_intensity.y, light.L_intensity.z);

		glBindVertexArray(vao);
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
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

		if (ebo != 0) {
			glDeleteBuffers(1, &ebo);
			ebo = 0;
		}

		if (m_shader) {
			m_shader.reset();
		}

	}
}

#endif // PN_PLATFORM_WINDOWS