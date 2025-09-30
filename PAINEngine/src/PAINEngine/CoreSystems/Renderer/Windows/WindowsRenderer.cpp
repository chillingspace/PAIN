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


 //#ifdef PN_PLATFORM_WINDOWS

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
		{0.2f, 0.2f, 0.2f},					// intensity
		Light::ORBIT_ORIGIN
	};

	WindowsRenderer::WindowsRenderer() {

	}

	WindowsRenderer::~WindowsRenderer() {
		Cleanup();
	}

	void WindowsRenderer::Init() {
#ifdef PN_PLATFORM_WINDOWS
		m_shader = Shader::LoadShaders("pbr.vert", "pbr.frag");
#else
		m_shader = Shader::LoadShaders("android_pbr.vert", "android_pbr.frag");
#endif

		if (!m_shader || m_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}
		else {
			PN_CORE_INFO("Successfully linked shader");
		}
#ifdef PN_PLATFORM_WINDOWS
		sphere_shader = Shader::LoadShaders("sphere.vert", "sphere.frag");
#else
		sphere_shader = Shader::LoadShaders("android_sphere.vert", "android_sphere.frag");
#endif

		if (!sphere_shader || sphere_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}
#ifdef PN_PLATFORM_WINDOWS
		floor_shader = Shader::LoadShaders("floor.vert", "floor.frag");
#else
		floor_shader = Shader::LoadShaders("android_floor.vert", "android_floor.frag");
#endif

		if (!floor_shader || floor_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		glGenVertexArrays(1, &empty_vao);
		if (empty_vao == 0) {
			PN_CORE_ERROR("Failed to create empty VAO");
			return;
		}

		glGenFramebuffers(1, &fbo);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		PN_CORE_INFO("Before loading ogre");
		m_mesh = Mesh::LoadObj("ogre.obj");
		PN_CORE_INFO("After loading ogre");

	}

	void WindowsRenderer::Render() {

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

		m_mesh->Draw();
	}

	void WindowsRenderer::RenderMesh(Mesh* mesh, const glm::mat4& model)
	{
		if (!mesh || !m_shader) return;

		m_shader->Bind();

		m_shader->SetUniform("u_M", model);
		m_shader->SetUniform("u_V", Camera::get().view());
		m_shader->SetUniform("u_P", Camera::get().projection());

		m_shader->SetUniform("material.rough", material.rough);
		m_shader->SetUniform("material.metal", material.metal);
		m_shader->SetUniform("material.color", material.color);

		m_shader->SetUniform("light[0].position", light.position);
		m_shader->SetUniform("light[0].L", light.L_intensity);

		mesh->Draw();
	}

	void WindowsRenderer::Clear() {
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

		if (fbo != 0) {
			glDeleteFramebuffers(1, &fbo);
			fbo = 0;
		}
	}
}

//#endif // PN_PLATFORM_WINDOWS