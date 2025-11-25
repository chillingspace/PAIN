/*****************************************************************//**
 * \file   skybox.cpp
 * \brief
 *
 * \author Lenovo
 * \date   October 2025
 *********************************************************************/


#include "skybox.h"
#include "stb_image.h"
#include "GraphicsSettings.h"
#include "CoreSystems/Scene/Camera.h"

#ifdef PN_PLATFORM_ANDROID
#include "Utility/AndroidFs.h"
#endif

#include "CoreSystems/Windows/Window.h"

namespace PAIN {
	Skybox::Skybox() {
	}

	Skybox::~Skybox() {
	}

	void Skybox::renderCube() {
		static unsigned int cubeVAO = 0;
		static unsigned int cubeVBO = 0;

		if (cubeVAO == 0) {
			float vertices[] = {
				// Back face
				-1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f,  1.0f, -1.0f,
				// Front face
				-1.0f, -1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				-1.0f, -1.0f,  1.0f,
				// Left face
				-1.0f,  1.0f,  1.0f,
				-1.0f,  1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f, -1.0f,
				-1.0f, -1.0f,  1.0f,
				-1.0f,  1.0f,  1.0f,
				// Right face
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f, -1.0f,
				 1.0f, -1.0f, -1.0f,
				 1.0f,  1.0f,  1.0f,
				 1.0f, -1.0f,  1.0f,
				 // Bottom face
				 -1.0f, -1.0f, -1.0f,
				  1.0f, -1.0f, -1.0f,
				  1.0f, -1.0f,  1.0f,
				  1.0f, -1.0f,  1.0f,
				 -1.0f, -1.0f,  1.0f,
				 -1.0f, -1.0f, -1.0f,
				 // Top face
				 -1.0f,  1.0f, -1.0f,
				  1.0f,  1.0f,  1.0f,
				  1.0f,  1.0f, -1.0f,
				  1.0f,  1.0f,  1.0f,
				 -1.0f,  1.0f, -1.0f,
				 -1.0f,  1.0f,  1.0f
			};

			glGenVertexArrays(1, &cubeVAO);
			glGenBuffers(1, &cubeVBO);
			glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
			glBindVertexArray(cubeVAO);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
		}

		glBindVertexArray(cubeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);
	}

	void Skybox::convertEquirectangularToCubemap() {
		glGenTextures(1, &cubemap_tex);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_tex);

		for (unsigned int i = 0; i < 6; ++i) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		//glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef PN_PLATFORM_ANDROID
		// Pre-allocate all mip levels for Android before generating mipmaps
		for (unsigned int mip = 1; mip < 10; ++mip) {
			unsigned int mipSize = static_cast<unsigned int>(512 * std::pow(0.5, mip));
			if (mipSize < 1) break;
			for (unsigned int i = 0; i < 6; ++i) {
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip, GL_RGB16F,
					mipSize, mipSize, 0, GL_RGB, GL_FLOAT, nullptr);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 9);
#endif

		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		unsigned int captureFBO, captureRBO;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			PN_CORE_ERROR("CaptureFBO in cETC not complete!");
		}

		// fov has to be 90 degree fov no matter what user sets
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] = {
			glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

		// Conversion shader setup
		conversionShader->Bind();
		conversionShader->SetUniform("equirectangularMap", 0);
		conversionShader->SetUniform("projection", captureProjection);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, skybox_tex);

		glViewport(0, 0, 512, 512);
		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

		// Render to each face
		for (unsigned int i = 0; i < 6; ++i) {
			conversionShader->SetUniform("view", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap_tex, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			renderCube(); // Render a unit cube
		}
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			PN_CORE_ERROR("Capture Framebuffer not complete!");
		}

#ifdef _DEBUG
		// debug
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, captureFBO);
			glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X, cubemap_tex, 0);
			glReadBuffer(GL_COLOR_ATTACHMENT0);

			GLfloat pixel[4];
			glReadPixels(256, 256, 1, 1, GL_RGBA, GL_FLOAT, pixel); // Center of 512x512
			PN_CORE_INFO("Cubemap center pixel: R={}, G={}, B={}, A={}",
				pixel[0], pixel[1], pixel[2], pixel[3]);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
