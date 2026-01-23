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
#include "CoreSystems/Assets/sAssets.h"
#include "CoreSystems/Renderer/skybox.h"
#include "CoreSystems/Renderer/text.h"
#include "CoreSystems/Windows/Window.h"
#include "ECS/Controller.h"

namespace PAIN {
	// Light light = {
	//	{2.f, 3.f, 2.f},	// position
	//	{0.2f, 0.2f, 0.2f},					// intensity
	//	Light::ORBIT_ORIGIN
	// };

	WindowsRenderer::WindowsRenderer() {
		PN_CORE_INFO("WindowsRenderer ctor");
	}

	WindowsRenderer::~WindowsRenderer() {
		Cleanup();
	}

	void WindowsRenderer::uploadTexture(std::shared_ptr<Assets::Texture> tex) {

		// ========================================
		// SAVE ACTIVE TEXTURE UNIT
		// ========================================
		GLint activeTextureUnit;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTextureUnit);

		PN_CORE_TRACE("Texture load started, active unit: GL_TEXTURE{}", activeTextureUnit - GL_TEXTURE0);

		// ========================================
		// RESET TO TEXTURE UNIT 0 FOR LOADING
		// ========================================
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Clear any errors
		while (glGetError() != GL_NO_ERROR);

		// VALIDATE EXTRACTED DATA
		if (tex->mipOffsets.size() != tex->mipSizes.size()) {
			throw std::runtime_error("Mip offset/size mismatch!");
		}

		size_t expectedMips = tex->is_cube_map ? (tex->mips * 6) : tex->mips;
		if (tex->mipOffsets.size() != expectedMips) {
			PN_CORE_WARN("Expected {} mip entries, got {}", expectedMips, tex->mipOffsets.size());
		}

		//Generate textures
		glGenTextures(1, &tex->gl_texture);

		if (tex->is_cube_map) {
			glBindTexture(GL_TEXTURE_CUBE_MAP, tex->gl_texture);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, tex->mips - 1);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			GLfloat maxAniso = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
			glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

