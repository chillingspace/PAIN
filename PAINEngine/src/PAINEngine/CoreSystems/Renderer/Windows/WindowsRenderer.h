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

#include "../Mesh.h"
#include "pch.h"


#include "../Light.h"
#include "../Material.h"

#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Collision/BoundingVolume.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Scene/Camera.h"
#include "CoreSystems/Scene/Scene.h"


namespace PAIN {
	extern Material material;
};

namespace PAIN {

	class WindowsRenderer {
	private:
		static constexpr int MAX_VOLUMETRIC_LIGHTS = 4;

		// for instanced rendering

		// scene vbo stuff
		struct SceneVboOffset {
			unsigned int idx_offset{};
			unsigned int idx_count{};
		};
		unsigned int currentVertexCount = 0;
		unsigned int currentIndexCount = 0;

		// OPTIMIZATION: CPU-side caches to avoid GPU read-back during buffer resizing
		// These store all uploaded vertices/indices so we can re-upload without stalling the GPU
		std::vector<Assets::Vertex> cpu_vertex_cache;
		std::vector<unsigned int> cpu_index_cache;
		bool cpu_cache_valid = false;

		std::unordered_map<std::string, SceneVboOffset> instanced_offsets{};

		struct IBOData {
			glm::mat4 model_xform;

			float base_rough;
			float base_metal;
			glm::vec3 base_color;

			bool use_tex;
			bool use_ao;
			bool use_normal;
			bool use_roughnessmetallic;
			bool use_emissive;
			bool is_animating;
		};

		// for batching materials
		struct MaterialKey {
			unsigned int albedoTexture = 0;
			unsigned int normalTexture = 0;
			unsigned int metallicTexture = 0;
			unsigned int roughnessTexture = 0;
			unsigned int aoTexture = 0;
			unsigned int emissiveTexture = 0;
			unsigned int heightTexture = 0;
			unsigned int opacityTexture = 0;

			bool operator==(const MaterialKey& mk) {
				return mk.albedoTexture == albedoTexture &&
					   mk.normalTexture == normalTexture &&
					   mk.metallicTexture == metallicTexture &&
					   mk.roughnessTexture == roughnessTexture &&
					   mk.aoTexture == aoTexture &&
					   mk.emissiveTexture == emissiveTexture &&
					   mk.heightTexture == heightTexture &&
					   mk.opacityTexture == opacityTexture;
			}

			bool operator!=(const MaterialKey& mk) {
				return !(*this == mk);
			}
		};

	public:
		static constexpr float ao = 1.f; // ambient occlusion	(1 = no occlusion)

		// vbo data for instanced rendering
		bool vboDirty = false;
		bool needsFullRebuild = false;
		bool resizeDirty = false;

		inline static int winWidth = 0;
		inline static int winHeight = 0;

		WindowsRenderer();
		~WindowsRenderer();

		void Init(std::shared_ptr<Services> app_services);
		void uploadTexture(std::shared_ptr<Assets::Texture> tex);
		void initSceneVbo();
		bool hasUploadedModel(const std::string& modelVPath) const {
			return instanced_offsets.find(modelVPath) != instanced_offsets.end();
		}
		void _initDeferredShadingBuffers();
		void _initGeometryBuffers();
		void clearBuffers();

		// PASSES
		void BeginShadowPass(const Light& l, bool clearDepth = true);
		void DrawShadows(const ModelRenderer& component, const glm::mat4& M,
						 const Light& l);
		void EndShadowPass();

		void BeginGeometryPass(std::shared_ptr<Scene::SceneManager> scene);
		void DrawGeometry(std::shared_ptr<Scene::SceneManager> scene,
						  ModelRenderer& component, const glm::mat4& M);
		// Draws multiple instances of the same model/submesh combo. Material uniforms
		// are taken from `component` (first instance's data). All matrices uploaded at once.
		void DrawGeometryInstanced(std::shared_ptr<Scene::SceneManager> scene,
								   ModelRenderer& component,
								   const std::vector<glm::mat4>& matrices);
		void EndGeometryPass();

		void BeginMinimapPass(const glm::mat4& view, const glm::mat4& proj);
		void EndMinimapPass();

		// === GPU-accelerated minimap wall rendering ===
		// Upload wall XZ data once (call only when walls change)
		void UploadMinimapWalls(const std::vector<glm::vec2>& worldXZVertices);
		// Draw walls using GPU shader (call every frame with updated uniforms)
		void DrawMinimapWalls(const glm::vec2& playerXZ,
							 const glm::vec2& transformCol0,
							 const glm::vec2& transformCol1,
							 const glm::vec2& invDoubleRadius,
							 const glm::vec2& ndcBase,
							 const glm::vec2& ndcScale,
							 const glm::vec4& color,
							 const glm::vec4& accentColor,
							 float patternStrength,
							 float patternScale,
							 float patternPhase);
		bool hasMinimapWallData() const { return minimap_wall_vertex_count > 0; }
		bool canDrawMinimapWalls() const { return minimap_wall_shader != nullptr && minimap_wall_vertex_count > 0; }

