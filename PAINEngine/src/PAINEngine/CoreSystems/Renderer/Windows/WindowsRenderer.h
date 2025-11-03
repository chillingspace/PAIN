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
#include "../Mesh.h"

#include "../Light.h"
#include "../Material.h"

#include "CoreSystems/Scene/Scene.h"
#include "CoreSystems/Scene/Camera.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Assets/sAssets.h"

namespace PAIN {
	extern Material material;
};

namespace PAIN {

	class WindowsRenderer {

	public:
		static constexpr float ao = 1.f;		// ambient occlusion	(1 = no occlusion)

		int winWidth = 0;
		int winHeight = 0;

		WindowsRenderer();
		~WindowsRenderer();

		void Init(std::shared_ptr<Services> app_services);

		// PASSES
		void BeginShadowPass(const Light& l);
		void DrawShadows(Mesh* mesh, const glm::mat4& M, const Light& l);
		void EndShadowPass();

		void BeginGeometryPass(std::shared_ptr<Scene> scene);
		void DrawGeometry(std::shared_ptr<Scene> scene, Mesh* mesh, const glm::mat4& M);
		void EndGeometryPass();


		void ReflectionPass(const std::shared_ptr<Mesh>& m);
		void LightingPass(std::shared_ptr<Scene> scene, const LightSources& lights);
		void DebugPass(const glm::vec3& min_p, const glm::vec3& max_p, const glm::vec4& color, std::shared_ptr<Scene> scene);
		void PostProcessPass();

		void Render2DTexture(GLuint texture_id, const glm::vec2& pos, float scale);

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
		unsigned int final_rbo = 0;

		unsigned int pp_fbo = 0;			// post-processing framebuffer (for ping-pong)
		unsigned int out_fbo = 0;			// output framebuffer (for imgui/display)

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
		unsigned int pp_texture = 0;	// for ping-pong for post-processing

		// === Debug Buffers ===
		unsigned int debug_VAO = 0;
		unsigned int debug_VBO = 0;

		// === Shaders ===
		std::shared_ptr<Assets::Shader> pbr_shader = nullptr;
		std::shared_ptr<Assets::Shader> geometry_shader = nullptr;
		std::shared_ptr<Assets::Shader> floor_shader = nullptr;
		std::shared_ptr<Assets::Shader> passthrough_shader = nullptr;
		std::shared_ptr<Assets::Shader> shadow_shader = nullptr;
		std::shared_ptr<Assets::Shader> texture2d_shader = nullptr;
		std::shared_ptr<Assets::Shader> debug_shader = nullptr;
		std::shared_ptr<Assets::Shader> gamma_shader = nullptr;
		std::shared_ptr<Assets::Shader> blur_shader = nullptr;
		std::shared_ptr<Assets::Shader> bloom_shader = nullptr;
		std::shared_ptr<Assets::Shader> tone_shader = nullptr;

		// for easy access to clear memory
		std::array<unsigned int*, 3> fbos{
			&ds_fbo,
			//&shadow_fbo, 
			&final_fbo,
			&pp_fbo,
		};
		std::array<unsigned int*, 2> rbos{ &ds_rbo, &final_rbo };
		std::array<unsigned int*, 6> texs{
			&pos_texture,
			&col_texture,
			&norm_texture,
			&material_properties_texture,
			//&shadow_texture,
			&final_texture,
			&pp_texture
		};

		std::shared_ptr<Services> services;
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