			size_t dataIndex = 0;
			for (int face = 0; face < 6; ++face) {
				for (uint32_t mip = 0; mip < tex->mips; ++mip) {
					if (dataIndex >= tex->mipOffsets.size()) {
						throw std::runtime_error("Ran out of mip data for cubemap!");
					}

					// USE STORED SIZES FROM EXTRACTION
					size_t offset = tex->mipOffsets[dataIndex];
					size_t mipSize = tex->mipSizes[dataIndex];

					int mipW = std::max(1, tex->width >> mip);
					int mipH = std::max(1, tex->height >> mip);

					if (offset + mipSize > tex->data.size()) {
						throw std::runtime_error("Mip data overflow at face " +
							std::to_string(face) + " mip " + std::to_string(mip));
					}

					glCompressedTexImage2D(
						GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
						mip,
						tex->glTexFormat,
						mipW,
						mipH,
						0,
						static_cast<GLsizei>(mipSize),
						tex->data.data() + offset
					);

					GLenum err = glGetError();
					if (err != GL_NO_ERROR) {
						throw std::runtime_error("OpenGL error uploading mip " +
							std::to_string(mip) + ": 0x" +
							std::to_string(err));
					}

					dataIndex++;
				}
			}

			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}
		else {
			glBindTexture(GL_TEXTURE_2D, tex->gl_texture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, tex->mips - 1);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			GLfloat maxAniso = 1.0f;
			glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);

			for (uint32_t mip = 0; mip < tex->mips; ++mip) {
				if (mip >= tex->mipOffsets.size()) {
					throw std::runtime_error("Missing mip data for level " + std::to_string(mip));
				}

				// USE STORED SIZES FROM EXTRACTION
				size_t offset = tex->mipOffsets[mip];
				size_t mipSize = tex->mipSizes[mip];

				int mipW = std::max(1, tex->width >> mip);
				int mipH = std::max(1, tex->height >> mip);

				PN_CORE_TRACE("Uploading mip {}: {}x{}, {} bytes at offset {}",
					mip, mipW, mipH, mipSize, offset);

				if (offset + mipSize > tex->data.size()) {
					throw std::runtime_error("Mip data overflow at mip " + std::to_string(mip));
				}

				glCompressedTexImage2D(
					GL_TEXTURE_2D,
					mip,
					tex->glTexFormat,
					mipW,
					mipH,
					0,
					static_cast<GLsizei>(mipSize),
					tex->data.data() + offset
				);

				GLenum err = glGetError();
				if (err != GL_NO_ERROR) {
					PN_CORE_ERROR("OpenGL error uploading mip {}: 0x{:X}", mip, err);
					PN_CORE_ERROR("  Dimensions: {}x{}", mipW, mipH);
					PN_CORE_ERROR("  Size: {} bytes", mipSize);
					PN_CORE_ERROR("  Offset: {}", offset);
					PN_CORE_ERROR("  Format: 0x{:X}", tex->glTexFormat);
					throw std::runtime_error("OpenGL error uploading mip " + std::to_string(mip) +
						": 0x" + std::to_string(err));
				}
			}

			glBindTexture(GL_TEXTURE_2D, 0);
		}

		// Free CPU-side texture data after GPU upload
		PN_CORE_INFO("Texture uploaded to GPU (ID: {}), freeing CPU data ({} bytes)",
			tex->gl_texture, tex->data.size());
		std::vector<uint8_t>().swap(tex->data);
		std::vector<size_t>().swap(tex->mipOffsets);
		std::vector<size_t>().swap(tex->mipSizes);

		// ========================================
		// RESTORE ACTIVE TEXTURE UNIT
		// ========================================
		glActiveTexture(activeTextureUnit);
	}

	void WindowsRenderer::initSceneVbo() {


		if (!geometry_vbo) return;

		// Initialize Buffers
		PN_CORE_INFO("Initializing New Buffers");

		// Clear buffers before building
		clearBuffers();

		glBindVertexArray(geometry_vao);

		// ========================================
		// UNIFIED GEOMETRY UPLOAD (Instancing-Compatible)
		// ========================================
		// Both instanced and non-instanced rendering now upload ALL geometry ONCE
		// This eliminates per-frame glBufferSubData calls and uses GL_STATIC_DRAW

		unsigned int vertexOffset = 0;
		unsigned int indexOffset = 0;
		std::vector<Assets::Vertex> allVertices;
		std::vector<unsigned int> allIndices;
		std::unordered_set<std::string> uploaded;

		// Get ECS controller to iterate ALL registries (including prefab edit
		// registries)
		auto ecs = services->get<ECS::Controller>();

		// Iterate ALL registries to collect models (fixes prefab editor rendering)
		for (auto registryId : ecs->getAllRegistryIDs()) {
			auto& registry = ecs->getRegistry(registryId);
			auto view = registry.view<ModelRenderer>();

			// Collect all unique models and build combined vertex/index buffers
			for (auto e : view) {

				// Get mdl asset
				auto mdl = ecs->getEntityComponent<ModelRenderer>(e, registryId);
				if (!mdl.has_value())
					continue;

				// Invalidate Cached Buffer Offsets
				mdl.value().get().bufferOffset.isUploaded = false;

				// Retrieve model asset with validation
				auto mdl_opt = services->get<Assets::Manager>()->getAsset<Assets::Model>(
					mdl.value().get().modelGUID);
				if (!mdl_opt.has_value() || mdl_opt.value()->type != Assets::Type::Model)
					continue;
				const auto& modelAsset = mdl_opt.value();

				// Skip if already uploaded (deduplicate by model path)
				if (uploaded.find(modelAsset->vpath) != uploaded.end())
					continue;
				uploaded.insert(modelAsset->vpath);

				// Store offset for this model in the map (used by both rendering paths)
				instanced_offsets[modelAsset->vpath] = {
					indexOffset, (unsigned int)modelAsset->indices.size()};

				// Add vertices
				allVertices.insert(allVertices.end(), modelAsset->vertices.begin(),
								   modelAsset->vertices.end());

				// Add indices with vertex offset applied
				for (unsigned int idx : modelAsset->indices) {
					allIndices.push_back(vertexOffset + idx);
				}

				vertexOffset += modelAsset->vertices.size();
				indexOffset += modelAsset->indices.size();
			}
		}

		// Upload ALL geometry ONCE to main geometry buffers
		glBindBuffer(GL_ARRAY_BUFFER, geometry_vbo);
		glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(Assets::Vertex),
					 allVertices.data(),
					 GL_STATIC_DRAW); // STATIC_DRAW for immutable geometry

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					 allIndices.size() * sizeof(unsigned int), allIndices.data(),
					 GL_STATIC_DRAW);

		// Also upload to shadow buffers (same geometry)
		glBindVertexArray(shadow_vao);
		glBindBuffer(GL_ARRAY_BUFFER, shadow_vbo);
		glBufferData(GL_ARRAY_BUFFER, allVertices.size() * sizeof(Assets::Vertex),
					 allVertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadow_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					 allIndices.size() * sizeof(unsigned int), allIndices.data(),
					 GL_STATIC_DRAW);

		// If instancing is enabled, create instance buffer for per-instance data
		if (GS.use_instanced_rendering) {
			glGenBuffers(1, &geometry_ibo);
			glBindBuffer(GL_ARRAY_BUFFER, geometry_ibo);
			glBufferData(GL_ARRAY_BUFFER, uploaded.size() * sizeof(IBOData), nullptr,
						 GL_DYNAMIC_DRAW);
		}

		glBindVertexArray(0);

		// End log
		PN_CORE_INFO("New buffers initialized!");
	}

	void WindowsRenderer::clearBuffers() {

		// Log
		PN_CORE_INFO("Clearing Existing Buffers To Build New Model Buffers");

		// Clear the offset tracking map
		instanced_offsets.clear();

		// Reset buffers to empty state
		glBindVertexArray(geometry_vao);
		glBindBuffer(GL_ARRAY_BUFFER, geometry_vbo);
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW); // Free memory

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

		// Also clear shadow buffers
		glBindVertexArray(shadow_vao);
		glBindBuffer(GL_ARRAY_BUFFER, shadow_vbo);
		glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadow_ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);

		// Unbind
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		// Clear buffers
		PN_CORE_INFO("Buffers cleared.");
	}

	void WindowsRenderer::initShaders() {

		// Identify all paths
#ifdef PN_PLATFORM_WINDOWS
		std::filesystem::path pbr_path = "engine/shaders/pbr.vert";
		std::filesystem::path geometry_path = "engine/shaders/geometry.vert";
		std::filesystem::path floor_path = "engine/shaders/floor.vert";
		std::filesystem::path passthrough_path = "engine/shaders/passthrough.vert";
		std::filesystem::path shadow_path = "engine/shaders/shadow.vert";
		std::filesystem::path texture2d_path = "engine/shaders/texture2d.vert";
		std::filesystem::path gamma_path = "engine/shaders/gamma.vert";
		std::filesystem::path debug_geometry_path =
			"engine/shaders/debug_geometry.vert";
		std::filesystem::path blur_path = "engine/shaders/blur.vert";
		std::filesystem::path bloom_path = "engine/shaders/bloom.vert";
		std::filesystem::path bloom_blend_path = "engine/shaders/bloom_blend.vert";
		std::filesystem::path tone_path = "engine/shaders/tone.vert";
#else
		std::filesystem::path pbr_path = "engine\\shaders\\android_pbr.vert";
		std::filesystem::path geometry_path =
			"engine\\shaders\\android_geometry.vert";
		std::filesystem::path floor_path = "engine\\shaders\\android_floor.vert";
		std::filesystem::path passthrough_path =
			"engine\\shaders\\android_passthrough.vert";
		std::filesystem::path shadow_path = "engine\\shaders\\android_shadow.vert";
		std::filesystem::path texture2d_path =
			"engine\\shaders\\android_texture2d.vert";
		std::filesystem::path gamma_path = "engine\\shaders\\android_gamma.vert";
		std::filesystem::path debug_geometry_path =
			"engine\\shaders\\android_debug_geometry.vert";
		std::filesystem::path blur_path = "engine\\shaders\\android_blur.vert";
		std::filesystem::path bloom_path = "engine\\shaders\\android_bloom.vert";
		std::filesystem::path bloom_blend_path =
			"engine\\shaders\\android_bloom_blend.vert";
		std::filesystem::path tone_path = "engine\\shaders\\android_tone.vert";
#endif

		// Get assets loader
		auto assets_loader = services->get<Assets::Manager>();

		// FLoor shader
		auto shader_opt = assets_loader->getAsset<Assets::Shader>(floor_path);
		floor_shader = shader_opt.has_value() ? shader_opt.value() : floor_shader;

		if (!floor_shader || floor_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for floor");
			throw std::runtime_error("");
			return;
		}

		// PBR Shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(pbr_path);
		pbr_shader = shader_opt.has_value() ? shader_opt.value() : pbr_shader;

		if (!pbr_shader || pbr_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for pbr");
			throw std::runtime_error("");
			return;
		}

		// Geometry shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(geometry_path);
		geometry_shader =
			shader_opt.has_value() ? shader_opt.value() : geometry_shader;

		if (!geometry_shader || geometry_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for geometry");
			throw std::runtime_error("");
			return;
		}

		// Pass through shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(passthrough_path);
		passthrough_shader =
			shader_opt.has_value() ? shader_opt.value() : passthrough_shader;

		if (!passthrough_shader || passthrough_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for passthrough");
			throw std::runtime_error("");
			return;
		}

		// Shadow shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(shadow_path);
		shadow_shader = shader_opt.has_value() ? shader_opt.value() : shadow_shader;

		if (!shadow_shader || shadow_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for shadow");
			throw std::runtime_error("");
			return;
		}

		// Texture shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(texture2d_path);
		texture2d_shader =
			shader_opt.has_value() ? shader_opt.value() : texture2d_shader;

		if (!texture2d_shader || texture2d_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for texture2d");
			throw std::runtime_error("");
			return;
		}

		// Tone mapping shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(tone_path);
		tone_shader = shader_opt.has_value() ? shader_opt.value() : tone_shader;

		if (!tone_shader || tone_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for tone");
			throw std::runtime_error("");
			return;
		}

		// Bloom shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(bloom_path);
		bloom_shader = shader_opt.has_value() ? shader_opt.value() : bloom_shader;

		if (!bloom_shader || bloom_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for bloom");
			throw std::runtime_error("");
			return;
		}

		// Bloom blend shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(bloom_blend_path);
		bloom_blend_shader =
			shader_opt.has_value() ? shader_opt.value() : bloom_blend_shader;

		if (!bloom_blend_shader || bloom_blend_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for bloom blend");
			throw std::runtime_error("");
			return;
		}

		// Blur shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(blur_path);
		blur_shader = shader_opt.has_value() ? shader_opt.value() : blur_shader;

		if (!blur_shader || blur_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for blur");
			throw std::runtime_error("");
			return;
		}

		// Gamma shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(gamma_path);
		gamma_shader = shader_opt.has_value() ? shader_opt.value() : gamma_shader;

		if (!gamma_shader || gamma_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for gamma");
			throw std::runtime_error("");
			return;
		}

		// Debug shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(debug_geometry_path);
		debug_shader = shader_opt.has_value() ? shader_opt.value() : debug_shader;

		if (!debug_shader || debug_shader->GetRendererID() == 0) {
			PN_CORE_ERROR("Failed to create shader program for debug");
			throw std::runtime_error("");
			return;
		}
	}

	// TO BE MOVED
	void WindowsRenderer::_createDeferredShadingBuffer(unsigned int& tex,
													   int num_channels,
													   int gl_color_attachment) {
		glGenTextures(1, &tex);
		if (!tex) {
			PN_CORE_ERROR("Failed to gen texture");
			throw std::runtime_error("");
			return;
		}

		glBindTexture(GL_TEXTURE_2D, tex);

		switch (num_channels) {
		case 2:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, winWidth, winHeight, 0, GL_RG,
						 GL_FLOAT, nullptr);
			break;
		case 3:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, winWidth, winHeight, 0, GL_RGB,
						 GL_FLOAT, nullptr);
			break;
		case 4:
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA,
						 GL_FLOAT, nullptr);
			break;
		default:
			PN_CORE_ERROR(
				"{} channels isn't supported(by me lol not opengl so need to add)!",
				num_channels);
			break;
		};
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
						GL_NEAREST); // DO NOT USE GL_LINEAR
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, gl_color_attachment, GL_TEXTURE_2D,
							   tex, 0);
	}

	// TO BE MOVED
	void WindowsRenderer::_initDeferredShadingBuffers() {
		PN_CORE_INFO("Initializing deferred shading buffers with size: {}x{}",
					 winWidth, winHeight);

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
			_createDeferredShadingBuffer(material_properties_texture, 3,
										 GL_COLOR_ATTACHMENT3);
			_createDeferredShadingBuffer(emission_texture, 3, GL_COLOR_ATTACHMENT4);

			static constexpr int NUM_GBUFFERS = 5;

			unsigned int attachments[NUM_GBUFFERS] = {
				GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2,
				GL_COLOR_ATTACHMENT3,
				GL_COLOR_ATTACHMENT4,
			};
			glDrawBuffers(NUM_GBUFFERS, attachments);

			glGenRenderbuffers(1, &ds_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, ds_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, winWidth,
								  winHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
									  GL_RENDERBUFFER, ds_rbo);

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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA,
						 GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
								   final_texture, 0);

			glGenRenderbuffers(1, &final_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, final_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, winWidth,
								  winHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
									  GL_RENDERBUFFER, final_rbo);

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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA,
						 GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
								   pp_texture, 0);

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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, winWidth, winHeight, 0, GL_RGBA,
						 GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
								   pp2_texture, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// === VAO/VBO For Final Passthrough Texture ===
		{
			static constexpr float quadVertices[] = {
				// positions    // texCoords
				-1.0f, 1.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
				1.0f, -1.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 1.0f,
				1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f};

			glGenVertexArrays(1, &passthrough_vao);
			glGenBuffers(1, &passthrough_vbo);

			glBindVertexArray(passthrough_vao);
			glBindBuffer(GL_ARRAY_BUFFER, passthrough_vbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices,
						 GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);

			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
								  (void*)(2 * sizeof(float)));

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
			// glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Assets::Vertex),
			// nullptr, GL_DYNAMIC_DRAW);

			// Generate and bind EBO (index buffer)
			glGenBuffers(1, &geometry_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry_ebo);
			// glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(unsigned int),
			// nullptr, GL_DYNAMIC_DRAW);

			// Position attribute, layout(location = 0)
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, pos));
			glEnableVertexAttribArray(0);

			// Normal attribute, layout(location = 1)
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, normal));
			glEnableVertexAttribArray(1);

			// texcoords attribute, layout(location = 2)
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, uv));
			glEnableVertexAttribArray(2);

			// bone indices
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, boneIndices_f));
			glEnableVertexAttribArray(3);

			// bone weights
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, boneWeights));
			glEnableVertexAttribArray(4);

			// tangent
			glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, tangent));
			glEnableVertexAttribArray(5);

			// bitangent
			glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, bitangent));
			glEnableVertexAttribArray(6);

			// Unbind VAO
			glBindVertexArray(0);
		}

		// for shadow
		{
			// Generate and bind VAO
			glGenVertexArrays(1, &shadow_vao);
			glBindVertexArray(shadow_vao);

			// Generate and bind VBO
			glGenBuffers(1, &shadow_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, shadow_vbo);
			// glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Assets::Vertex),
			// nullptr, GL_DYNAMIC_DRAW);

			// Generate and bind EBO (index buffer)
			glGenBuffers(1, &shadow_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadow_ebo);
			// glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(unsigned int),
			// nullptr, GL_DYNAMIC_DRAW);

			// Position attribute, layout(location = 0)
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, pos));
			glEnableVertexAttribArray(0);

			// Normal attribute, layout(location = 1)
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, normal));
			glEnableVertexAttribArray(1);

			// texcoords attribute, layout(location = 2)
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex),
								  (void*)offsetof(Assets::Vertex, uv));
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
			// glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * 7 * sizeof(float), nullptr,
			// GL_DYNAMIC_DRAW);

			// Position attribute, layout(location = 0)
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
								  (void*)0);

			// Color attribute, layout(location = 1)
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float),
								  (void*)(3 * sizeof(float)));
			glBindVertexArray(0);
		}
	}

	void WindowsRenderer::Init(std::shared_ptr<Services> app_services) {
		services = app_services;

		// Set win width and height
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

	void WindowsRenderer::Render2DTexture(GLuint texture_id, const glm::vec2& pos,
										  glm::vec2& scale,
										  const glm::vec4& uv_transform) {
		if (texture_id == 0) {
			PN_CORE_ERROR("Invalid texture_id in Render2DTexture");
			return;
		}

		if (!texture2d_shader) {
			PN_CORE_ERROR("Unable to find texture2d_shader");
			return;
		}

		// ========================================
		// SAVE STATE
		// ========================================
		GLint currentActiveTexture;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &currentActiveTexture);

		GLint currentTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);

		GLint currentVAO;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);

		GLint currentProgram;
		glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

		// ========================================
		// RENDER
		// ========================================
		auto window_service = services->get<Window::Window>();
		auto framebuffer = window_service->getFrameBuffer();

		float aspect_ratio =
			static_cast<float>(framebuffer.x) / static_cast<float>(framebuffer.y);
		glm::vec2 corrected_scale = glm::vec2(scale.x / aspect_ratio, scale.y);

		texture2d_shader->Bind();

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("Error after shader bind: 0x{:X}", err);
		}

		texture2d_shader->SetUniform("pos", pos);
		texture2d_shader->SetUniform("ndc_scale", corrected_scale);
		texture2d_shader->SetUniform("uv_transform",
									 uv_transform); // Pass UV transform

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, texture_id);

		// Force clamp to edge to prevent bleeding from wrapping
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		texture2d_shader->SetUniform("tex", 6);

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("Error after texture bind: 0x{:X}", err);
		}

		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("Error after draw call: 0x{:X}", err);
		}

		// ========================================
		// RESTORE STATE
		// ========================================
		glActiveTexture(currentActiveTexture);
		glBindTexture(GL_TEXTURE_2D, currentTexture);
		glBindVertexArray(currentVAO);
		glUseProgram(currentProgram);
	}

	void WindowsRenderer::BeginShadowPass(const Light& l) {
		glBindFramebuffer(GL_FRAMEBUFFER, l.getShadowFbo());
		// glClearDepth(1.0f);  // Explicitly set clear value

#ifdef PN_PLATFORM_ANDROID
		// critical for Mali GPU on android
		// disable color writes
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
#endif

		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void WindowsRenderer::DrawShadows(const ModelRenderer& component,
									  const glm::mat4& M, const Light& l) {
		if (!shadow_shader || !component.cachedModelAsset || !component.castShadows) {
			return;
		}

		// ========================================
		// PERFORMANCE OPTIMIZATION: Offset-Based Drawing
		// ========================================
		// Geometry was uploaded ONCE in initSceneVbo(), check if uploaded
		if (!component.bufferOffset.isUploaded) {
			return; // Model not yet uploaded to buffers
		}

		shadow_shader->Bind();
		shadow_shader->SetUniform("u_M", M);
		shadow_shader->SetUniform("u_V", l.view());
		shadow_shader->SetUniform("u_P", l.projection());

		// Bind shared shadow VAO (geometry already uploaded)
		glBindVertexArray(shadow_vao);

		// Draw using offset into shared buffer
		const auto& modelAsset = component.cachedModelAsset;
		if (modelAsset->submeshes.empty()) {
			// No submeshes - draw entire model
			glDrawElements(
				GL_TRIANGLES, component.bufferOffset.indexCount, GL_UNSIGNED_INT,
				(void*)(component.bufferOffset.indexOffset * sizeof(unsigned int)));
		} else {
			// Draw each submesh with correct offset
			for (const auto& submesh : modelAsset->submeshes) {
				glDrawElements(
					GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT,
					(void*)((component.bufferOffset.indexOffset + submesh.firstIndex) *
							sizeof(unsigned int)));
			}
		}

		glBindVertexArray(0);

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error in DrawShadows: {} on mesh {}", err,
						  component.cachedModelAsset->vpath);
		}
	}

	void WindowsRenderer::EndShadowPass() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

