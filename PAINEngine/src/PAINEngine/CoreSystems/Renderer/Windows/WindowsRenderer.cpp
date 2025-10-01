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

	//Light light = {
	//	{2.f, 3.f, 2.f},	// position
	//	{0.2f, 0.2f, 0.2f},					// intensity
	//	Light::ORBIT_ORIGIN
	//};

	WindowsRenderer::WindowsRenderer() {

	}

	WindowsRenderer::~WindowsRenderer() {
		Cleanup();
	}

	void WindowsRenderer::_initDeferredShadingBuffers() {
		PN_CORE_INFO("Initializing deferred shading buffers with size: {}x{}", winWidth, winHeight);

		if (winWidth == 0 || winHeight == 0) {
			PN_CORE_ERROR("Invalid window dimensions: {}x{}", winWidth, winHeight);
			return;
		}

		// fbo/texture for deferred shading
		// !TODO: resize when window resizes

		{
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

			glGenTextures(1, &material_properties_texture);
			glBindTexture(GL_TEXTURE_2D, material_properties_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, winWidth, winHeight, 0, GL_RG, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, material_properties_texture, 0);

			unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
			glDrawBuffers(4, attachments);

			glGenRenderbuffers(1, &rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, winWidth, winHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("G-buffer FBO is incomplete! Status: 0x{:x}", status);
				return;
			}
			PN_CORE_INFO("G-buffer FBO is complete");
		}

		// final fbo/texture(final output, for rendering)

		{
			glGenFramebuffers(1, &final_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);

			glGenTextures(1, &final_texture);
			if (final_texture == 0) {
				PN_CORE_ERROR("Failed to create final texture");
				return;
			}
			glBindTexture(GL_TEXTURE_2D, final_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, final_texture, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// vao/vbo for final passthrough texture

		{
			static constexpr float quadVertices[] = {
				// positions    // texCoords
				-1.0f,  1.0f,   0.0f, 1.0f,
				-1.0f, -1.0f,   0.0f, 0.0f,
				 1.0f, -1.0f,   1.0f, 0.0f,
				-1.0f,  1.0f,   0.0f, 1.0f,
				 1.0f, -1.0f,   1.0f, 0.0f,
				 1.0f,  1.0f,   1.0f, 1.0f
			};

			glGenVertexArrays(1, &passthrough_vao);
			glGenBuffers(1, &passthrough_vbo);

			glBindVertexArray(passthrough_vao);
			glBindBuffer(GL_ARRAY_BUFFER, passthrough_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

			glBindVertexArray(0);
		}
	}

	void WindowsRenderer::Init() {
#ifdef PN_PLATFORM_WINDOWS
		pbr_shader = Shader::LoadShaders("pbr.vert", "pbr.frag");
#else
		pbr_shader = Shader::LoadShaders("android_pbr.vert", "android_pbr.frag");
#endif

		if (!pbr_shader || pbr_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}
		else {
			PN_CORE_INFO("Successfully linked shader");
		}
#ifdef PN_PLATFORM_WINDOWS
		geometry_shader = Shader::LoadShaders("geometry.vert", "geometry.frag");
#else
		geometry_shader = Shader::LoadShaders("android_geometry.vert", "android_geometry.frag");
#endif

		if (!geometry_shader || geometry_shader->GetRendererID() == 0) {
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

#ifdef PN_PLATFORM_WINDOWS
		passthrough_shader = Shader::LoadShaders("texture.vert", "texture.frag");
#else
		passthrough_shader = Shader::LoadShaders("android_texture.vert", "android_texture.frag");
#endif

		glGenVertexArrays(1, &empty_vao);
		if (empty_vao == 0) {
			PN_CORE_ERROR("Failed to create empty VAO");
			return;
		}

		_initDeferredShadingBuffers();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		m_mesh = Mesh::LoadObj();

		// init light source(s)

		LightSources::get().create("cam");
		auto olcam = LightSources::get().get("cam");
		Light& lcam = olcam.value();
		lcam.L_intensity = glm::vec3(0.2f);

		LightSources::get().create("a");
		auto ola = LightSources::get().get("a");
		Light& la = ola.value();
		la.position = glm::vec3(2.f, 3.f, 2.f);
		la.L_intensity = glm::vec3(0.1f);

		LightSources::get().create("b");
		auto olb = LightSources::get().get("b");
		Light& lb = olb.value();
		lb.position = glm::vec3(-4.f, 4.f, -5.f);
		lb.L_intensity = glm::vec3(0.1f);
	}

	void WindowsRenderer::Render(std::shared_ptr<Scene> scene) {
		glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// draw floor
		{
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
		}

		// render scene
		if (scene)
		{
			for (auto& obj : scene->GetObjects())
			{
				RenderMesh(obj.mesh, obj.transform); // uses geometry_shader
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);

		//glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);
		glClear(GL_COLOR_BUFFER_BIT);

		//#ifdef JS_DEBUG
		//		passthrough_shader->Bind();
		//		glActiveTexture(GL_TEXTURE0);
		//		glBindTexture(GL_TEXTURE_2D, col_texture);
		//		passthrough_shader->SetUniform("tex", 0);
		//#else
				// render to final framebuffer for post processing/imgui/display

		{
			// pbr pass

			pbr_shader->Bind();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, pos_texture);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, col_texture);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, norm_texture);

			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, material_properties_texture);

			pbr_shader->SetUniform("gPos", 0);
			pbr_shader->SetUniform("gCol", 1);
			pbr_shader->SetUniform("gNorm", 2);
			pbr_shader->SetUniform("gMaterial", 3);
			pbr_shader->SetUniform("u_V", Camera::get().view());
			pbr_shader->SetUniform("u_NumLights", LightSources::get().getCount() * 1.f);
			pbr_shader->SetUniform("u_AmbientLight", LightSources::get().AMBIENT_LIGHT);

			int i{};
			for (const Light& l : LightSources::get().getAll()) {
				std::stringstream ss;

				ss << "u_Lights[" << i << "].position";
				pbr_shader->SetUniform(ss.str(), l.position);
				ss.str("");
				ss.clear();

				if (LightSources::get().lightsOn) {
					ss << "u_Lights[" << i << "].L";
					pbr_shader->SetUniform(ss.str(), l.L_intensity);
					ss.str("");
					ss.clear();
				}

				i++;
			}

			//auto ol = LightSources::get().get("a");
			//Light& l = ol.value();
			//pbr_shader->SetUniform("u_Lights[0].position", l.position);
			//pbr_shader->SetUniform("u_Lights[0].L", l.L_intensity);

			//#endif

			glBindVertexArray(passthrough_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		//{
		//	/* this block is for debug tracing. print color texture(buffer) straight to screen */

		//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//	passthrough_shader->Bind();

		//	glActiveTexture(GL_TEXTURE0);
		//	glBindTexture(GL_TEXTURE_2D, col_texture);

		//	glBindVertexArray(passthrough_vao);
		//	glDrawArrays(GL_TRIANGLES, 0, 6);

		//	return;
		//}

		// render to actual screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		passthrough_shader->Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, final_texture);

		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
	}

	void WindowsRenderer::RenderMesh(Mesh* mesh, const glm::mat4& model)
	{
		if (!mesh || !geometry_shader) return;

		geometry_shader->Bind();

		geometry_shader->SetUniform("u_M", model);
		geometry_shader->SetUniform("u_V", Camera::get().view());
		geometry_shader->SetUniform("u_P", Camera::get().projection());

		geometry_shader->SetUniform("material.rough", material.rough);
		geometry_shader->SetUniform("material.metal", material.metal);
		geometry_shader->SetUniform("material.color", material.color);

		mesh->Draw();
	}

	//void WindowsRenderer::RenderScene(std::shared_ptr<Scene> scene)
	//{

	//	for (auto& obj : scene.get()->GetObjects()) {

	//		RenderMesh(obj.mesh, obj.transform);
	//	}
	//}

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

		if (pbr_shader) {
			pbr_shader.reset();
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

		if (material_properties_texture) {
			glDeleteTextures(1, &material_properties_texture);
			material_properties_texture = 0;
		}

		if (final_texture) {
			glDeleteTextures(1, &final_texture);
			final_texture = 0;
		}
	}
}

//#endif // PN_PLATFORM_WINDOWS