		void ReflectionPass(const ModelRenderer& component);
		void LightingPass(std::shared_ptr<Scene::SceneManager> scene,
						  const LightSources& lights);
		void DebugPass(const glm::vec3& min_p, const glm::vec3& max_p,
					   const glm::vec4& color,
					   std::shared_ptr<Scene::SceneManager> scene);
		void DebugPassOBB(const glm::vec3 corners[8], const glm::vec4& color,
						  std::shared_ptr<Scene::SceneManager> scene);
		void DebugPass2D(const glm::vec2& min_p, const glm::vec2& max_p,
						 const glm::vec4& color);
		void DebugPass2DLine(const glm::vec2& start_p, const glm::vec2& end_p,
							const glm::vec4& color);
		void DebugPass2DLines(const std::vector<glm::vec2>& lineVertices,
							 const glm::vec4& color);
		void DebugPass2DTriangleFilled(const glm::vec2& a,
							 const glm::vec2& b,
							 const glm::vec2& c,
							 const glm::vec4& color);
		void DebugPass2DTrianglesFilled(const std::vector<glm::vec2>& triangleVertices,
							  const glm::vec4& color);
		void DebugPass2DCircle(const glm::vec2& center_p, const glm::vec2& radius_ndc,
							  const glm::vec4& color, int segments = 32);
		void VolumetricPass(std::shared_ptr<Scene::SceneManager> scene,
						   const LightSources& lights);
		void PostProcessPass(bool presentToSwapchain);
		void BlitFinalToScreen();  // Simple blit without post-processing
		void InvalidateFramebufferAttachments(GLuint fbo, bool invalidateColor = false,
											 bool invalidateDepth = false,
											 bool invalidateStencil = false);

		void Render2DTexture(GLuint texture_id, const glm::vec2& pos,
							 glm::vec2& scale,
							 const glm::vec4& uv_transform = glm::vec4(1.0f, 1.0f,
																	   0.0f, 0.0f));
		void Render2DTextureCircular(GLuint texture_id, const glm::vec2& pos,
							 glm::vec2& scale,
							 const glm::vec4& uv_transform = glm::vec4(1.0f, 1.0f,
																	   0.0f, 0.0f));

		// Setup stencil buffer for circular clipping of subsequent draws
		void BeginCircularStencilClip(const glm::vec2& center_ndc, const glm::vec2& radius_ndc);
		void EndCircularStencilClip();

		void Cleanup();

		unsigned int getFinalFbo() const {
			return final_fbo;
		}

		unsigned int getFinalTexture() const {
			return final_texture;
		}

		unsigned int getMinimapTexture() const {
			return minimap_texture;
		}

		static constexpr int MAX_VERTICES = 1000000;
		static constexpr int MAX_INDICES = MAX_VERTICES;

	  private:
		/*
                  unsigned int empty_vao = 0;

                  unsigned int passthrough_vao = 0;
                  unsigned int passthrough_vbo = 0;
  */
		unsigned int ds_fbo = 0; // deferred shading framebuffer
		unsigned int ds_depth_texture = 0;
		// unsigned int shadow_fbo = 0;
		unsigned int final_fbo = 0;
		unsigned int final_rbo = 0;
		unsigned int minimap_fbo = 0;
		unsigned int minimap_rbo = 0;
		std::array<unsigned int, 2> volumetric_fbos{0, 0};

		unsigned int pp_fbo = 0;  // post-processing framebuffer (for ping-pong)
		unsigned int pp2_fbo = 0; // post-processing framebuffer 2 (for ping-pong)
								  // needed for stuff like bloom
		unsigned int out_fbo = 0; // output framebuffer (for imgui/display)
		GLint max_draw_buffers = 0;
		GLint max_color_attachments = 0;
		int active_gbuffer_count = 5;

		// === Textures ===
		unsigned int pos_texture = 0;
		unsigned int col_texture = 0;
		unsigned int norm_texture = 0;
		// unsigned int shadow_texture = 0;					//
		// shadow map
		unsigned int material_properties_texture =
			0; // 2D to store roughness, metallic properties
		unsigned int emission_texture = 0;
		unsigned int fallback_material_texture = 0;
		unsigned int fallback_emission_texture = 0;