#ifdef PN_PLATFORM_ANDROID
		// critical for Mali GPU on android
		// reenable color writes
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif
	}

	void WindowsRenderer::BeginGeometryPass(
		std::shared_ptr<Scene::SceneManager> scene) {
		// PN_CORE_INFO("Viewport: {}, {}", winWidth, winHeight);

		glViewport(0, 0, winWidth, winHeight);
		glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// GLenum err = glGetError();
		// if (err != GL_NO_ERROR) {
		//	PN_CORE_ERROR("OpenGL error before drawing skybox: {}", err);
		// }

		//// draw skybox
		//{
		//	Skybox::get().render(scene->GetActiveCamera()->view(),
		// scene->GetActiveCamera()->projection());
		//}
		// err = glGetError();
		// if (err != GL_NO_ERROR) {
		//	PN_CORE_ERROR("OpenGL error after drawing skybox: {}", err);
		//}

		// draw floor
		if (GraphicsSettings::get().draw_floor) {
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

	void WindowsRenderer::DrawGeometry(std::shared_ptr<Scene::SceneManager> scene,
									   ModelRenderer& component,
									   const glm::mat4& M) {

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error before DrawGeometry: {}", err);
		}

		if (!geometry_shader || !component.cachedModelAsset) {
			return;
		}

		auto assetManager = services->get<Assets::Manager>();

		const auto& modelAsset = component.cachedModelAsset;

		geometry_shader->SetUniform("u_M", M);
		geometry_shader->SetUniform("u_InvertUvY", 0.f);

		// ========================================
		// PERFORMANCE OPTIMIZATION: Offset-Based Drawing
		// ========================================
		// Geometry was uploaded ONCE in initSceneVbo(), now we just draw from offsets
		// No more per-frame glBufferSubData or glBufferData calls!

		// Ensure component has offset data populated
		if (!component.bufferOffset.isUploaded) {
			// Look up offset from instanced_offsets map (populated in initSceneVbo)
			auto it = instanced_offsets.find(modelAsset->vpath);
			if (it != instanced_offsets.end()) {
				component.bufferOffset.indexOffset = it->second.idx_offset;
				component.bufferOffset.indexCount = it->second.idx_count;
				component.bufferOffset.isUploaded = true;
			} else {
				PN_CORE_ERROR("Model {} not found in uploaded buffers! Did you call "
							  "initSceneVbo()?",
							  modelAsset->vpath);
				return;
			}
		}

		// Bind shared VAO (geometry already uploaded)
		glBindVertexArray(geometry_vao);

		// Render each submesh with its material
		for (size_t i = 0; i < modelAsset->submeshes.size(); ++i) {
			const auto& submesh = modelAsset->submeshes[i];

			// Check out of bounds
			if (submesh.materialIndex >= component.materials.size()) {
				PN_CORE_WARN("Submesh {} references material index {} but only {} "
							 "materials available",
							 i, submesh.materialIndex, component.materials.size());
				continue; // Skip this submesh
			}

			MaterialInstance* material = &component.materials[submesh.materialIndex];

			// GPU texture handles (uploaded once, reused)
			unsigned int albedoTexture = 0;
			unsigned int normalTexture = 0;
			unsigned int metallicTexture = 0;
			unsigned int roughnessTexture = 0;
			unsigned int aoTexture = 0;
			unsigned int emissiveTexture = 0;
			unsigned int heightTexture = 0;
			unsigned int opacityTexture = 0;

			// optional material asset
			auto materialAssetOpt =
				assetManager->getAsset<Assets::Material>(material->materialGUID);

			// Load material asset
			auto materialAsset =
				materialAssetOpt.has_value() ? materialAssetOpt.value() : nullptr;

			// Check material asset
			if (materialAsset) {

				{
					// Albedo Texture
					std::optional<std::shared_ptr<Assets::Texture>> tex_opt =
						material->useOverrides ? assetManager->getAsset<Assets::Texture>(
													 material->albedoTextureOverride)
											   : assetManager->getAsset<Assets::Texture>(
													 materialAsset->albedoTexturePath);

					if (tex_opt.has_value() && GS.DEBUG_USE_DIFFUSE_MAP) {
						albedoTexture = tex_opt.value()->gl_texture;
					}

					// Normal texture
					tex_opt = material->useOverrides
								  ? assetManager->getAsset<Assets::Texture>(
										material->normalTextureOverride)
								  : assetManager->getAsset<Assets::Texture>(
										materialAsset->normalTexturePath);

					if (tex_opt.has_value() && GS.DEBUG_USE_NORMAL_MAP) {
						normalTexture = tex_opt.value()->gl_texture;
					}

					// Metallic texture
					tex_opt = material->useOverrides
								  ? assetManager->getAsset<Assets::Texture>(
										material->metallicTextureOverride)
								  : assetManager->getAsset<Assets::Texture>(
										materialAsset->metallicTexturePath);

					if (tex_opt.has_value() && GS.DEBUG_USE_ROUGHNESSMETALLIC_MAP) {
						metallicTexture = tex_opt.value()->gl_texture;
					}

					// Roughness texture
					tex_opt = material->useOverrides
								  ? assetManager->getAsset<Assets::Texture>(
										material->roughnessTextureOverride)
								  : assetManager->getAsset<Assets::Texture>(
										materialAsset->roughnessTexturePath);

					if (tex_opt.has_value() && GS.DEBUG_USE_ROUGHNESSMETALLIC_MAP) {
						roughnessTexture = tex_opt.value()->gl_texture;
					}

					// AO texture
					tex_opt = material->useOverrides
								  ? assetManager->getAsset<Assets::Texture>(
										material->aoTextureOverride)
								  : assetManager->getAsset<Assets::Texture>(
										materialAsset->aoTexturePath);

					if (tex_opt.has_value() && GS.DEBUG_USE_AO_MAP) {
						aoTexture = tex_opt.value()->gl_texture;
					}

					// Emissive texture
					tex_opt = material->useOverrides
								  ? assetManager->getAsset<Assets::Texture>(
										material->emissiveTextureOverride)
								  : assetManager->getAsset<Assets::Texture>(
										materialAsset->emissiveTexturePath);

					if (tex_opt.has_value() && GS.DEBUG_USE_EMISSION_MAP) {
						emissiveTexture = tex_opt.value()->gl_texture;
					}

					if (material->useOverrides) {
						geometry_shader->SetUniform("u_UseEmissionOverride", 1.f);
						geometry_shader->SetUniform("u_EmissionOverride", material->emissiveOverride);
						// geometry_shader->SetUniform("u_EmissionOverride", {1,0,1});
					}
					else {
						geometry_shader->SetUniform("u_UseEmissionOverride", 0.f);
					}

					// Height texture
					/*
        tex_opt = material->useOverrides ?
                assetManager->getAsset<Assets::Texture>(material->heightTextureOverride)
                :
        assetManager->getAsset<Assets::Texture>(materialAsset->heightTexturePath);

        if (tex_opt.has_value()) {
                heightTexture = tex_opt.value()->gl_texture;
        }
        */

					// Opacity texture
					tex_opt = material->useOverrides
								  ? assetManager->getAsset<Assets::Texture>(
										material->opacityTextureOverride)
								  : assetManager->getAsset<Assets::Texture>(
										materialAsset->opacityTexturePath);

					if (tex_opt.has_value()) {
						opacityTexture = tex_opt.value()->gl_texture;
					}

					// Use override or asset default
					glm::vec3 baseColor = material->useOverrides
											  ? material->baseColorOverride
											  : materialAsset->baseColor;

					float metallic = material->useOverrides ? material->metallicOverride
															: materialAsset->metallic;

					float roughness = material->useOverrides ? material->roughnessOverride
															 : materialAsset->roughness;

					geometry_shader->SetUniform("material.rough", roughness);
					geometry_shader->SetUniform("material.metal", metallic);
					geometry_shader->SetUniform("material.color", baseColor);
				}
			}

			// Bind textures from MaterialInstance
			bool hasTexture = albedoTexture != 0;
			geometry_shader->SetUniform("material.useTex", hasTexture ? 1.0f : 0.0f);
			// geometry_shader->SetUniform("material.alwaysLit", emissiveTexture ? 1.f :
			// 0.f);

			if (hasTexture && GS.DEBUG_USE_DIFFUSE_MAP) {
				glActiveTexture(GL_TEXTURE6);
				glBindTexture(GL_TEXTURE_2D, albedoTexture);
				geometry_shader->SetUniform("material.tex", 6);
			} else {
				geometry_shader->SetUniform("material.useTex", 0.f);
			}

			if (GraphicsSettings::get().DEBUG_USE_AO_MAP && aoTexture != 0) {
				glActiveTexture(GL_TEXTURE7);
				glBindTexture(GL_TEXTURE_2D, aoTexture);
				geometry_shader->SetUniform("material.ao_map", 7);
				geometry_shader->SetUniform("material.use_ao", 1.0f);
			} else {
				geometry_shader->SetUniform("material.use_ao", 0.0f);
			}

			if (GS.DEBUG_USE_NORMAL_MAP && normalTexture) {
				glActiveTexture(GL_TEXTURE8);
				glBindTexture(GL_TEXTURE_2D, normalTexture);
				geometry_shader->SetUniform("material.normal_map", 8);
				geometry_shader->SetUniform("material.use_normal", 1.f);
			} else {
				geometry_shader->SetUniform("material.use_normal", 0.f);
			}

			if (GS.DEBUG_USE_ROUGHNESSMETALLIC_MAP && roughnessTexture) {
				glActiveTexture(GL_TEXTURE9);
				glBindTexture(GL_TEXTURE_2D, roughnessTexture);
				geometry_shader->SetUniform("material.roughnessmetallic_map", 9);
				geometry_shader->SetUniform("material.use_roughnessmetallic", 1.f);
			} else {
				geometry_shader->SetUniform("material.use_roughnessmetallic", 0.f);
			}

			if (GS.DEBUG_USE_EMISSION_MAP && emissiveTexture) {
				glActiveTexture(GL_TEXTURE10);
				glBindTexture(GL_TEXTURE_2D, emissiveTexture);
				geometry_shader->SetUniform("material.use_emission", 1.f);
				geometry_shader->SetUniform("material.emission_map", 10);
			} else {
				geometry_shader->SetUniform("material.use_emission", 0.f);
			}

			// animation
			geometry_shader->SetUniform("u_Animated", component.isPlaying ? 1.f : 0.f);
			int bones_skipped{};
			if (component.isPlaying) {
				// PN_CORE_TRACE("Animation playing: {}s", component.animationTime);

				static std::vector<glm::mat4> boneMatrices;
				static constexpr int MAX_BONES = 100;
				boneMatrices.resize(MAX_BONES, glm::mat4(1.f));

				// find local bone xforms relative to parent
				// these mtx move this particular bone the specific amount RELATIVE to
				// it's parent eg. how much a finger moves relative to the hand bone (not
				// absolute positioning)
				std::vector<glm::mat4> relative_poses(modelAsset->skeleton.size(),
													  glm::mat4(1.f));

				// each track controls a single bone's animation
				for (const auto& [bone_name, track] :
					 modelAsset->animations[component.currentAnimationIndex].track_map) {
					// find the animation keyframe corresponding to current animation time
					const auto key_it = std::lower_bound(
						track.begin(), track.end(), component.animationTime,
						[](const auto& key, const float t) { return key.time < t; });
					if (key_it == track.end())
						PN_CORE_ERROR("Invalid iterator key_it in animation block in "
									  "DrawGeometry in WindowsRenderer.cpp");

					// find the bone idx affected by current track
					std::string bone_name_copy = bone_name; // Create copy for lambda
					const auto bone_it = std::find_if(
						modelAsset->skeleton.begin(), modelAsset->skeleton.end(),
						[&bone_name_copy](const Assets::Bone& b) {
							return b.name == bone_name_copy;
						});
					if (bone_it == modelAsset->skeleton.end()) {
						// PN_CORE_WARN("Bone does not exist for animation track {} for model
						// {}. Skipped: {}", component.currentAnimationIndex,
						// modelAsset->vpath, ++bones_skipped); PN_CORE_ERROR("Invalid
						// iterator bone_it in animation block in DrawGeometry in
						// WindowsRenderer.cpp");
						continue;
					}
					const int bone_idx =
						std::distance(modelAsset->skeleton.begin(), bone_it);

					// get the xform matrix that applies to current vertex from current
					// animation key
					const glm::mat4 scale = glm::scale(glm::mat4(1.f), key_it->scale);
					const glm::mat4 rotate = glm::mat4_cast(key_it->rotation);
					const glm::mat4 translate =
						glm::translate(glm::mat4(1.f), key_it->translation);
					glm::mat4 animated_pose = translate * rotate * scale;

					if (GraphicsSettings::get().interpolate_animation) {
						auto next_key_it = std::next(key_it);
						if (next_key_it != track.end()) {
							// interpolate between current keyframe pose and next keyframe pose
							const float t = (component.animationTime - key_it->time) /
											(next_key_it->time - key_it->time);
							const glm::vec3 i_scale =
								glm::mix(key_it->scale, next_key_it->scale, t);
							const glm::quat i_rotate =
								glm::slerp(key_it->rotation, next_key_it->rotation, t);
							const glm::vec3 i_translate =
								glm::mix(key_it->translation, next_key_it->translation, t);

							const glm::mat4 i_scale_mtx = glm::scale(glm::mat4(1.f), i_scale);
							const glm::mat4 i_rotate_mtx = glm::mat4_cast(i_rotate);
							const glm::mat4 i_translate_mtx =
								glm::translate(glm::mat4(1.f), i_translate);

							// update interpolated matrix
							animated_pose = i_translate_mtx * i_rotate_mtx * i_scale_mtx;
						}
					}

					relative_poses[bone_idx] = animated_pose;

					// multiply with bind pose mtx(the T shape thingy) for final xform
					// matrix
					// boneMatrices[bone_idx] = animated_pose *
					// glm::inverse(bone_it->bindPose);
				}

				// account for parent bone transformation
				std::vector<glm::mat4> poses(modelAsset->skeleton.size());
				for (int i{}; i < modelAsset->skeleton.size(); ++i) {
					// if is parent, no need
					if (modelAsset->skeleton[i].parent == -1) {
						poses[i] = relative_poses[i];
						continue;
					}

					// account for parent's xform
					poses[i] = poses[modelAsset->skeleton[i].parent] * relative_poses[i];
					// poses[i] = relative_poses[i] * poses[modelAsset->skeleton[i].parent];
				}

				// apply to bind pose (T pose)
				for (int i{}; i < modelAsset->skeleton.size(); ++i) {
					boneMatrices[i] = poses[i] * modelAsset->skeleton[i].bindPose;
					// boneMatrices[i] = glm::inverse(modelAsset->skeleton[i].bindPose) *
					// poses[i];
				}

				// populate animated bone xforms in shader
				for (size_t i{}; i < boneMatrices.size(); ++i) {
					const std::string uniform_name =
						"u_BoneMatrices[" + std::to_string(i) + "]";
					geometry_shader->SetUniform(uniform_name, boneMatrices[i]);
				}

			} else {
				// PN_CORE_TRACE("{} is not playing animation", modelAsset->vpath);
			}

			// debug
			geometry_shader->SetUniform(
				"DEBUG_TYPE", (float)GraphicsSettings::get().DEBUG_PBR_MAP_TYPE);

			// ========================================
			// Draw this submesh using offset into shared buffer
			// ========================================
			glDrawElements(
				GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT,
				(void*)((component.bufferOffset.indexOffset + submesh.firstIndex) *
						sizeof(unsigned int)));
		}

		// LogMemoryFullDiagnostic("After Rendering Sub Meshes.");

		glBindVertexArray(0);

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL error after DrawGeometry: {}", err);
		}
	}

	void WindowsRenderer::EndGeometryPass() {
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void WindowsRenderer::ReflectionPass(const ModelRenderer& component) {
		// if (m.materials[0].reflection_type ==
		// m.materials[0].REFLECTION_TYPES::NONE) { 	return;
		// }
	}

	void WindowsRenderer::LightingPass(std::shared_ptr<Scene::SceneManager> scene,
									   const LightSources& lights) {
		//{
		//	/* this block is for debug tracing. print color texture(buffer) straight
		// to screen */

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

		// glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);
		glClear(GL_COLOR_BUFFER_BIT);

		// #ifdef JS_DEBUG
		//		passthrough_shader->Bind();
		//		glActiveTexture(GL_TEXTURE0);
		//		glBindTexture(GL_TEXTURE_2D, col_texture);
		//		passthrough_shader->SetUniform("tex", 0);
		// #else
		//  render to final framebuffer for post processing/imgui/display

		{
			// pbr pass

			// lighting shouldnt write to depth buffer
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);

			pbr_shader->Bind();

			{
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, pos_texture);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, col_texture);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, norm_texture);

				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, material_properties_texture);

				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, emission_texture);
			}

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after binding gbuffer textures: {}", err);
			}

			static constexpr int NEXT_VALID_TEXID =
				5; // increment this after adding new texture bindings above
			int tex_id = NEXT_VALID_TEXID;
			int i{};
			for (const Light& l : LightSources::get().getAll()) {
				std::stringstream ss;
				if (l.getShadowType() == Light::SHADOW_TYPES::MAPPED) {
					glActiveTexture(GL_TEXTURE0 + tex_id);
					glBindTexture(GL_TEXTURE_2D, l.getShadowTexture());

#ifdef PN_PLATFORM_WINDOWS
					ss << "u_ShadowMaps[" << (tex_id - NEXT_VALID_TEXID) << "]";
#else
					ss << "u_ShadowMap" << (tex_id - NEXT_VALID_TEXID);
#endif

					pbr_shader->SetUniform(ss.str(), tex_id);
					ss.str("");
					ss.clear();

					ss << "u_Lights[" << i << "].shadowMapIdx";
					pbr_shader->SetUniform(ss.str(),
										   tex_id - static_cast<float>(NEXT_VALID_TEXID));
					ss.str("");
					ss.clear();

					++tex_id;
				} else {
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

				// For spotlights
				ss << "u_Lights[" << i << "].innerCutoff";
				pbr_shader->SetUniform(ss.str(), glm::cos(glm::radians(l.inner_angle)));
				ss.str("");
				ss.clear();

				ss << "u_Lights[" << i << "].outerCutoff";
				pbr_shader->SetUniform(ss.str(), glm::cos(glm::radians(l.outer_angle)));
				ss.str("");
				ss.clear();

				ss << "u_Lights[" << i << "].direction";
				pbr_shader->SetUniform(ss.str(),
									   l.direction); // or l.direction if you renamed it
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

			pbr_shader->SetUniform("DEBUG_TYPE",
								   (float)GraphicsSettings::get().DEBUG_PBR_MAP_TYPE);

			pbr_shader->SetUniform("u_NumShadowMaps",
								   (tex_id - NEXT_VALID_TEXID) * 1.f);

			pbr_shader->SetUniform("gPos", 0);
			pbr_shader->SetUniform("gCol", 1);
			pbr_shader->SetUniform("gNorm", 2);
			pbr_shader->SetUniform("gMaterial", 3);
			pbr_shader->SetUniform("gEmission", 4);

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

			glActiveTexture(GL_TEXTURE0 + tex_id);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox::get().getIrradianceMap());
			pbr_shader->SetUniform("irradianceMap", tex_id++);

			glActiveTexture(GL_TEXTURE0 + tex_id);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox::get().getPrefilterMap());
			pbr_shader->SetUniform("prefilterMap", tex_id++);

			glActiveTexture(GL_TEXTURE0 + tex_id);
			glBindTexture(GL_TEXTURE_2D, Skybox::get().getBrdfLUT());
			pbr_shader->SetUniform("brdfLut", tex_id++);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after setting ibl uniforms: {}", err);
			}

			// #endif

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
		glBlitFramebuffer(0, 0, winWidth, winHeight, 0, 0, winWidth, winHeight,
						  GL_DEPTH_BUFFER_BIT, GL_NEAREST); // Copy depth only

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after blitting depth buffer: {}", err);
		}

		// Now final_fbo has depth info. Render skybox:
		{
			glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
			glDepthFunc(GL_LEQUAL); // Pass if depth <= existing depth
			glDepthMask(GL_FALSE);	// Don't write to depth buffer

			Skybox::get().render(scene->GetActiveCamera()->view(),
								 scene->GetActiveCamera()->projection());

			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LESS);
		}

		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after drawing skybox in lighting pass: {}", err);
		}
	}

	void WindowsRenderer::DebugPass(const glm::vec3& min_p, const glm::vec3& max_p,
									const glm::vec4& color,
									std::shared_ptr<Scene::SceneManager> scene) {
		if (!debug_VAO || !debug_shader)
			return;

		std::vector<float> verts;
		verts.reserve(24 * 7);

		// converts min/max into 8 corners,
		glm::vec3 v[8] = {{min_p.x, min_p.y, min_p.z}, {max_p.x, min_p.y, min_p.z}, {max_p.x, max_p.y, min_p.z}, {min_p.x, max_p.y, min_p.z}, {min_p.x, min_p.y, max_p.z}, {max_p.x, min_p.y, max_p.z}, {max_p.x, max_p.y, max_p.z}, {min_p.x, max_p.y, max_p.z}};

		// edge index list (tells which pairs of the 8 AABB corners should be
		// connected to form the 12 box edges)
		int e[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
					 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

		// xyz and rgba
		auto push = [&](const glm::vec3& p, const glm::vec4& c) {
			verts.insert(verts.end(), {p.x, p.y, p.z, c.r, c.g, c.b, c.a});
		};

		// The loop iterates over all 12 edges by stepping i += 2, takes the two
		// endpoint corners for each edge via v[e[i]] and v[e[i+1]], and pushes both
		// endpoints
		for (int i = 0; i < 24; i += 2) {
			push(v[e[i]], color);
			push(v[e[i + 1]], color);
		}

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
					 GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		debug_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
		debug_shader->SetUniform("u_P", scene->GetActiveCamera()->projection());

		glDrawArrays(GL_LINES, 0, 24);

		glDepthMask(GL_TRUE);
		glBindVertexArray(0);
	}

	// DebugPassOBB - Draw oriented bounding box using pre-calculated 8 corners
	void WindowsRenderer::DebugPassOBB(const glm::vec3 corners[8],
									   const glm::vec4& color,
									   std::shared_ptr<Scene::SceneManager> scene) {
		if (!debug_VAO || !debug_shader)
			return;

		std::vector<float> verts;
		verts.reserve(24 * 7);

		// edge index list (tells which pairs of the 8 OBB corners should be connected
		// to form the 12 box edges)
		int e[24] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6,
					 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7};

		// xyz and rgba
		auto push = [&](const glm::vec3& p, const glm::vec4& c) {
			verts.insert(verts.end(), {p.x, p.y, p.z, c.r, c.g, c.b, c.a});
		};

		// The loop iterates over all 12 edges by stepping i += 2, takes the two
		// endpoint corners for each edge via v[e[i]] and v[e[i+1]], and pushes both
		// endpoints
		for (int i = 0; i < 24; i += 2) {
			push(corners[e[i]], color);
			push(corners[e[i + 1]], color);
		}

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
					 GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		debug_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
		debug_shader->SetUniform("u_P", scene->GetActiveCamera()->projection());

		glDrawArrays(GL_LINES, 0, 24);

		glDepthMask(GL_TRUE);
		glBindVertexArray(0);
	}

	void WindowsRenderer::PostProcessPass() {
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err before tone mapping pass: {}", err);
		}

		int postprocess_passes = 0;

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
				bloom_shader->SetUniform("threshold",
										 GraphicsSettings::get().bloom_threshold);
				glBindVertexArray(empty_vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			}

			// reset postprocess_passes for bloom blur
			postprocess_passes = 0;

			// blur bright areas above threshold
			{

				blur_shader->Bind();
				blur_shader->SetUniform("tex", 0);
				blur_shader->SetUniform("strength",
										GraphicsSettings::get().bloom_blur_strength);

				// on i = 0, bright areas are in pp_texture

				for (int i{}; i < GraphicsSettings::get().bloom_quality; ++i) {
					const unsigned int dest_fbo =
						postprocess_passes % 2 == 0 ? pp2_fbo : pp_fbo;
					const unsigned int src_tex =
						postprocess_passes % 2 == 0 ? pp_texture : pp2_texture;

					glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, src_tex);
					blur_shader->SetUniform("is_horizontal_pass", i % 2 ? 0.f : 1.f);
					glBindVertexArray(empty_vao);
					glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
					++postprocess_passes;
				}
			}

			// as blur pass will always be an even number (istg if yall put odd yall
			// trolling me), final blurred bright will be in pp_texture

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

				bloom_blend_shader->SetUniform("bloom_strength",
											   GraphicsSettings::get().bloom_strength);

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

		// tone mapping pass
		// do after bloom for HDR rendering!
		{
			const unsigned int dest_fbo =
				postprocess_passes % 2 == 0 ? pp_fbo : final_fbo;
			const unsigned int src_tex =
				postprocess_passes % 2 == 0 ? final_texture : pp_texture;

			glCheck(glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo));
			tone_shader->Bind();
			glCheck(glActiveTexture(GL_TEXTURE0));
			glCheck(glBindTexture(GL_TEXTURE_2D, src_tex));
			glCheck(tone_shader->SetUniform("tex", 0));
			glCheck(tone_shader->SetUniform(
				"exposure", GraphicsSettings::get().tone_mapping_exposure));
			glCheck(tone_shader->SetUniform(
				"toneMapMode",
				static_cast<float>(GraphicsSettings::get().tone_mapping_mode)));
			glCheck(glBindVertexArray(empty_vao));
			glCheck(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
			++postprocess_passes;
		}
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after tone mapping pass: {}", err);
		}

		// blur pass
		if (GraphicsSettings::get().blur_strength) {
			const int blur_iterations = GraphicsSettings::get().blur_quality;

			blur_shader->Bind();
			blur_shader->SetUniform("tex", 0);
			blur_shader->SetUniform("strength", GraphicsSettings::get().blur_strength);

			for (int i{}; i < blur_iterations; ++i) {
				const unsigned int dest_fbo =
					postprocess_passes % 2 == 0 ? pp_fbo : final_fbo;
				const unsigned int src_tex =
					postprocess_passes % 2 == 0 ? final_texture : pp_texture;

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
			const unsigned int dest_fbo =
				postprocess_passes % 2 == 0 ? pp_fbo : final_fbo;
			const unsigned int src_tex =
				postprocess_passes % 2 == 0 ? final_texture : pp_texture;

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
		// use passthrough to render pp_texture to final_texture if odd number of
		// passes
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
		;
		glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
		// glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
		// final_texture, 0);

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

		if (geometry_ibo) {
			glDeleteBuffers(1, &geometry_ibo);
			geometry_ibo = 0;
		}

		if (shadow_vao != 0) {
			glDeleteVertexArrays(1, &shadow_vao);
			shadow_vao = 0;
		}

		if (shadow_vbo != 0) {
			glDeleteBuffers(1, &shadow_vbo);
			shadow_vbo = 0;
		}

		if (shadow_ebo != 0) {
			glDeleteBuffers(1, &shadow_ebo);
			shadow_ebo = 0;
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
} // namespace PAIN

// #endif // PN_PLATFORM_WINDOWS