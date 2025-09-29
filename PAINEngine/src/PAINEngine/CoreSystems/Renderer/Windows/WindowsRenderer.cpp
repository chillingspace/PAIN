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
#include "Applications/Application.h"
#include "CoreSystems/Events/GLFW/KeyEvents.h"

namespace PAIN {

	Material material = {
	0.1f,		// 0.1 -> smooth, 1 -> rough
	0.3f,
	{0.5f,0.5f,0.5f}
	};

	Light light = {
		{2.f, 3.f, 2.f},	// position
		{0.5f, 0.5f, 0.5f},					// intensity
		Light::ORBIT_ORIGIN
	};

	WindowsRenderer::WindowsRenderer() {

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

		sphere_shader = Shader::LoadShaders("sphere.vert", "sphere.frag");

		if (!sphere_shader || sphere_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		floor_shader = Shader::LoadShaders("floor.vert", "floor.frag");

		if (!floor_shader || floor_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		glGenVertexArrays(1, &empty_vao);
		if (empty_vao == 0) {
			PN_CORE_ERROR("Failed to create empty VAO");
			return;
		}

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		m_mesh = Mesh::LoadObj("ogre.obj");

	}

	void WindowsRenderer::Render() {

		// update

		//auto* app = static_cast<Application*>(glfwGetWindowUserPointer(window));

		//light.position = Camera::get().pos;

		// render
		// Clear screen
		glClearColor(.2f, .3f, .3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		// draw floor
		if (!floor_shader) {
			PN_CORE_ERROR("Unable to find floor_shader");
			return;
		}
		floor_shader->Bind();
		floor_shader->SetUniform("u_V", Camera::get().view());
		floor_shader->SetUniform("u_P", Camera::get().projection());
		glBindVertexArray(empty_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);


		// Bind shader and VAO, then draw triangle
		if (!m_shader) {
			PN_CORE_ERROR("Unable to find m_shaders");
			return;
		}
		m_shader->Bind();

		//glm::mat4 mvp = Camera::get().projection() * Camera::get().view() * Camera::get().model();

		m_shader->SetUniform("u_M", Camera::get().model());
		m_shader->SetUniform("u_V", Camera::get().view());
		m_shader->SetUniform("u_P", Camera::get().projection());

		m_shader->SetUniform("material.rough", material.rough);
		m_shader->SetUniform("material.metal", material.metal);
		m_shader->SetUniform("material.color", material.color);

		glUniform3f(glGetUniformLocation(m_shader->GetRendererID(), "light[0].position"), light.position.x, light.position.y, light.position.z);
		glUniform3f(glGetUniformLocation(m_shader->GetRendererID(), "light[0].L"), light.L_intensity.x, light.L_intensity.y, light.L_intensity.z);

		if (m_mesh) m_mesh->Draw();

		// render sphere (for light pos)
		if (!sphere_shader) {
			PN_CORE_ERROR("Unable to find sphere_shader");
			return;
		}
		sphere_shader->Bind();

		// build lightsource model
		glm::mat4 model = glm::mat4(1.f);
		model = glm::translate(model, light.position);
		model = glm::scale(model, glm::vec3(0.1f));

		// reuse uniforms
		sphere_shader->SetUniform("u_M", model);
		sphere_shader->SetUniform("u_V", Camera::get().view());
		sphere_shader->SetUniform("u_P", Camera::get().projection());

		sphere_shader->SetUniform("material.rough", material.rough);
		sphere_shader->SetUniform("material.metal", material.metal);
		sphere_shader->SetUniform("material.color", material.color);

		sphere_shader->SetUniform("light[0].position", light.position);
		sphere_shader->SetUniform("light[0].L", light.L_intensity);

		sphere_shader->SetUniform("u_SphereCenter", light.position);
		sphere_shader->SetUniform("u_SphereRadius", 0.1f);

		if (m_mesh) m_mesh->Draw();
	}


	void WindowsRenderer::Cleanup() {
		if (vao != 0) {
			glDeleteVertexArrays(1, &vao);
			vao = 0;
		}

		if (empty_vao != 0) {
			glDeleteVertexArrays(1, &empty_vao);
			empty_vao = 0;
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

		if (m_mesh) {
			m_mesh.reset();
		}


	}
}

#endif // PN_PLATFORM_WINDOWS