#endif

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, WindowsRenderer::winWidth, WindowsRenderer::winHeight);
		glDeleteFramebuffers(1, &captureFBO);
		glDeleteRenderbuffers(1, &captureRBO);
		//glDeleteTextures(1, &skybox_tex); // delete the original HDR
	}

	void Skybox::init(const std::shared_ptr<Services>& s) {
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err before Skybox init: {}", err);
		}

		services = s;

		//Set win width and height
		auto window_service = services->get<Window::Window>();
		WindowsRenderer::winWidth = window_service->getFrameBuffer().x;
		WindowsRenderer::winHeight = window_service->getFrameBuffer().y;

		// compile and link shader
		{
#ifdef PN_PLATFORM_ANDROID
			std::filesystem::path eqr_shader_path = "engine\\shaders\\android_eqr_to_skybox.vert";
#else
			std::filesystem::path eqr_shader_path = "engine\\shaders\\eqr_to_skybox.vert";
#endif
			auto conversionShader_opt = services->get<Assets::Manager>()->getAsset<Assets::Shader>(eqr_shader_path);
			conversionShader = conversionShader_opt.has_value() ? conversionShader_opt.value() : conversionShader;
			PN_CORE_INFO("Equirectangular to cubemap shader compiled, ID: {}", conversionShader->GetRendererID());
		}

		// compile and link shader
		{
#ifdef PN_PLATFORM_ANDROID
			std::filesystem::path skybox_shader_path = "engine\\shaders\\android_skybox.vert";
#else
			std::filesystem::path skybox_shader_path = "engine\\shaders\\skybox.vert";
#endif
			auto shader_opt = services->get<Assets::Manager>()->getAsset<Assets::Shader>(skybox_shader_path);
			shader = shader_opt.has_value() ? shader_opt.value() : shader;
			PN_CORE_INFO("Skybox shader compiled, ID: {}", shader->GetRendererID());
		}

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after Skybox init: {}", err);
		}
	}

	void Skybox::setTexture(Assets::GUID const& id) {
		auto sky_box_tex_opt = services->get<Assets::Manager>()->getAsset<Assets::Texture>(id);
		skybox_tex = sky_box_tex_opt.has_value() ? sky_box_tex_opt.value()->gl_texture : skybox_tex;
		convertEquirectangularToCubemap();

		// generate IBL textures
		generateIrradianceMap();
		generatePrefilterMap();
		generateBRDFLUT();

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after Skybox set texture: {}", err);
		}
	}

	void Skybox::setTexture(const std::filesystem::path& skybox_path) {
		auto sky_box_tex_opt = services->get<Assets::Manager>()->getAsset<Assets::Texture>(skybox_path);
		skybox_tex = sky_box_tex_opt.has_value() ? sky_box_tex_opt.value()->gl_texture : skybox_tex;
		convertEquirectangularToCubemap();

		// generate IBL textures
		generateIrradianceMap();
		generatePrefilterMap();
		generateBRDFLUT();

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after Skybox set texture: {}", err);
		}
	}

	void Skybox::render(const glm::mat4& view, const glm::mat4& proj) {
		glDisable(GL_CULL_FACE);	// we are inside cube, so disable culling
		glDepthFunc(GL_LEQUAL);  // so skybox renders at max depth

		shader->Bind();

		// skybox to follow cam
		glm::mat4 viewNoTranslation = glm::mat4(glm::mat3(view));

		shader->SetUniform("view", viewNoTranslation);
		shader->SetUniform("projection", proj);
		shader->SetUniform("skybox", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_tex);

		renderCube();

		glDepthFunc(GL_LESS);  // reset depth function to default
		glEnable(GL_CULL_FACE);
	}

	void Skybox::generateIrradianceMap() {
		GLenum err;

		while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error before generateIrradianceMap: {}", err);

		// Create irradiance cubemap
		glGenTextures(1, &irradiance_map);
		glBindTexture(GL_TEXTURE_CUBE_MAP, irradiance_map);

		// Irradiance map is typically smaller (32x32 is enough)
		for (unsigned int i = 0; i < 6; ++i) {
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
				32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Setup framebuffer
		unsigned int captureFBO, captureRBO;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			PN_CORE_ERROR("CaptureFBO in generateIrradianceMap not complete!");
		}

		// Same projection and views as your existing code
		glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
		glm::mat4 captureViews[] = {
			glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
			glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
		};

#ifdef PN_PLATFORM_ANDROID
		std::filesystem::path irradiance_shader_path = "engine\\shaders\\android_irradiance_convolution.vert";
#else
		std::filesystem::path irradiance_shader_path = "engine\\shaders\\irradiance_convolution.vert";
#endif

		// Load irradiance shader
		auto irradianceShader_opt = services->get<Assets::Manager>()->getAsset<Assets::Shader>(irradiance_shader_path);
		if (irradianceShader_opt.has_value()) {
			auto irradianceShader = irradianceShader_opt.value();

			// Force complete mipmap chain OR disable mipmaps
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_tex);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // NO mipmaps
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// debug
			{
				// Verify texture is complete
				GLint textureComplete;
				glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, &textureComplete);
				PN_CORE_INFO("Cubemap min filter: {}", textureComplete);
			}

			irradianceShader->Bind();
			irradianceShader->SetUniform("environmentMap", 0);
			irradianceShader->SetUniform("projection", captureProjection);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_tex);

			glViewport(0, 0, 32, 32);
			glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

			// debug
			{
				PN_CORE_INFO("cubemap_tex ID: {}", cubemap_tex);
				GLint boundTexture;
				glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &boundTexture);
				PN_CORE_INFO("Bound cubemap before irradiance render: {} (expected: {})", boundTexture, cubemap_tex);

				if (boundTexture != (GLint)cubemap_tex) {
					PN_CORE_ERROR("CUBEMAP NOT BOUND! Rebinding...");
					glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_tex);
				}
			}

			// Render to each cubemap face
			for (unsigned int i = 0; i < 6; ++i) {
				irradianceShader->SetUniform("view", captureViews[i]);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradiance_map, 0);

				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
					PN_CORE_ERROR("CaptureFBO in generateIrradianceMap not complete! side: {}", i);
					GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
					switch (status) {
					case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
						PN_CORE_ERROR("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
					case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
						PN_CORE_ERROR("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
					case GL_FRAMEBUFFER_UNSUPPORTED:
						PN_CORE_ERROR("GL_FRAMEBUFFER_UNSUPPORTED"); break;
					default:
						PN_CORE_ERROR("Unknown FBO error: {}", status); break;
					}
					while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error in glCheckFramebufferStatus: {}", err);
				}

				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				renderCube();
			}

			// debug
			{
				// Read back a pixel from the center of the irradiance map
				glBindFramebuffer(GL_READ_FRAMEBUFFER, captureFBO);
				glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X, irradiance_map, 0);
				glReadBuffer(GL_COLOR_ATTACHMENT0);

				GLfloat pixel[4];
				glReadPixels(16, 16, 1, 1, GL_RGBA, GL_FLOAT, pixel); // Center of 32x32
				PN_CORE_INFO("Irradiance center pixel: R={}, G={}, B={}, A={}",
					pixel[0], pixel[1], pixel[2], pixel[3]);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, WindowsRenderer::winWidth, WindowsRenderer::winHeight);

			glDeleteFramebuffers(1, &captureFBO);

			while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error after generateIrradianceMap: {}", err);

			PN_CORE_INFO("Irradiance map generated, ID: {}", irradiance_map);
		}
	}





	void Skybox::generatePrefilterMap() {
		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error before generatePrefilterMap: {}", err);

		// Create prefiltered cubemap
		glGenTextures(1, &prefilter_map);
		glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map);

		static constexpr int MIP_WIDTH = 512;

		// generate different mipmaps for different roughness
		// 5 mips, meaning {0, 0.25, 0.5, 0.75, 1.0}
		//for (unsigned int i = 0; i < 6; ++i) {
		//	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, MIP_WIDTH, MIP_WIDTH, 0, GL_RGB, GL_FLOAT, nullptr);
		//}
		static constexpr int maxMipLevels = 5;
		glTexStorage2D(GL_TEXTURE_CUBE_MAP, maxMipLevels, GL_RGB16F, MIP_WIDTH, MIP_WIDTH);
		//glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