		// !TODO: jspoh cleanup memory
		// === Geometry Buffers ===
		unsigned int geometry_vao = 0;
		unsigned int geometry_vbo = 0;
		unsigned int geometry_ebo = 0;
		unsigned int geometry_ibo = 0;
		GLsizeiptr geometry_ibo_capacity = 0;
		unsigned int shadow_vao = 0;
		unsigned int shadow_vbo = 0;
		unsigned int shadow_ebo = 0;
		unsigned int empty_vao = 0;
		unsigned int passthrough_vao = 0;
		unsigned int passthrough_vbo = 0;
		unsigned int pbr_light_ubo = 0;
		unsigned int pbr_light_ubo_bound_program = 0;

		// OPTIMIZATION: UBO for bone matrices (avoids 100+ SetUniform calls per animated mesh)
		static constexpr int MAX_BONES = 100;
		unsigned int bone_matrix_ubo = 0;
		unsigned int bone_matrix_ubo_bound_program = 0;

		unsigned int final_texture = 0; // for imgui/post-processing/display
		unsigned int pp_texture = 0;	// for ping-pong for post-processing
		unsigned int pp2_texture = 0;	// for ping-pong for post-processing (bloom etc)
		unsigned int minimap_texture = 0;
		std::array<unsigned int, 2> volumetric_textures{0, 0};
		int minimap_width = 0;
		int minimap_height = 0;
		int pp_width = 0;  // post-process render width (may be < winWidth on Android)
		int pp_height = 0; // post-process render height (may be < winHeight on Android)
		int volumetric_width = 0;
		int volumetric_height = 0;
		int volumetric_history_index = 0;
		bool volumetric_history_valid = false;
		glm::mat4 volumetric_prev_vp = glm::mat4(1.0f);
		glm::vec3 volumetric_prev_cam_pos = glm::vec3(0.0f);
		glm::vec3 volumetric_prev_cam_forward = glm::vec3(0.0f, 0.0f, -1.0f);
		unsigned int volumetric_frame_index = 0;
		std::unordered_map<std::string, int> volumetric_selection_ttl;

		GLint minimap_prev_fbo = 0;
		GLint minimap_prev_viewport[4] = {0, 0, 0, 0};
		GLfloat minimap_prev_clear_color[4] = {0.f, 0.f, 0.f, 0.f};
		GLboolean minimap_prev_depth_test = GL_FALSE;
		GLboolean minimap_prev_depth_mask = GL_TRUE;
		GLboolean minimap_prev_blend = GL_FALSE;
		GLint minimap_prev_blend_src_rgb = GL_ONE;
		GLint minimap_prev_blend_dst_rgb = GL_ZERO;
		GLint minimap_prev_blend_src_alpha = GL_ONE;
		GLint minimap_prev_blend_dst_alpha = GL_ZERO;
		bool minimap_state_saved = false;

		// === Debug Buffers ===
		unsigned int debug_VAO = 0;
		unsigned int debug_VBO = 0;
		GLsizeiptr debug_vbo_capacity = 0;
		std::unordered_set<unsigned int> clamp_configured_textures;

		// === Minimap Wall GPU Buffers ===
		unsigned int minimap_wall_vao = 0;
		unsigned int minimap_wall_vbo = 0;
		GLsizei minimap_wall_vertex_count = 0;

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
		std::shared_ptr<Assets::Shader> tone_gamma_shader = nullptr;
		std::shared_ptr<Assets::Shader> bloom_blend_shader = nullptr;
		std::shared_ptr<Assets::Shader> minimap_wall_shader = nullptr;
		std::shared_ptr<Assets::Shader> volumetric_shader = nullptr;
		std::shared_ptr<Assets::Shader> fxaa_shader = nullptr;

		// for easy access to clear memory
		std::array<unsigned int*, 5> fbos{
			&ds_fbo,
			//&shadow_fbo,
			&final_fbo,
			&pp_fbo,
			&pp2_fbo,
			&minimap_fbo,
		};
		std::array<unsigned int*, 2> rbos{&final_rbo, &minimap_rbo};
		std::array<unsigned int*, 12> texs{
			&ds_depth_texture,
			&pos_texture,
			&col_texture,
			&norm_texture,
			&material_properties_texture,
			&emission_texture,
			&fallback_material_texture,
			&fallback_emission_texture,
			//&shadow_texture,
			&final_texture,
			&pp_texture,
			&pp2_texture,
			&minimap_texture,
		};

		std::shared_ptr<Services> services;
		void initShaders();

		/**
		* .
		*
		* \param tex
		* \param num_i channels
		* \param gl_color_attachment THIS IS NOT YOUR NORMAL ID. USE GL_ATTACHMENT`n`
		* HERE.
		*/
		void _createDeferredShadingBuffer(unsigned int& tex, int num_channels,
										  int gl_color_attachment,
										  GLenum internal_format_override = 0,
										  GLenum format_override = 0,
										  GLenum type_override = 0);

	};
} // namespace PAIN

#endif // PN_PLATFORM_WINDFOWS
