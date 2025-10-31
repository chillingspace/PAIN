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

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#endif
#include "stb_image.h"

namespace PAIN {
	Skybox::Skybox() {
	}

	Skybox::~Skybox() {
	}

	void Skybox::loadHdr(const std::string& path) {
		stbi_set_flip_vertically_on_load(true);
		int width, height, nrComponents;
#ifdef PN_PLATFORM_ANDROID
		std::string fileData = ReadFileAndroid(path);
		if (fileData.empty()) {
			PN_CORE_ERROR("Failed to load HDR asset: {}", path);
			return;
		}

		float* data = stbi_loadf_from_memory(
			reinterpret_cast<const unsigned char*>(fileData.data()),
			fileData.size(),
			&width, &height, &nrComponents, 0
		);
#else
		float* data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
#endif
		if (data) {
			PN_CORE_INFO("Loaded HDR image with size: {}x{} and {} components", width, height, nrComponents);

			glGenTextures(1, &skybox_tex);
			glBindTexture(GL_TEXTURE_2D, skybox_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);
		}
		else {
			PN_CORE_ERROR("Failed to load HDR image at path: {}", path);
		}
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
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		unsigned int captureFBO, captureRBO;
		glGenFramebuffers(1, &captureFBO);
		glGenRenderbuffers(1, &captureRBO);

		glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

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


		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, winWidth, winHeight);
		glDeleteFramebuffers(1, &captureFBO);
		glDeleteRenderbuffers(1, &captureRBO);
		glDeleteTextures(1, &skybox_tex); // delete the original HDR
	}

	void Skybox::init(const std::shared_ptr<Services>& s, const std::string& skybox_path) {
		services = s;

		//Set win width and height
		auto window_service = services->get<Window::Window>();
		winWidth = window_service->getFrameBuffer().x;
		winHeight = window_service->getFrameBuffer().y;

		// compile and link shader
		{
#ifdef PN_PLATFORM_ANDROID
			std::filesystem::path eqr_shader_path = "engine\\shaders\\android_eqr_to_skybox.vert";
#else
			std::filesystem::path eqr_shader_path = "engine\\shaders\\eqr_to_skybox.vert";
#endif

			conversionShader = services->get<Assets::Manager>()->getAsset<Assets::Shader>(eqr_shader_path);
			PN_CORE_INFO("Equirectangular to cubemap shader compiled, ID: {}", conversionShader->GetRendererID());
		}

		loadHdr(skybox_path);
		convertEquirectangularToCubemap();

		// compile and link shader
		{
#ifdef PN_PLATFORM_ANDROID
			std::filesystem::path skybox_shader_path = "engine\\shaders\\android_skybox.vert";
#else
			std::filesystem::path skybox_shader_path = "engine\\shaders\\skybox.vert";
#endif
			shader = services->get<Assets::Manager>()->getAsset<Assets::Shader>(skybox_shader_path);
			PN_CORE_INFO("Skybox shader compiled, ID: {}", shader->GetRendererID());
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
}
