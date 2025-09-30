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

		// fbo/texture for deferred shading
		// !TODO: resize when window resizes

		glGenFramebuffers(1, &ds_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);

		glGenTextures(1, &pos_texture);
		if (pos_texture == 0) {
			PN_CORE_ERROR("Failed to create position texture");
			return;
		}
		glBindTexture(GL_TEXTURE_2D, pos_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, winWidth, winHeight, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pos_texture, 0);

		glGenTextures(1, &col_texture);
		if (col_texture == 0) {
			PN_CORE_ERROR("Failed to create color texture");
			return;
		}
		glBindTexture(GL_TEXTURE_2D, col_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, winWidth, winHeight, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, col_texture, 0);

		glGenTextures(1, &norm_texture);
		if (norm_texture == 0) {
			PN_CORE_ERROR("Failed to create normal texture");
			return;
		}
		glBindTexture(GL_TEXTURE_2D, norm_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, winWidth, winHeight, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, norm_texture, 0);

		unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
		glDrawBuffers(3, attachments);

		glGenRenderbuffers(1, &rbo);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, winWidth, winHeight);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

		// final fbo/texture(final output, for rendering)

		glGenFramebuffers(1, &final_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);

		glGenTextures(1, &final_texture);
		if (final_texture == 0) {
			PN_CORE_ERROR("Failed to create final texture");
			return;
		}
		glBindTexture(GL_TEXTURE_2D, final_texture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, winWidth, winHeight, 0, GL_RGB, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, final_texture, 0);

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

		if (ds_fbo != 0) {
			glDeleteFramebuffers(1, &ds_fbo);
			ds_fbo = 0;
		}

		if (final_fbo) {
			glDeleteFramebuffers(1, &final_fbo);
			final_fbo = 0;
		}

		if (rbo) {
			glDeleteRenderbuffers(1, &rbo);
			rbo = 0;
		}

		if (pos_texture) {
			glDeleteTextures(1, &pos_texture);
			pos_texture = 0;
		}

		if (col_texture) {
			glDeleteTextures(1, &col_texture);
			col_texture = 0;
		}

		if (norm_texture) {
			glDeleteTextures(1, &norm_texture);
			col_texture = 0;
		}
	}
}

//#endif // PN_PLATFORM_WINDOWS