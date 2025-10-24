/**
 * @file WindowsRenderer.h
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2025-09-27
 *
 * @copyright Copyright (c) 2025
 *
 */


#pragma once

#ifndef __WINDOWS_RENDERER_H__
#define __WINDOWS_RENDERER_H__

#include "pch.h"
#include "../Shader.h"
#include "../Mesh.h"

#include "../Light.h"
#include "../Material.h"

#include "CoreSystems/Scene/Scene.h"
#include "CoreSystems/Scene/Camera.h"
#include "CoreSystems/Path/Path.h"

namespace PAIN {
	extern Material material;
};

namespace PAIN {

	class WindowsRenderer {

	public:
		static constexpr float ao = 1.f;		// ambient occlusion	(1 = no occlusion)

		WindowsRenderer();
		~WindowsRenderer();

		void Init(std::shared_ptr<Services> app_services);

		void BeginRendering(std::shared_ptr<Scene> scene);
		void EndRendering(std::shared_ptr<Scene> scene);

		void RenderGeometry(std::shared_ptr<Scene> scene, Mesh* mesh, const glm::mat4& model);
		void RenderGeometryShadows(Mesh* mesh, const glm::mat4& model, const Light& light);

		void Render2DTexture(const std::string& ref, const glm::vec2& pos, float scale);

		void Cleanup();

		unsigned int getFinalFbo() const {
			return final_fbo;
		}

		unsigned int getFinalTexture() const {
			return final_texture;
		}

		static constexpr int MAX_VERTICES = 1000000;
		static constexpr int MAX_INDICES = 1000000;

	private:
		/*
				unsigned int empty_vao = 0;

				unsigned int passthrough_vao = 0;
				unsigned int passthrough_vbo = 0;
		*/
		unsigned int ds_fbo = 0;			// deferred shading framebuffer
		unsigned int ds_rbo = 0;				// depth buffer
		//unsigned int shadow_fbo = 0;
		unsigned int final_fbo = 0;

		// === Textures ===
		unsigned int pos_texture = 0;
		unsigned int col_texture = 0;
		unsigned int norm_texture = 0;
		//unsigned int shadow_texture = 0;					// shadow map
		unsigned int material_properties_texture = 0;		// 2D to store roughness, metallic properties

		// === Geometry Buffers ===
		unsigned int geometry_vao = 0;
		unsigned int geometry_vbo = 0;
		unsigned int geometry_ebo = 0;
		unsigned int empty_vao = 0;
		unsigned int passthrough_vao = 0;
		unsigned int passthrough_vbo = 0;

		unsigned int final_texture = 0;		// for imgui/post-processing/display

		// === Shaders ===
		std::unique_ptr<Shader> pbr_shader = nullptr;
		std::unique_ptr<Shader> geometry_shader = nullptr;
		std::unique_ptr<Shader> floor_shader = nullptr;
		std::unique_ptr<Shader> passthrough_shader = nullptr;
		std::unique_ptr<Shader> shadow_shader = nullptr;
		std::unique_ptr<Camera> active_cam = nullptr;		// what is this for? -js
		std::unique_ptr<Shader> texture2d_shader = nullptr;

		// for easy access to clear memory
		std::array<unsigned int*, 2> fbos{
			&ds_fbo,
			//&shadow_fbo, 
			&final_fbo
		};
		std::array<unsigned int*, 1> rbos{ &ds_rbo };
		std::array<unsigned int*, 4> texs{
			&pos_texture,
			&col_texture,
			&norm_texture,
			&material_properties_texture,
			//&shadow_texture
		};

		/*
		std::unique_ptr<Shader> pbr_shader = nullptr;
		std::unique_ptr<Shader> geometry_shader = nullptr;
		std::unique_ptr<Shader> floor_shader = nullptr;
		std::unique_ptr<Shader> passthrough_shader = nullptr;
		std::unique_ptr<Shader> shadow_shader = nullptr;
		std::unique_ptr<Shader> texture_shader = nullptr;
		*/

		std::shared_ptr<Services> services;

		std::string ReadFile(const std::filesystem::path& path);
		std::unique_ptr<Shader> LoadShaders(const std::string& vert_file, const std::string& frag_file);

		void initShaders();

		/**
		 * .
		 *
		 * \param tex
		 * \param num_i channels
		 * \param gl_color_attachment THIS IS NOT YOUR NORMAL ID. USE GL_ATTACHMENT`n` HERE.
		 */
		void _createDeferredShadingBuffer(unsigned int& tex, int num_channels, int gl_color_attachment);
		void _initDeferredShadingBuffers();
	};
}


#endif // PN_PLATFORM_WINDFOWS