#ifdef PN_PLATFORM_ANDROID
		std::filesystem::path prefilter_shader_path = "engine\\shaders\\android_prefilter.vert";
#else
		std::filesystem::path prefilter_shader_path = "engine\\shaders\\prefilter.vert";
#endif

		// Load prefilter shader
		auto prefilterShader_opt = services->get<Assets::Manager>()->getAsset<Assets::Shader>(prefilter_shader_path);
		if (prefilterShader_opt.has_value()) {
			auto prefilterShader = prefilterShader_opt.value();

			// Setup projection and views (same as before)
			glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
			glm::mat4 captureViews[] = {
				glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
				glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
				glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
				glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
			};

			prefilterShader->Bind();
			prefilterShader->SetUniform("environmentMap", 0);
			prefilterShader->SetUniform("projection", captureProjection);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap_tex);

			unsigned int captureFBO;
			glGenFramebuffers(1, &captureFBO);
			glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

			while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error before rendering to mipmap: {}", err);

			// Render for each mip level (each roughness level)
			for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
			{
				// Resize framebuffer according to mip-level size
				unsigned int mipWidth = static_cast<unsigned int>(MIP_WIDTH * std::pow(0.5, mip));



				glViewport(0, 0, mipWidth, mipWidth);

				float roughness = (float)mip / (float)(maxMipLevels - 1);
				prefilterShader->SetUniform("roughness", roughness);

				for (unsigned int i = 0; i < 6; ++i)
				{
					prefilterShader->SetUniform("view", captureViews[i]);

					glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilter_map, mip);

					if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
						PN_CORE_ERROR("CaptureFBO in generatePrefilterMap not complete! mip: {}, side: {}", mip, i);
						GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
						switch (status) {
						case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
							PN_CORE_ERROR("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
						case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
							PN_CORE_ERROR("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
						case GL_FRAMEBUFFER_UNSUPPORTED:
							PN_CORE_ERROR("GL_FRAMEBUFFER_UNSUPPORTED"); break;
						default:
							PN_CORE_ERROR("Unknown FBO error: {}", status); break;
						}
						while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error in glCheckFramebufferStatus: {}", err);
					}

					glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

					while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error before renderCube in generatePrefilterMap: {}", err);
					renderCube();
					while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error after renderCube in generatePrefilterMap: {}", err);
				}
			}

			glBindTexture(GL_TEXTURE_CUBE_MAP, prefilter_map);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 4);

			// debug
			{
				// Read back a pixel from the center of the irradiance map
				glBindFramebuffer(GL_READ_FRAMEBUFFER, captureFBO);
				glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
					GL_TEXTURE_CUBE_MAP_POSITIVE_X, prefilter_map, 0);
				glReadBuffer(GL_COLOR_ATTACHMENT0);

				GLfloat pixel[4];
				glReadPixels(16, 16, 1, 1, GL_RGBA, GL_FLOAT, pixel); // Center of 32x32
				PN_CORE_INFO("Prefilter center pixel: R={}, G={}, B={}, A={}",
					pixel[0], pixel[1], pixel[2], pixel[3]);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, WindowsRenderer::winWidth, WindowsRenderer::winHeight);

			glDeleteFramebuffers(1, &captureFBO);

			PN_CORE_INFO("Prefiltered map generated, ID: {}", prefilter_map);
		}
	}



	void Skybox::generateBRDFLUT() {
		PN_CORE_INFO("Generating BRDFLUT..");

		GLenum err;
		while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error before generateBRDFLUT: {}", err);

		// Create BRDF LUT texture
		glGenTextures(1, &brdf_tex);
		glBindTexture(GL_TEXTURE_2D, brdf_tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error after generating texture: {}", err);

		// Setup framebuffer
		unsigned int captureFBO;
		glGenFramebuffers(1, &captureFBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		//#ifdef PN_PLATFORM_ANDROID
		//		glGenRenderbuffers(1, &captureRBO);
		//		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		//		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		//#endif
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdf_tex, 0);

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			PN_CORE_ERROR("CaptureFBO in generateBRDFLUT not complete!");
		}

		while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error after generating fbo: {}", err);

#ifdef PN_PLATFORM_ANDROID
		std::filesystem::path brdf_shader_path = "engine\\shaders\\android_brdf.vert";
#else
		std::filesystem::path brdf_shader_path = "engine\\shaders\\brdf.vert";
#endif


		// Load BRDF shader
		auto brdfShader_opt = services->get<Assets::Manager>()->getAsset<Assets::Shader>(brdf_shader_path);
		if (brdfShader_opt.has_value()) {
			auto brdfShader = brdfShader_opt.value();

			glViewport(0, 0, 512, 512);
			brdfShader->Bind();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error before renderQuad: {}", err);

			renderQuad(); // Render a fullscreen quad (see below)

			while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error after renderQuad: {}", err);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, WindowsRenderer::winWidth, WindowsRenderer::winHeight);

			glDeleteFramebuffers(1, &captureFBO);
			//glDeleteRenderbuffers(1, &captureRBO);

			PN_CORE_INFO("BRDF LUT generated, ID: {}", brdf_tex);
			while ((err = glGetError()) != GL_NO_ERROR) PN_CORE_ERROR("OpenGL error at the end of generateBRDFLUT: {}", err);
		}
	}




	void Skybox::renderQuad() {
		static unsigned int quadVAO = 0;
		static unsigned int quadVBO = 0;

		if (quadVAO == 0) {
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};

			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}

		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
	}
}
