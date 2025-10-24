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

#include "WindowsRenderer.h"
#include "CoreSystems/Renderer/texture.h"


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

	// TO BE MOVED
	std::string WindowsRenderer::ReadFile(const std::filesystem::path& path)
	{
		std::ifstream file(path);
		if (!file.is_open()) {
			PN_CORE_WARN("Failed to open shader file: {}", path.string());
			assert(0);
		}
		std::stringstream buffer;
		buffer << file.rdbuf();

		return buffer.str();
	}

	// TO BE MOVED 
	std::unique_ptr<Shader> WindowsRenderer::LoadShaders(const std::string& vert_file, const std::string& frag_file)
	{
		PN_CORE_INFO("Compiling shaders {0}, {1}", vert_file, frag_file);

#ifdef PN_PLATFORM_WINDOWS
		// Get current working directory and build paths from there
		std::filesystem::path current_path = std::filesystem::current_path();
		std::filesystem::path project_root = current_path / "PAIN"; // Adjust as needed

		// Or try to find the project root by looking for a marker file
		std::filesystem::path search_path = current_path;
		while (search_path.has_parent_path()) {
			if (std::filesystem::exists(search_path / "PAIN" / "assets")) {
				project_root = search_path / "PAIN";
				break;
			}
			search_path = search_path.parent_path();
		}
		
		//Get path service
		auto path_service = services->get<Path::Path>();

		std::filesystem::path vert_full = path_service->resolvePath("engine_assets://Shaders/" + vert_file);
		std::filesystem::path frag_full = path_service->resolvePath("engine_assets://Shaders/" + frag_file);

		PN_CORE_INFO("Using paths: {0}, {1}", vert_full.string(), frag_full.string());
		std::string vert_code = ReadFile(vert_full);
		PN_CORE_INFO("Successfully read vertex shader");
		std::string frag_code = ReadFile(frag_full);
		PN_CORE_INFO("Successfully read fragment shader");
#else
		PN_CORE_INFO("Using Android asset manager for shaders");

        //Get path service
        auto path_service = services->get<Path::Path>();
        auto vert_path = path_service->resolvePath("engine_assets://Shaders/" + vert_file);
        auto frag_path = path_service->resolvePath("engine_assets://Shaders/" + frag_file);
		std::string vert_code = ReadFileAndroid(vert_path);
		std::string frag_code = ReadFileAndroid(frag_path);
#endif

		return std::make_unique<Shader>(vert_code, frag_code);
	}

	void WindowsRenderer::initShaders()
	{
#ifdef PN_PLATFORM_WINDOWS
		pbr_shader = LoadShaders("pbr.vert", "pbr.frag");
#else
		pbr_shader = LoadShaders("android_pbr.vert", "android_pbr.frag");
#endif

		if (!pbr_shader || pbr_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}
		else {
			PN_CORE_INFO("Successfully linked shader");
		}
#ifdef PN_PLATFORM_WINDOWS
		geometry_shader = LoadShaders("geometry.vert", "geometry.frag");
#else
		geometry_shader = LoadShaders("android_geometry.vert", "android_geometry.frag");
#endif

		if (!geometry_shader || geometry_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}
#ifdef PN_PLATFORM_WINDOWS
		floor_shader = LoadShaders("floor.vert", "floor.frag");
#else
		floor_shader = LoadShaders("android_floor.vert", "android_floor.frag");
#endif

		if (!floor_shader || floor_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

#ifdef PN_PLATFORM_WINDOWS
		passthrough_shader = LoadShaders("passthrough.vert", "passthrough.frag");
#else
		passthrough_shader = LoadShaders("android_passthrough.vert", "android_passthrough.frag");
#endif

#ifdef PN_PLATFORM_WINDOWS
		shadow_shader = LoadShaders("shadow.vert", "shadow.frag");
#else
		shadow_shader = LoadShaders("android_shadow.vert", "android_shadow.frag");
#endif

#ifdef PN_PLATFORM_WINDOWS
		texture2d_shader = LoadShaders("texture2d.vert", "texture2d.frag");
#else
		texture2d_shader = LoadShaders("android_texture2d.vert", "android_texture2d.frag");
#endif
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

		// fbo/texture for deferred shading
		// !TODO: resize when window resizes

		{
			glGenFramebuffers(1, &ds_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);

			_createDeferredShadingBuffer(pos_texture, 3, GL_COLOR_ATTACHMENT0);
			_createDeferredShadingBuffer(col_texture, 3, GL_COLOR_ATTACHMENT1);
			_createDeferredShadingBuffer(norm_texture, 3, GL_COLOR_ATTACHMENT2);
			_createDeferredShadingBuffer(material_properties_texture, 2, GL_COLOR_ATTACHMENT3);

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
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

		// vao/vbo for geometry shader

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
	}

	void WindowsRenderer::Init(std::shared_ptr<Services> app_services) {
		services = app_services;
		
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

	void WindowsRenderer::Render2DTexture(const std::string& ref, const glm::vec2& pos, float scale) {
		const auto texmap = TextureManager::get().getTextureMap();
		if (texmap.find(ref) == texmap.end()) {
			PN_CORE_ERROR("Texture with ref '{}' not found!", ref);
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
		glBindTexture(GL_TEXTURE_2D, texmap.at(ref));
		texture2d_shader->SetUniform("tex", 6);
		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindVertexArray(0);
	}

	void WindowsRenderer::BeginRendering(std::shared_ptr<Scene> scene)
	{
		glViewport(0, 0, winWidth, winHeight);

		// Setup framebuffers, clear buffers
		glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void WindowsRenderer::EndRendering(std::shared_ptr<Scene> scene)
	{
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

			int tex_id = 4;
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
			pbr_shader->SetUniform("u_NumShadowMaps", (tex_id - 4) * 1.f);

			pbr_shader->SetUniform("gPos", 0);
			pbr_shader->SetUniform("gCol", 1);
			pbr_shader->SetUniform("gNorm", 2);
			pbr_shader->SetUniform("gMaterial", 3);
			pbr_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
			pbr_shader->SetUniform("u_NumLights", LightSources::get().getCount() * 1.f);
			pbr_shader->SetUniform("u_AmbientLight", LightSources::get().AMBIENT_LIGHT);

			//#endif

			glBindVertexArray(passthrough_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		// render 2D textures onto screen

		{
			// !TODO: add queue and iterate through all 2D textures to be rendered last
			Render2DTexture("sunshine", { 0.85f, -0.85f }, 0.1f);
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
/*
	void WindowsRenderer::Init(std::shared_ptr<Services> app_services) {
		services = app_services;
#ifdef PN_PLATFORM_WINDOWS
		pbr_shader = LoadShaders("pbr.vert", "pbr.frag");
#else
		pbr_shader = LoadShaders("android_pbr.vert", "android_pbr.frag");
#endif

		if (!pbr_shader || pbr_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}
		else {
			PN_CORE_INFO("Successfully linked shader");
		}
#ifdef PN_PLATFORM_WINDOWS
		geometry_shader = LoadShaders("geometry.vert", "geometry.frag");
#else
		geometry_shader = LoadShaders("android_geometry.vert", "android_geometry.frag");
#endif

		if (!geometry_shader || geometry_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}
#ifdef PN_PLATFORM_WINDOWS
		floor_shader = LoadShaders("floor.vert", "floor.frag");
#else
		floor_shader = LoadShaders("android_floor.vert", "android_floor.frag");
#endif

		if (!floor_shader || floor_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program");
			return;
		}

#ifdef PN_PLATFORM_WINDOWS
		passthrough_shader = LoadShaders("passthrough.vert", "passthrough.frag");
#else
		passthrough_shader = LoadShaders("android_passthrough.vert", "android_passthrough.frag");
#endif

#ifdef PN_PLATFORM_WINDOWS
		shadow_shader = LoadShaders("shadow.vert", "shadow.frag");
#else
		shadow_shader = LoadShaders("android_shadow.vert", "android_shadow.frag");
#endif

#ifdef PN_PLATFORM_WINDOWS
		texture_shader = LoadShaders("texture.vert", "texture.frag");
#else
		texture_shader = LoadShaders("android_texture.vert", "android_texture.frag");
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
		
		// init light source(s)

		LightSources::get().create("cam");
		auto olcam = LightSources::get().get("cam");
		Light& lcam = olcam.value();
		lcam.L_intensity = glm::vec3(0.01f);
		//lcam.setShadowType(Light::SHADOW_TYPES::MAPPED);

		LightSources::get().create("a");
		auto ola = LightSources::get().get("a");
		Light& la = ola.value();
		la.position = glm::vec3(4.f, 4.f, -8.f);
		la.L_intensity = glm::vec3(0.2f);

		LightSources::get().create("b");
		auto olb = LightSources::get().get("b");
		Light& lb = olb.value();
		lb.position = glm::vec3(-4.f, 4.f, -8.f);
		lb.L_intensity = glm::vec3(0.2f);

		LightSources::get().create("c");
		auto olc = LightSources::get().get("c");
		Light& lc = olc.value();
		lc.position = glm::vec3(0.f, 30.f, 0.f);
		lc.forward = -glm::normalize(lc.position);		// point at origin for dir light
		lc.L_intensity = glm::vec3(0.5f);
		lc.setShadowType(Light::SHADOW_TYPES::MAPPED);
		lc.type = Light::TYPES::DIRECTIONAL;
		//lc.far_plane = 200.f;
		//lc.forward = -lc.position;
	}
*/

	void WindowsRenderer::RenderGeometry(std::shared_ptr<Scene> scene, Mesh* mesh, const glm::mat4& model)
	{
		if (!mesh || !geometry_shader) return;

		geometry_shader->Bind();

		geometry_shader->SetUniform("u_M", model);
		geometry_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
		geometry_shader->SetUniform("u_P", scene->GetActiveCamera()->projection());

		geometry_shader->SetUniform("material.rough", mesh->material.rough);
		geometry_shader->SetUniform("material.metal", mesh->material.metal);
		geometry_shader->SetUniform("material.color", mesh->material.color);
		geometry_shader->SetUniform("material.useTex", mesh->material.useTex ? 1.f : 0.f);

		if (mesh->material.useTex) {
			glActiveTexture(GL_TEXTURE6);
			glBindTexture(GL_TEXTURE_2D, mesh->texture_id);
			geometry_shader->SetUniform("material.tex", 6);
		}

		mesh->Draw(geometry_vao, geometry_vbo, geometry_ebo);
	}

	void WindowsRenderer::RenderGeometryShadows(Mesh* mesh, const glm::mat4& model, const Light& light) {
		if (!mesh || !shadow_shader) return;

		shadow_shader->Bind();

		shadow_shader->SetUniform("u_M", model);
		shadow_shader->SetUniform("u_V", light.view());
		shadow_shader->SetUniform("u_P", light.projection());

		mesh->Draw(geometry_vao, geometry_vbo, geometry_ebo);
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