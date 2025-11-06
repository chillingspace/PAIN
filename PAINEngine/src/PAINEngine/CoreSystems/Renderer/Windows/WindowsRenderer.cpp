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
#include "CoreSystems/Assets/sAssets.h"
#include "WindowsRenderer.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Renderer/skybox.h"

#include "CoreSystems/Windows/Window.h"



namespace PAIN {
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

	void WindowsRenderer::initShaders()
	{

		//Identify all paths
#ifdef PN_PLATFORM_WINDOWS
		std::filesystem::path pbr_path = "engine/shaders/pbr.vert";
		std::filesystem::path geometry_path = "engine/shaders/geometry.vert";
		std::filesystem::path floor_path = "engine/shaders/floor.vert";
		std::filesystem::path passthrough_path = "engine/shaders/passthrough.vert";
		std::filesystem::path shadow_path = "engine/shaders/shadow.vert";
		std::filesystem::path texture2d_path = "engine/shaders/texture2d.vert";
		std::filesystem::path gamma_path = "engine/shaders/gamma.vert";
		std::filesystem::path debug_geometry_path = "engine/shaders/debug_geometry.vert";
		std::filesystem::path blur_path = "engine/shaders/blur.vert";
		std::filesystem::path bloom_path = "engine/shaders/bloom.vert";
		std::filesystem::path bloom_blend_path = "engine/shaders/bloom_blend.vert";
		std::filesystem::path tone_path = "engine/shaders/tone.vert";
#else	
		std::filesystem::path pbr_path = "engine\\shaders\\android_pbr.vert";
		std::filesystem::path geometry_path = "engine\\shaders\\android_geometry.vert";
		std::filesystem::path floor_path = "engine\\shaders\\android_floor.vert";
		std::filesystem::path passthrough_path = "engine\\shaders\\android_passthrough.vert";
		std::filesystem::path shadow_path = "engine\\shaders\\android_shadow.vert";
		std::filesystem::path texture2d_path = "engine\\shaders\\android_texture2d.vert";
		std::filesystem::path gamma_path = "engine\\shaders\\android_gamma.vert";
		std::filesystem::path debug_geometry_path = "engine\\shaders\\android_debug_geometry.vert";
		std::filesystem::path blur_path = "engine\\shaders\\android_blur.vert";
		std::filesystem::path bloom_path = "engine\\shaders\\android_bloom.vert";
		std::filesystem::path bloom_blend_path = "engine\\shaders\\android_bloom_blend.vert";
		std::filesystem::path tone_path = "engine\\shaders\\android_tone.vert";
#endif

		//Get assets loader
		auto assets_loader = services->get<Assets::Manager>();

		//PBR Shader
		pbr_shader = assets_loader->getAsset<Assets::Shader>(pbr_path);

		if (!pbr_shader || pbr_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//Geometry shader
		geometry_shader = assets_loader->getAsset<Assets::Shader>(geometry_path);

		if (!geometry_shader || geometry_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//FLoor shader
		floor_shader = assets_loader->getAsset<Assets::Shader>(floor_path);

		if (!floor_shader || floor_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//Pass through shader
		passthrough_shader = assets_loader->getAsset<Assets::Shader>(passthrough_path);

		if (!passthrough_shader || passthrough_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//Shadow shader
		shadow_shader = assets_loader->getAsset<Assets::Shader>(shadow_path);

		if (!shadow_shader || shadow_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//Texture shader
		texture2d_shader = assets_loader->getAsset<Assets::Shader>(texture2d_path);

		if (!texture2d_shader || texture2d_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//Tone mapping shader
		tone_shader = assets_loader->getAsset<Assets::Shader>(tone_path);
		if (!tone_shader || tone_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//Bloom shader
		bloom_shader = assets_loader->getAsset<Assets::Shader>(bloom_path);
		if (!bloom_shader || bloom_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		// Bloom blend shader
		bloom_blend_shader = assets_loader->getAsset<Assets::Shader>(bloom_blend_path);
		if (!bloom_blend_shader || bloom_blend_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		// Blur shader
		blur_shader = assets_loader->getAsset<Assets::Shader>(blur_path);
		if (!blur_shader || blur_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//Gamma shader
		gamma_shader = assets_loader->getAsset<Assets::Shader>(gamma_path);

		if (!gamma_shader || gamma_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

		//Debug shader
		debug_shader = assets_loader->getAsset<Assets::Shader>(debug_geometry_path);

		if (!debug_shader || debug_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}
	}

	// TO BE MOVED
	void WindowsRenderer::_createDeferredShadingBuffer(unsigned int& tex, int num_channels, int gl_color_attachment) {
		glGenTextures(1, &tex);
		if (!tex) {
			PN_CORE_ERROR("Failed to gen texture");
			return;
		}

		glBindTexture(GL_TEXTURE_2D, tex);

		switch (num_channels) {
		case 2:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, winWidth, winHeight, 0, GL_RG, GL_FLOAT, nullptr);
			break;
		case 3:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, winWidth, winHeight, 0, GL_RGB, GL_FLOAT, nullptr);
			break;
		case 4:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
			break;
		default:
			PN_CORE_ERROR("{} channels isn't supported(by me lol not opengl so need to add)!", num_channels);
			break;
		};
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);		// DO NOT USE GL_LINEAR
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, gl_color_attachment, GL_TEXTURE_2D, tex, 0);
	}

	// TO BE MOVED
	void WindowsRenderer::_initDeferredShadingBuffers() {
		PN_CORE_INFO("Initializing deferred shading buffers with size: {}x{}", winWidth, winHeight);

		if (winWidth == 0 || winHeight == 0) {
			PN_CORE_ERROR("Invalid window dimensions: {}x{}", winWidth, winHeight);
			return;
		}

		// === Final FBO/Texture For Deffered Shading ===
		// !TODO: resize when window resizes
		{
			glGenFramebuffers(1, &ds_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);

			_createDeferredShadingBuffer(pos_texture, 3, GL_COLOR_ATTACHMENT0);
			_createDeferredShadingBuffer(col_texture, 3, GL_COLOR_ATTACHMENT1);
			_createDeferredShadingBuffer(norm_texture, 3, GL_COLOR_ATTACHMENT2);
			_createDeferredShadingBuffer(material_properties_texture, 3, GL_COLOR_ATTACHMENT3);

			unsigned int attachments[4] = {
				GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2,
				GL_COLOR_ATTACHMENT3,
			};
			glDrawBuffers(4, attachments);

			glGenRenderbuffers(1, &ds_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, ds_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, winWidth, winHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ds_rbo);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("G-buffer FBO is incomplete! Status: 0x{:x}", status);
				return;
			}
			PN_CORE_INFO("G-buffer FBO is complete");

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// === Final VAO/Texture (final output, for rendering) ===
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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, final_texture, 0);

			glGenRenderbuffers(1, &final_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, final_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, winWidth, winHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, final_rbo);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// pp_texture for ping-pong if needed in post-processing
			glGenFramebuffers(1, &pp_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, pp_fbo);

			glGenTextures(1, &pp_texture);
			if (pp_texture == 0) {
				PN_CORE_ERROR("Failed to create final texture");
				return;
			}
			glBindTexture(GL_TEXTURE_2D, pp_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pp_texture, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// pp2_texture for ping-pong if needed in post-processing
			glGenFramebuffers(1, &pp2_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, pp2_fbo);

			glGenTextures(1, &pp2_texture);
			if (pp2_texture == 0) {
				PN_CORE_ERROR("Failed to create final texture");
				return;
			}
			glBindTexture(GL_TEXTURE_2D, pp2_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pp2_texture, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// === VAO/VBO For Final Passthrough Texture ===
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

		// === VAO/VBO For Geometry Shaders ===
		{
			// Generate and bind VAO
			glGenVertexArrays(1, &geometry_vao);
			glBindVertexArray(geometry_vao);

			// Generate and bind VBO
			glGenBuffers(1, &geometry_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, geometry_vbo);
			glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

			// Generate and bind EBO (index buffer)
			glGenBuffers(1, &geometry_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry_ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(unsigned int), nullptr, GL_DYNAMIC_DRAW);

			// Position attribute, layout(location = 0)
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
			glEnableVertexAttribArray(0);

			// Normal attribute, layout(location = 1)
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
			glEnableVertexAttribArray(1);

			// texcoords attribute, layout(location = 2)
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
			glEnableVertexAttribArray(2);

			// Unbind VAO
			glBindVertexArray(0);
		}


		// === VAO/VBO For Debug Shaders ===
		{
			// Generate and bind VAO
			glGenVertexArrays(1, &debug_VAO);
			glBindVertexArray(debug_VAO);


			// Generate and bind VBO
			glGenBuffers(1, &debug_VBO);
			glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
			glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * 7 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

			// Position attribute, layout(location = 0)
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);

			// Color attribute, layout(location = 1)
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
			glBindVertexArray(0);
		}
	}

	void WindowsRenderer::Init(std::shared_ptr<Services> app_services) {
		services = app_services;

		//Set win width and height
		auto window_service = services->get<Window::Window>();
		winWidth = window_service->getFrameBuffer().x;
		winHeight = window_service->getFrameBuffer().y;

		initShaders();

		// fallback or placeholder VAO to avoid OpenGL errors 
		glGenVertexArrays(1, &empty_vao);
		if (empty_vao == 0) {
			PN_CORE_ERROR("Failed to create empty VAO");
			return;
		}

		_initDeferredShadingBuffers();

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

	}

	void WindowsRenderer::Render2DTexture(GLuint texture_id, const glm::vec2& pos, float scale) {
		if (texture_id == 0) {
			PN_CORE_ERROR("Invalid texture_id in Render2DTexture");
			return;
		}

		if (!texture2d_shader) {
			PN_CORE_ERROR("Unable to find texture2d_shader");
			return;
		}

		texture2d_shader->Bind();
		texture2d_shader->SetUniform("pos", pos);
		texture2d_shader->SetUniform("ndc_scale", scale);

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		texture2d_shader->SetUniform("tex", 6);
		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindVertexArray(0);
	}

	void WindowsRenderer::BeginShadowPass(const Light& l)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, l.getShadowFbo());
		//glClearDepth(1.0f);  // Explicitly set clear value

#ifdef PN_PLATFORM_ANDROID
		// critical for Mali GPU on android
		// disable color writes
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
#endif

		glClear(GL_DEPTH_BUFFER_BIT);

	}

	void WindowsRenderer::DrawShadows(Mesh* mesh, const glm::mat4& M, const Light& l)
	{
		if (!mesh || !shadow_shader) return;

		shadow_shader->Bind();

		shadow_shader->SetUniform("u_M", M);
		shadow_shader->SetUniform("u_V", l.view());
		shadow_shader->SetUniform("u_P", l.projection());

		mesh->Draw(geometry_vao, geometry_vbo, geometry_ebo);
	}

	void WindowsRenderer::EndShadowPass()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef PN_PLATFORM_ANDROID
		// critical for Mali GPU on android
		// reenable color writes
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif
	}

	void WindowsRenderer::BeginGeometryPass(std::shared_ptr<Scene> scene)
	{
		glViewport(0, 0, winWidth, winHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//GLenum err = glGetError();
		//if (err != GL_NO_ERROR) {
		//	PN_CORE_ERROR("OpenGL error before drawing skybox: {}", err);
		//}

		//// draw skybox
		//{
		//	Skybox::get().render(scene->GetActiveCamera()->view(), scene->GetActiveCamera()->projection());
		//}
		//err = glGetError();
		//if (err != GL_NO_ERROR) {
		//	PN_CORE_ERROR("OpenGL error after drawing skybox: {}", err);
		//}

		// draw floor
		{
			if (!floor_shader) {
				PN_CORE_ERROR("Unable to find floor_shader");
				return;
			}


			floor_shader->Bind();
			floor_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
			floor_shader->SetUniform("u_P", scene->GetActiveCamera()->projection());
			glBindVertexArray(empty_vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);
		}

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error after drawing floor: {}", err);
		}

		geometry_shader->Bind();
		geometry_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
		geometry_shader->SetUniform("u_P", scene->GetActiveCamera()->projection());

	}

	void WindowsRenderer::DrawGeometry(std::shared_ptr<Scene> scene, Mesh* mesh, const glm::mat4& M)
	{
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error before DrawGeometry: {}", err);
		}

		if (!mesh || !geometry_shader) return;

		geometry_shader->SetUniform("u_M", M);

		geometry_shader->SetUniform("material.rough", mesh->material.rough);
		geometry_shader->SetUniform("material.metal", mesh->material.metal);
		geometry_shader->SetUniform("material.color", mesh->material.color);
		geometry_shader->SetUniform("material.useTex", mesh->material.useTex ? 1.f : 0.f);
		geometry_shader->SetUniform("material.alwaysLit", mesh->material.alwaysLit ? 1.f : 0.f);

		if (mesh->material.useTex) {
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, mesh->material.tex);
			geometry_shader->SetUniform("material.tex", 6);

			if (GraphicsSettings::get().ao && mesh->material.useAo) {
				glActiveTexture(GL_TEXTURE7);
				glBindTexture(GL_TEXTURE_2D, mesh->material.aoTex);
				geometry_shader->SetUniform("material.ao_map", 7);
				geometry_shader->SetUniform("material.use_ao", 1.f);
			}
			else {
				geometry_shader->SetUniform("material.use_ao", 0.f);
			}
		}

		mesh->Draw(geometry_vao, geometry_vbo, geometry_ebo);

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error after DrawGeometry: {}", err);
		}
	}

	void WindowsRenderer::EndGeometryPass()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void WindowsRenderer::ReflectionPass(const std::shared_ptr<Mesh>& m)
	{
		if (m->material.reflection_type == m->material.REFLECTION_TYPES::NONE) {
			return;
		}


	}

	void WindowsRenderer::LightingPass(std::shared_ptr<Scene> scene, const LightSources& lights)
	{
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

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err before lighting pass: {}", err);
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

			// lighting shouldnt write to depth buffer
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);

			pbr_shader->Bind();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, pos_texture);

			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, col_texture);

			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, norm_texture);

			glActiveTexture(GL_TEXTURE3);
			glBindTexture(GL_TEXTURE_2D, material_properties_texture);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after binding gbuffer textures: {}", err);
			}

			int tex_id = 7;
			int i{};
			for (const Light& l : LightSources::get().getAll()) {
				std::stringstream ss;
				if (l.getShadowType() == Light::SHADOW_TYPES::MAPPED) {
					glActiveTexture(GL_TEXTURE0 + tex_id);
					glBindTexture(GL_TEXTURE_2D, l.getShadowTexture());

#ifdef PN_PLATFORM_WINDOWS
					ss << "u_ShadowMaps[" << (tex_id - 4) << "]";
#else
					ss << "u_ShadowMap" << (tex_id - 4);
#endif

					pbr_shader->SetUniform(ss.str(), tex_id);
					ss.str("");
					ss.clear();

					ss << "u_Lights[" << i << "].shadowMapIdx";
					pbr_shader->SetUniform(ss.str(), tex_id - 4.f);
					ss.str("");
					ss.clear();

					++tex_id;
				}
				else {
					ss << "u_Lights[" << i << "].shadowMapIdx";
					pbr_shader->SetUniform(ss.str(), -1.f);
					ss.str("");
					ss.clear();
				}

				ss << "u_Lights[" << i << "].position";
				pbr_shader->SetUniform(ss.str(), l.position);
				ss.str("");
				ss.clear();

				ss << "u_Lights[" << i << "].V";
				pbr_shader->SetUniform(ss.str(), l.view());
				ss.str("");
				ss.clear();

				ss << "u_Lights[" << i << "].P";
				pbr_shader->SetUniform(ss.str(), l.projection());
				ss.str("");
				ss.clear();

				ss << "u_Lights[" << i << "].type";
				pbr_shader->SetUniform(ss.str(), static_cast<float>(l.type));
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

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after setting light uniforms: {}", err);
			}

			pbr_shader->SetUniform("u_NumShadowMaps", (tex_id - 4) * 1.f);

			pbr_shader->SetUniform("gPos", 0);
			pbr_shader->SetUniform("gCol", 1);
			pbr_shader->SetUniform("gNorm", 2);
			pbr_shader->SetUniform("gMaterial", 3);
			pbr_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
			pbr_shader->SetUniform("u_NumLights", LightSources::get().getCount() * 1.f);
			pbr_shader->SetUniform("u_AmbientLight", LightSources::get().AMBIENT_LIGHT);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after setting lighting pbr uniforms: {}", err);
			}

			// for image based lighting
			pbr_shader->SetUniform("u_CamPos", scene->GetActiveCamera()->pos);
			pbr_shader->SetUniform("u_UseIbl", GraphicsSettings::get().ibl ? 1.f : 0.f);

			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox::get().getIrradianceMap());
			pbr_shader->SetUniform("irradianceMap", 4);

			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox::get().getPrefilterMap());
			pbr_shader->SetUniform("prefilterMap", 5);

			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, Skybox::get().getBrdfLUT());
			pbr_shader->SetUniform("brdfLut", 6);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after setting ibl uniforms: {}", err);
			}


			//#endif

			glBindVertexArray(passthrough_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after drawing lighting pass: {}", err);
			}
		}

		glEnable(GL_DEPTH_TEST);

		// After lighting pass, final_fbo has the lit scene but NO depth buffer yet
		// So we copy it:
		glBindFramebuffer(GL_READ_FRAMEBUFFER, ds_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, final_fbo);
		glBlitFramebuffer(0, 0, winWidth, winHeight,
			0, 0, winWidth, winHeight,
			GL_DEPTH_BUFFER_BIT, GL_NEAREST);  // Copy depth only

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after blitting depth buffer: {}", err);
		}

		// Now final_fbo has depth info. Render skybox:
		{
			glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
			glDepthFunc(GL_LEQUAL);  // Pass if depth <= existing depth
			glDepthMask(GL_FALSE);   // Don't write to depth buffer

			Skybox::get().render(scene->GetActiveCamera()->view(), scene->GetActiveCamera()->projection());

			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
		}

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after drawing skybox in lighting pass: {}", err);
		}

	}


	void WindowsRenderer::DebugPass(const glm::vec3& min_p, const glm::vec3& max_p, const glm::vec4& color, std::shared_ptr<Scene> scene)
	{
		if (!debug_VAO || !debug_shader) return;

		std::vector<float> verts; verts.reserve(24 * 7);

		// converts min/max into 8 corners,
		glm::vec3 v[8] = {
		  {min_p.x,min_p.y,min_p.z},{max_p.x,min_p.y,min_p.z},
		  {max_p.x,max_p.y,min_p.z},{min_p.x,max_p.y,min_p.z},
		  {min_p.x,min_p.y,max_p.z},{max_p.x,min_p.y,max_p.z},
		  {max_p.x,max_p.y,max_p.z},{min_p.x,max_p.y,max_p.z}
		};

		// edge index list (tells which pairs of the 8 AABB corners should be connected to form the 12 box edges)
		int e[24] = { 0,1,1,2,2,3,3,0, 4,5,5,6,6,7,7,4, 0,4,1,5,2,6,3,7 };

		// xyz and rgba
		auto push = [&](const glm::vec3& p, const glm::vec4& c) {
			verts.insert(verts.end(), { p.x,p.y,p.z, c.r,c.g,c.b,c.a });
			};

		// The loop iterates over all 12 edges by stepping i += 2, takes the two endpoint corners for each edge via v[e[i]] and v[e[i+1]], and pushes both endpoints
		for (int i = 0; i < 24; i += 2) {
			push(v[e[i]], color);
			push(v[e[i + 1]], color);
		}

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		debug_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
		debug_shader->SetUniform("u_P", scene->GetActiveCamera()->projection());

		glDrawArrays(GL_LINES, 0, 24);

		glDepthMask(GL_TRUE);
		glBindVertexArray(0);
	}


	void WindowsRenderer::PostProcessPass()
	{
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err before tone mapping pass: {}", err);
		}


		int postprocess_passes = 0;

		// tone mapping pass
		{
			const unsigned int dest_fbo = postprocess_passes % 2 == 0 ? pp_fbo : final_fbo;
			const unsigned int src_tex = postprocess_passes % 2 == 0 ? final_texture : pp_texture;

			glCheck(glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo));
			tone_shader->Bind();
			glCheck(glActiveTexture(GL_TEXTURE0));
			glCheck(glBindTexture(GL_TEXTURE_2D, src_tex));
			glCheck(tone_shader->SetUniform("tex", 0));
			glCheck(tone_shader->SetUniform("exposure", GraphicsSettings::get().tone_mapping_exposure));
			glCheck(tone_shader->SetUniform("toneMapMode", static_cast<float>(GraphicsSettings::get().tone_mapping_mode)));
			glCheck(glBindVertexArray(empty_vao));
			glCheck(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
			++postprocess_passes;
		}
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after tone mapping pass: {}", err);
		}

		// bloom pass
		if (GraphicsSettings::get().bloom) {
			// save scene_tex to final_texture first
			if (postprocess_passes % 2) {
				glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
				passthrough_shader->Bind();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, pp_texture);
				passthrough_shader->SetUniform("tex", 0);
				glBindVertexArray(passthrough_vao);
				glDrawArrays(GL_TRIANGLES, 0, 6);
			}

			// extract bright areas with bloom_shader
			{
				const unsigned int dest_fbo = pp_fbo;
				const unsigned int src_tex = final_texture;

				glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);
				bloom_shader->Bind();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, src_tex);
				bloom_shader->SetUniform("tex", 0);
				bloom_shader->SetUniform("threshold", GraphicsSettings::get().bloom_threshold);
				glBindVertexArray(empty_vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}

			// reset postprocess_passes for bloom blur
			postprocess_passes = 0;

			// blur bright areas above threshold
			{

				blur_shader->Bind();
				blur_shader->SetUniform("tex", 0);
				blur_shader->SetUniform("strength", GraphicsSettings::get().bloom_blur_strength);

				// on i = 0, bright areas are in pp_texture

				for (int i{}; i < GraphicsSettings::get().bloom_quality; ++i) {
					const unsigned int dest_fbo = postprocess_passes % 2 == 0 ? pp2_fbo : pp_fbo;
					const unsigned int src_tex = postprocess_passes % 2 == 0 ? pp_texture : pp2_texture;

					glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, src_tex);
					blur_shader->SetUniform("is_horizontal_pass", i % 2 ? 0.f : 1.f);
					glBindVertexArray(empty_vao);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					++postprocess_passes;
				}
			}

			// as blur pass will always be an even number (istg if yall put odd yall trolling me),
			// final blurred bright will be in pp_texture

			// add blurred bright areas back to original image
			{
				const unsigned int dest_fbo = pp2_fbo;
				const unsigned int bloom_tex = pp_texture;

				glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);
				bloom_blend_shader->Bind();

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, final_texture);
				bloom_blend_shader->SetUniform("scene_tex", 0);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, bloom_tex);
				bloom_blend_shader->SetUniform("bloom_tex", 1);

				bloom_blend_shader->SetUniform("bloom_strength", GraphicsSettings::get().bloom_strength);

				glBindVertexArray(empty_vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}

			// set back to final_texture
			{
				const unsigned int src_tex = pp2_texture;

				glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
				passthrough_shader->Bind();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, src_tex);
				passthrough_shader->SetUniform("tex", 0);
				glBindVertexArray(passthrough_vao);
				glDrawArrays(GL_TRIANGLES, 0, 6);

				// reset postprocess_passes
				postprocess_passes = 0;
			}
		}


		// blur pass
		if (GraphicsSettings::get().blur_strength) {
			const int blur_iterations = GraphicsSettings::get().blur_quality;

			blur_shader->Bind();
			blur_shader->SetUniform("tex", 0);
			blur_shader->SetUniform("strength", GraphicsSettings::get().blur_strength);

			for (int i{}; i < blur_iterations; ++i) {
				const unsigned int dest_fbo = postprocess_passes % 2 == 0 ? pp_fbo : final_fbo;
				const unsigned int src_tex = postprocess_passes % 2 == 0 ? final_texture : pp_texture;

				glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, src_tex);
				blur_shader->SetUniform("is_horizontal_pass", i % 2 ? 0.f : 1.f);
				glBindVertexArray(empty_vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				++postprocess_passes;
			}
		}
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after blur pass: {}", err);
		}


		// gamma correction
		if (GraphicsSettings::get().gamma_correction) {
			const unsigned int dest_fbo = postprocess_passes % 2 == 0 ? pp_fbo : final_fbo;
			const unsigned int src_tex = postprocess_passes % 2 == 0 ? final_texture : pp_texture;

			glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);
			gamma_shader->Bind();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, src_tex);
			gamma_shader->SetUniform("tex", 0);
			glBindVertexArray(empty_vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			++postprocess_passes;
		}

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after gamma pass: {}", err);
		}

		// make sure final_texture now holds the gamma corrected texture
		// use passthrough to render pp_texture to final_texture if odd number of passes
		if (postprocess_passes % 2) {
			glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
			passthrough_shader->Bind();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, pp_texture);
			passthrough_shader->SetUniform("tex", 0);
			glBindVertexArray(passthrough_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after finalizing post process pass: {}", err);
		}

		// set back to use final_fbo and final_texture for further rendering
		;		glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
		//glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, final_texture, 0);

		{
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);

			// render 2D textures onto screen
			{
				// !TODO: add queue and iterate through all 2D textures to be rendered last
				auto texture = services->get<Assets::Manager>()->getAsset<Assets::Texture>(Assets::GUID("796cf7f1-0fe5-234b-b1a8-a602d3da43dc"));
				Render2DTexture(texture->gl_texture, { 0.85f, -0.85f }, 0.1f);
			}
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after Render2DTexture in PostProcessPass: {}", err);
			}

			// render text onto screen
			{
				TextRenderer::get().renderText("Pantat", 100.f, 100.f, 1.f, { 1.f, 1.f, 1.f });
				TextRenderer::get().debugRenderQuad();
			}
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after TextRenderer in PostProcessPass: {}", err);
			}

			glEnable(GL_DEPTH_TEST);
		}

		// render to actual screen
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		passthrough_shader->Bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, final_texture);
		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after PostProcessPass: {}", err);
		}
	}

	void WindowsRenderer::Cleanup() {
		if (geometry_vao != 0) {
			glDeleteVertexArrays(1, &geometry_vao);
			geometry_vao = 0;
		}

		if (empty_vao != 0) {
			glDeleteVertexArrays(1, &empty_vao);
			empty_vao = 0;
		}

		if (geometry_vbo != 0) {
			glDeleteBuffers(1, &geometry_vbo);
			geometry_vbo = 0;
		}

		if (geometry_ebo != 0) {
			glDeleteBuffers(1, &geometry_ebo);
			geometry_ebo = 0;
		}

		if (debug_VAO) {
			glDeleteVertexArrays(1, &debug_VAO);
			debug_VAO = 0;
		}

		if (debug_VBO) {
			glDeleteBuffers(1, &debug_VBO);
			debug_VBO = 0;
		}

		if (pbr_shader) {
			pbr_shader.reset();
		}

		for (unsigned int* fbo : fbos) {
			if (*fbo != 0) {
				glDeleteFramebuffers(1, fbo);
				*fbo = 0;
			}
		}

		for (unsigned int* tex : texs) {
			if (*tex) {
				glDeleteTextures(1, tex);
				*tex = 0;
			}
		}

		for (unsigned int* rbo : rbos) {
			if (*rbo) {
				glDeleteRenderbuffers(1, rbo);
				*rbo = 0;
			}
		}
	}
}

//#endif // PN_PLATFORM_WINDOWS