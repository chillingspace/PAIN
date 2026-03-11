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

namespace {
	constexpr int kMaxVolumetricLights = 4;
	constexpr int kVolumetricFirstShadowTextureUnit = 2;
	constexpr int kGBufferTextureCount = 5;
	constexpr int kFixedShadowTextureUnitStart = kGBufferTextureCount;
	constexpr int kMaxPbrShadowMaps = 4;
	constexpr int kIrradianceTextureUnit = kFixedShadowTextureUnitStart + kMaxPbrShadowMaps;
	constexpr int kPrefilterTextureUnit = kIrradianceTextureUnit + 1;
	constexpr int kBrdfLutTextureUnit = kPrefilterTextureUnit + 1;
	constexpr int kLightingTextureUnitsUsed = kBrdfLutTextureUnit + 1;

	const char* DescribeGlError(GLenum err) {
		switch (err) {
		case GL_NO_ERROR: return "GL_NO_ERROR";
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
		default: return "GL_UNKNOWN_ERROR";
		}
	}

	bool ValidateProgramForDraw(GLuint program, const char* label) {
		if (program == 0 || !glIsProgram(program)) {
			PN_CORE_ERROR("[GL] {} program handle is invalid: {}", label, program);
			return false;
		}

		glValidateProgram(program);
		GLint validateStatus = GL_FALSE;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &validateStatus);
		if (validateStatus == GL_TRUE) {
			return true;
		}

		GLint logLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		std::string infoLog(logLength > 0 ? logLength : 1, '\0');
		if (logLength > 1) {
			glGetProgramInfoLog(program, logLength, nullptr, infoLog.data());
		}

		PN_CORE_ERROR("[GL] {} program validation failed for {}: {}",
					  label, program, infoLog.c_str());
		return false;
	}

	void LogLightingDrawDiagnostics(GLuint program, GLuint vao, GLuint fbo, int usedTextureUnits) {
		static bool loggedOnce = false;
		if (loggedOnce) {
			return;
		}
		loggedOnce = true;

		GLint currentProgram = 0;
		GLint currentVao = 0;
		GLint drawFbo = 0;
		GLint activeTexture = 0;
		GLint framebufferBinding = 0;
		GLint maxFragTextureUnits = 0;
		GLint maxCombinedTextureUnits = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVao);
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFbo);
		glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebufferBinding);
		glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxFragTextureUnits);
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits);

		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		const GLenum framebufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);

		PN_CORE_ERROR(
			"[GL] Lighting draw diagnostics: program={} currentProgram={} isProgram={} vao={} currentVao={} isVao={} fbo={} currentFbo={} fboStatus=0x{:x} usedTextureUnits={} maxFragTextureUnits={} maxCombinedTextureUnits={}",
			program,
			currentProgram,
			glIsProgram(program),
			vao,
			currentVao,
			glIsVertexArray(vao),
			fbo,
			drawFbo,
			framebufferStatus,
			usedTextureUnits,
			maxFragTextureUnits,
			maxCombinedTextureUnits);

		const GLint previousActiveTexture = activeTexture;
		for (int unit = 0; unit < usedTextureUnits; ++unit) {
			glActiveTexture(GL_TEXTURE0 + unit);
			GLint tex2d = 0;
			GLint texCube = 0;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &tex2d);
			glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &texCube);
			PN_CORE_ERROR("[GL] Lighting draw texture unit {}: tex2D={} texCube={}",
						  unit, tex2d, texCube);
		}
		glActiveTexture(previousActiveTexture);
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferBinding);
	}

	void LogDepthBlitDiagnostics(GLuint readFbo, GLuint drawFbo) {
		static bool loggedOnce = false;
		if (loggedOnce) {
			return;
		}
		loggedOnce = true;

		GLint previousReadFbo = 0;
		GLint previousDrawFbo = 0;
		GLint previousRenderbuffer = 0;
		glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFbo);
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDrawFbo);
		glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, readFbo);
		const GLenum readStatus = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, drawFbo);
		const GLenum drawStatus = glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);

		GLint readType = GL_NONE;
		GLint readName = 0;
		GLint drawType = GL_NONE;
		GLint drawName = 0;
		GLint drawSamples = 0;

		glGetFramebufferAttachmentParameteriv(
			GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &readType);
		glGetFramebufferAttachmentParameteriv(
			GL_READ_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &readName);
		glGetFramebufferAttachmentParameteriv(
			GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &drawType);
		glGetFramebufferAttachmentParameteriv(
			GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &drawName);

		if (drawType == GL_RENDERBUFFER) {
			glBindRenderbuffer(GL_RENDERBUFFER, static_cast<GLuint>(drawName));
			glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &drawSamples);
		}

		PN_CORE_ERROR(
			"[GL] Depth blit diagnostics: readFbo={} status=0x{:x} depthType=0x{:x} depthObj={} drawFbo={} status=0x{:x} depthType=0x{:x} depthObj={} drawSamples={}",
			readFbo, readStatus, readType, readName,
			drawFbo, drawStatus, drawType, drawName, drawSamples);

		glBindRenderbuffer(GL_RENDERBUFFER, previousRenderbuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, previousReadFbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDrawFbo);
	}

	struct PackedVolumetricLight {
		const PAIN::Light* light = nullptr;
		int shadowTextureUnit = -1;
		int shadowMapIdx = -1;
	};

	struct CandidateVolumetricLight {
		std::string key;
		const PAIN::Light* light = nullptr;
		bool hasShadowMap = false;
		bool inCameraView = false;
		bool hysteresisActive = false;
		float screenCoverage = 0.0f;
		float viewScore = std::numeric_limits<float>::max();
		float distToCamera = std::numeric_limits<float>::max();
	};

	struct VolumetricUniformNames {
		std::array<std::string, kMaxVolumetricLights> position;
		std::array<std::string, kMaxVolumetricLights> intensity;
		std::array<std::string, kMaxVolumetricLights> view;
		std::array<std::string, kMaxVolumetricLights> projection;
		std::array<std::string, kMaxVolumetricLights> shadowMapIdx;
		std::array<std::string, kMaxVolumetricLights> type;
		std::array<std::string, kMaxVolumetricLights> direction;
		std::array<std::string, kMaxVolumetricLights> innerCutoff;
		std::array<std::string, kMaxVolumetricLights> outerCutoff;
	};

	const VolumetricUniformNames& GetVolumetricUniformNames() {
		static const VolumetricUniformNames names = [] {
			VolumetricUniformNames out{};
			for (int i = 0; i < kMaxVolumetricLights; ++i) {
				const std::string prefix = "u_Lights[" + std::to_string(i) + "].";
				out.position[i] = prefix + "position";
				out.intensity[i] = prefix + "L";
				out.view[i] = prefix + "V";
				out.projection[i] = prefix + "P";
				out.shadowMapIdx[i] = prefix + "shadowMapIdx";
				out.type[i] = prefix + "type";
				out.direction[i] = prefix + "direction";
				out.innerCutoff[i] = prefix + "innerCutoff";
				out.outerCutoff[i] = prefix + "outerCutoff";
			}
			return out;
		}();

		return names;
	}

	bool IsSphereInsideFrustum(const PAIN::Frustum& frustum, const glm::vec3& center, float radius) {
		const PAIN::Plane* planes[6] = {
			&frustum.leftFace, &frustum.rightFace,
			&frustum.bottomFace, &frustum.topFace,
			&frustum.nearFace, &frustum.farFace
		};

		for (const PAIN::Plane* plane : planes) {
			if (plane->getSignedDistanceToPlane(center) < -radius) {
				return false;
			}
		}
		return true;
	}

	bool IsPointInsideCameraView(const PAIN::Camera& camera, const glm::vec3& point) {
		const glm::vec4 clipPos = camera.projection() * camera.view() * glm::vec4(point, 1.0f);
		if (clipPos.w <= 0.0f) {
			return false;
		}

		const glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
		return ndc.z >= -1.0f && ndc.z <= 1.0f &&
			   ndc.x >= -1.0f && ndc.x <= 1.0f &&
			   ndc.y >= -1.0f && ndc.y <= 1.0f;
	}

	float GetVolumetricInfluenceRadius(const PAIN::Light& light, float volumetricMaxDistance) {
		if (light.type == PAIN::Light::TYPES::POINT) {
			return std::max(0.5f, std::min(light.far_plane, volumetricMaxDistance));
		}
		if (light.type == PAIN::Light::TYPES::SPOTLIGHT) {
			const float coneLength = std::max(0.5f, std::min(light.far_plane, volumetricMaxDistance));
			const float coneRadius = std::tan(glm::radians(light.outer_angle)) * coneLength;
			return std::sqrt((coneLength * 0.5f) * (coneLength * 0.5f) + coneRadius * coneRadius);
		}
		return std::max(0.5f, volumetricMaxDistance);
	}

	glm::vec3 GetVolumetricInfluenceCenter(const PAIN::Light& light, float volumetricMaxDistance) {
		if (light.type == PAIN::Light::TYPES::SPOTLIGHT) {
			const float coneLength = std::max(0.5f, std::min(light.far_plane, volumetricMaxDistance));
			return light.position + glm::normalize(light.direction) * (coneLength * 0.5f);
		}
		return light.position;
	}

	float ComputeViewPriority(const PAIN::Camera& camera, const PAIN::Light& light, float volumetricMaxDistance) {
		const glm::vec3 influenceCenter = GetVolumetricInfluenceCenter(light, volumetricMaxDistance);
		const glm::vec3 toCenter = influenceCenter - camera.pos;
		const float distance = glm::length(toCenter);
		if (distance <= 0.0001f) {
			return 0.0f;
		}

		const glm::vec3 dirToCenter = toCenter / distance;
		const float facing = glm::clamp(glm::dot(glm::normalize(camera.forward), dirToCenter), -1.0f, 1.0f);
		const float anglePenalty = 1.0f - facing;
		return anglePenalty * 1000.0f + distance;
	}

	bool ProjectPointToNdc(const glm::mat4& vp, const glm::vec3& point, glm::vec3& ndcOut) {
		const glm::vec4 clipPos = vp * glm::vec4(point, 1.0f);
		if (clipPos.w <= 0.0001f) {
			return false;
		}

		ndcOut = glm::vec3(clipPos) / clipPos.w;
		return true;
	}

	std::array<glm::vec3, 12> GetSpotlightSamplePoints(const PAIN::Light& light, float volumetricMaxDistance) {
		const float coneLength = std::max(0.5f, std::min(light.far_plane, volumetricMaxDistance));
		const glm::vec3 forward = glm::normalize(light.direction);
		glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
		if (glm::abs(glm::dot(forward, up)) > 0.98f) {
			up = glm::vec3(1.0f, 0.0f, 0.0f);
		}

		const glm::vec3 right = glm::normalize(glm::cross(forward, up));
		const glm::vec3 basisUp = glm::normalize(glm::cross(right, forward));
		const glm::vec3 baseCenter = light.position + forward * coneLength;
		const float coneRadius = std::tan(glm::radians(light.outer_angle)) * coneLength;

		return {
			light.position,
			light.position + forward * (coneLength * 0.33f),
			light.position + forward * (coneLength * 0.66f),
			baseCenter,
			baseCenter + right * coneRadius,
			baseCenter - right * coneRadius,
			baseCenter + basisUp * coneRadius,
			baseCenter - basisUp * coneRadius,
			baseCenter + glm::normalize(right + basisUp) * coneRadius,
			baseCenter + glm::normalize(right - basisUp) * coneRadius,
			baseCenter + glm::normalize(-right + basisUp) * coneRadius,
			baseCenter + glm::normalize(-right - basisUp) * coneRadius
		};
	}

	struct VolumetricVisibilityMetrics {
		bool visible = false;
		float coverage = 0.0f;
		float viewScore = std::numeric_limits<float>::max();
	};

	VolumetricVisibilityMetrics ComputeVolumetricVisibilityMetrics(const PAIN::Camera& camera,
		const PAIN::Frustum& frustum, const PAIN::Light& light, float volumetricMaxDistance) {
		VolumetricVisibilityMetrics metrics{};
		const glm::mat4 vp = camera.projection() * camera.view();
		float minX = 1.0f;
		float minY = 1.0f;
		float maxX = 0.0f;
		float maxY = 0.0f;
		bool anyOnScreen = false;
		int validProjectedPoints = 0;

		if (light.type == PAIN::Light::TYPES::SPOTLIGHT) {
			const auto samplePoints = GetSpotlightSamplePoints(light, volumetricMaxDistance);
			for (const glm::vec3& samplePoint : samplePoints) {
				const glm::vec3 toPoint = samplePoint - camera.pos;
				const float distance = glm::length(toPoint);
				if (distance > 0.0001f) {
					const glm::vec3 dirToPoint = toPoint / distance;
					const float facing = glm::clamp(glm::dot(glm::normalize(camera.forward), dirToPoint), -1.0f, 1.0f);
					const float anglePenalty = 1.0f - facing;
					metrics.viewScore = std::min(metrics.viewScore, anglePenalty * 1000.0f + distance);
				}

				glm::vec3 ndcPoint{};
				if (!ProjectPointToNdc(vp, samplePoint, ndcPoint)) {
					continue;
				}

				++validProjectedPoints;
				const bool onScreen =
					ndcPoint.z >= -1.0f && ndcPoint.z <= 1.0f &&
					ndcPoint.x >= -1.0f && ndcPoint.x <= 1.0f &&
					ndcPoint.y >= -1.0f && ndcPoint.y <= 1.0f;
				anyOnScreen = anyOnScreen || onScreen;

				const float uvX = glm::clamp(ndcPoint.x * 0.5f + 0.5f, 0.0f, 1.0f);
				const float uvY = glm::clamp(ndcPoint.y * 0.5f + 0.5f, 0.0f, 1.0f);
				minX = std::min(minX, uvX);
				minY = std::min(minY, uvY);
				maxX = std::max(maxX, uvX);
				maxY = std::max(maxY, uvY);
			}

			if (validProjectedPoints >= 2) {
				const float width = std::max(0.0f, maxX - minX);
				const float height = std::max(0.0f, maxY - minY);
				metrics.coverage = width * height;
				metrics.visible = anyOnScreen || metrics.coverage > 0.0005f;
			}

			if (!metrics.visible) {
				const glm::vec3 influenceCenter = GetVolumetricInfluenceCenter(light, volumetricMaxDistance);
				const float influenceRadius = GetVolumetricInfluenceRadius(light, volumetricMaxDistance);
				metrics.visible = IsSphereInsideFrustum(frustum, influenceCenter, influenceRadius);
			}

			if (metrics.viewScore == std::numeric_limits<float>::max()) {
				metrics.viewScore = ComputeViewPriority(camera, light, volumetricMaxDistance);
			}
			return metrics;
		}

		const glm::vec3 influenceCenter = GetVolumetricInfluenceCenter(light, volumetricMaxDistance);
		const float influenceRadius = GetVolumetricInfluenceRadius(light, volumetricMaxDistance);
		metrics.visible = IsSphereInsideFrustum(frustum, influenceCenter, influenceRadius) ||
						  IsPointInsideCameraView(camera, light.position);
		metrics.viewScore = ComputeViewPriority(camera, light, volumetricMaxDistance);

		glm::vec3 ndcCenter{};
		if (ProjectPointToNdc(vp, influenceCenter, ndcCenter)) {
			const float approxRadiusNdc = glm::clamp(influenceRadius / std::max(glm::length(influenceCenter - camera.pos), 0.5f), 0.01f, 0.75f);
			const float approxRadiusUv = approxRadiusNdc * 0.5f;
			metrics.coverage = glm::pi<float>() * approxRadiusUv * approxRadiusUv;
		}

		return metrics;
	}

	float ComputeVolumetricViewPriority(const PAIN::Camera& camera, const PAIN::Light& light,
										const std::string& key,
										const std::unordered_map<std::string, int>& selectionTtl,
										float volumetricMaxDistance,
										float baseScore) {
		float bestScore = baseScore;
		if (bestScore == std::numeric_limits<float>::max()) {
			bestScore = ComputeViewPriority(camera, light, volumetricMaxDistance);
		}

		const auto ttlIt = selectionTtl.find(key);
		if (ttlIt != selectionTtl.end() && ttlIt->second > 0) {
			bestScore = std::max(0.0f, bestScore - 250.0f);
		}
		return bestScore;
	}
}

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
		// SAVE TEXTURE STATE
		// ========================================
		GLint activeTextureUnit = 0;
		GLint previousTex2D = 0;
		GLint previousTexCube = 0;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTextureUnit);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTex2D);
		glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousTexCube);

		PN_CORE_TRACE("Texture load started, active unit: GL_TEXTURE{}", activeTextureUnit - GL_TEXTURE0);

		// ========================================
		// RESET TO TEXTURE UNIT 0 FOR LOADING
		// ========================================
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);

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
		// RESTORE TEXTURE STATE
		// ========================================
		glActiveTexture(activeTextureUnit);
		glBindTexture(GL_TEXTURE_2D, previousTex2D);
		glBindTexture(GL_TEXTURE_CUBE_MAP, previousTexCube);
	}

	void WindowsRenderer::initSceneVbo() {
		if (!geometry_vbo) return;

		// Full rebuild path: triggered on delete or scene load/clear
		if (needsFullRebuild) {
			needsFullRebuild = false;
			currentVertexCount = 0;
			currentIndexCount = 0;
			clearBuffers(); // clears instanced_offsets too

			// Pre-allocate with GL_DYNAMIC_DRAW since we'll be appending
			glBindVertexArray(geometry_vao);
			glBindBuffer(GL_ARRAY_BUFFER, geometry_vbo);
			glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry_ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

			glBindVertexArray(shadow_vao);
			glBindBuffer(GL_ARRAY_BUFFER, shadow_vbo);
			glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadow_ebo);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

			glBindVertexArray(0);
		}

		// Collect ONLY new models not yet in instanced_offsets
		std::vector<Assets::Vertex>  newVertices;
		std::vector<unsigned int>    newIndices;

		unsigned int vertexOffset = currentVertexCount;
		unsigned int indexOffset = currentIndexCount;

		auto ecs = services->get<ECS::Controller>();

		for (auto registryId : ecs->getAllRegistryIDs()) {
			auto& registry = ecs->getRegistry(registryId);
			auto view = registry.view<ModelRenderer>();

			for (auto e : view) {
				auto mdl = ecs->getEntityComponent<ModelRenderer>(e, registryId);
				if (!mdl.has_value())
					continue;

				auto mdl_opt = services->get<Assets::Manager>()->getAsset<Assets::Model>(
					mdl.value().get().modelGUID);
				if (!mdl_opt.has_value() || mdl_opt.value()->type != Assets::Type::Model)
					continue;
				const auto& modelAsset = mdl_opt.value();

				// SKIP if already on GPU
				if (instanced_offsets.find(modelAsset->vpath) != instanced_offsets.end())
					continue;

				// Register offset BEFORE appending so it's atomic
				instanced_offsets[modelAsset->vpath] = {
					indexOffset, (unsigned int)modelAsset->indices.size() };

				for (const auto& v : modelAsset->vertices)
					newVertices.push_back(v);

				for (unsigned int idx : modelAsset->indices)
					newIndices.push_back(vertexOffset + idx);

				vertexOffset += (unsigned int)modelAsset->vertices.size();
				indexOffset += (unsigned int)modelAsset->indices.size();
			}
		}

		// Nothing new to upload - exit early, no GPU calls
		if (newVertices.empty()) {
			PN_CORE_INFO("initSceneVbo: nothing new to upload, skipping.");
			return;
		}

		PN_CORE_INFO("initSceneVbo: appending {} vertices, {} indices",
			newVertices.size(), newIndices.size());

		// Resize GPU buffers first, then sub-upload the new slice
		auto appendToBuffer = [](GLuint buf, GLenum target,
			unsigned int existingBytes,
			const void* newData, unsigned int newBytes) {
				glBindBuffer(target, buf);

				// Get current size
				GLint currentSize = 0;
				glGetBufferParameteriv(target, GL_BUFFER_SIZE, &currentSize);

				unsigned int totalBytes = existingBytes + newBytes;

				if ((unsigned int)currentSize < totalBytes) {
					// Orphan + resize: copy old data into a temp, re-upload everything
					std::vector<uint8_t> temp(currentSize);
					if (currentSize > 0) {
						void* ptr = glMapBufferRange(target, 0, currentSize, GL_MAP_READ_BIT);
						if (ptr) {
							memcpy(temp.data(), ptr, currentSize);
							glUnmapBuffer(target);
						}
					}
					glBufferData(target, totalBytes, nullptr, GL_DYNAMIC_DRAW); // orphan
					if (currentSize > 0)
						glBufferSubData(target, 0, currentSize, temp.data());
				}

				// Append new data at the end
				glBufferSubData(target, existingBytes, newBytes, newData);
			};

		unsigned int existingVertexBytes = currentVertexCount * sizeof(Assets::Vertex);
		unsigned int newVertexBytes = (unsigned int)(newVertices.size() * sizeof(Assets::Vertex));
		unsigned int existingIndexBytes = currentIndexCount * sizeof(unsigned int);
		unsigned int newIndexBytes = (unsigned int)(newIndices.size() * sizeof(unsigned int));

		// Append to geometry buffers
		glBindVertexArray(geometry_vao);
		appendToBuffer(geometry_vbo, GL_ARRAY_BUFFER,
			existingVertexBytes, newVertices.data(), newVertexBytes);
		appendToBuffer(geometry_ebo, GL_ELEMENT_ARRAY_BUFFER,
			existingIndexBytes, newIndices.data(), newIndexBytes);

		// Append to shadow buffers
		glBindVertexArray(shadow_vao);
		appendToBuffer(shadow_vbo, GL_ARRAY_BUFFER,
			existingVertexBytes, newVertices.data(), newVertexBytes);
		appendToBuffer(shadow_ebo, GL_ELEMENT_ARRAY_BUFFER,
			existingIndexBytes, newIndices.data(), newIndexBytes);

		// Update IBO if instancing enabled
		if (GS.use_instanced_rendering && geometry_ibo) {
			glBindBuffer(GL_ARRAY_BUFFER, geometry_ibo);
			glBufferData(GL_ARRAY_BUFFER,
				instanced_offsets.size() * sizeof(IBOData),
				nullptr, GL_DYNAMIC_DRAW);
		}

		glBindVertexArray(0);

		// Commit new counts
		currentVertexCount = vertexOffset;
		currentIndexCount = indexOffset;

		PN_CORE_INFO("initSceneVbo: done. Total: {} vertices, {} indices",
			currentVertexCount, currentIndexCount);
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
		std::filesystem::path volumetric_path = "engine/shaders/volumetric.vert";
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
		std::filesystem::path volumetric_path = "engine\\shaders\\android_volumetric.vert";
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

		// Minimap wall shader
#ifdef PN_PLATFORM_WINDOWS
		std::filesystem::path minimap_wall_path = "engine/shaders/minimap_wall.vert";
#else
		std::filesystem::path minimap_wall_path = "engine\\shaders\\android_minimap_wall.vert";
#endif
		shader_opt = assets_loader->getAsset<Assets::Shader>(minimap_wall_path);
		minimap_wall_shader = shader_opt.has_value() ? shader_opt.value() : minimap_wall_shader;

		if (!minimap_wall_shader || minimap_wall_shader->GetRendererID() == 0) {
			PN_CORE_WARN("Failed to create shader program for minimap_wall (non-fatal, will fall back to debug path)");
			minimap_wall_shader = nullptr;
		}

		shader_opt = assets_loader->getAsset<Assets::Shader>(volumetric_path);
		volumetric_shader = shader_opt.has_value() ? shader_opt.value() : volumetric_shader;

		if (!volumetric_shader || volumetric_shader->GetRendererID() == 0) {
			PN_CORE_WARN("Failed to create shader program for volumetric lighting (non-fatal)");
			volumetric_shader = nullptr;
		}
	}

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

	void WindowsRenderer::_initDeferredShadingBuffers() {
		PN_CORE_INFO("Initializing deferred shading buffers with size: {}x{}",
					 winWidth, winHeight);

		GLint previousFramebuffer = 0;
		GLint previousRenderbuffer = 0;
		GLint previousTexture2D = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
		glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture2D);

		auto checkFramebufferComplete = [](const char* label) -> bool {
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("{} is incomplete! Status: 0x{:x}", label, status);
				return false;
			}
			return true;
		};

		auto restoreBindingState = [&]() {
			glBindTexture(GL_TEXTURE_2D, previousTexture2D);
			glBindRenderbuffer(GL_RENDERBUFFER, previousRenderbuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
		};

		auto resetFramebufferResources = [&]() {
			if (ds_fbo) { glDeleteFramebuffers(1, &ds_fbo); ds_fbo = 0; }
			if (final_fbo) { glDeleteFramebuffers(1, &final_fbo); final_fbo = 0; }
			if (pp_fbo) { glDeleteFramebuffers(1, &pp_fbo); pp_fbo = 0; }
			if (pp2_fbo) { glDeleteFramebuffers(1, &pp2_fbo); pp2_fbo = 0; }
			glDeleteFramebuffers(static_cast<GLsizei>(volumetric_fbos.size()), volumetric_fbos.data());
			volumetric_fbos = {0, 0};
			if (minimap_fbo) { glDeleteFramebuffers(1, &minimap_fbo); minimap_fbo = 0; }
			if (minimap_texture) { glDeleteTextures(1, &minimap_texture); minimap_texture = 0; }
			if (minimap_rbo) { glDeleteRenderbuffers(1, &minimap_rbo); minimap_rbo = 0; }

			GLuint textures[] = { pos_texture, col_texture, norm_texture,
								  material_properties_texture, emission_texture,
								  ds_depth_texture, final_texture, pp_texture,
								  pp2_texture, volumetric_textures[0], volumetric_textures[1] };
			glDeleteTextures(10, textures);
			pos_texture = col_texture = norm_texture = material_properties_texture
				= emission_texture = ds_depth_texture = final_texture = pp_texture
				= pp2_texture = 0;
			volumetric_textures = {0, 0};
			volumetric_history_index = 0;
			volumetric_history_valid = false;
			volumetric_prev_vp = glm::mat4(1.0f);
			volumetric_prev_cam_pos = glm::vec3(0.0f);
			volumetric_prev_cam_forward = glm::vec3(0.0f, 0.0f, -1.0f);
			volumetric_frame_index = 0;
			volumetric_selection_ttl.clear();
			if (final_rbo) { glDeleteRenderbuffers(1, &final_rbo); final_rbo = 0; }
		};

		auto failInit = [&](const char* label) {
			PN_CORE_ERROR("Deferred renderer buffer initialization failed at {}", label);
			resetFramebufferResources();
			restoreBindingState();
		};

		// DELETE OLD RESOURCES FIRST
		resetFramebufferResources();

		if (winWidth == 0 || winHeight == 0) {
			PN_CORE_ERROR("Invalid window dimensions: {}x{}", winWidth, winHeight);
			restoreBindingState();
			return;
		}

		// === Final FBO/Texture For Deffered Shading ===
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

			glGenTextures(1, &ds_depth_texture);
			glBindTexture(GL_TEXTURE_2D, ds_depth_texture);
#ifdef PN_PLATFORM_ANDROID
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, winWidth, winHeight,
						 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
#else
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, winWidth, winHeight,
						 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
#endif
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
								   ds_depth_texture, 0);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("G-buffer FBO is incomplete! Status: 0x{:x}", status);
				failInit("G-buffer FBO");
				return;
			}
			PN_CORE_INFO("G-buffer FBO is complete");

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		{
			const float scale =
				glm::clamp(GraphicsSettings::get().volumetric_resolution_scale, 0.25f, 1.0f);
			volumetric_width = std::max(1, static_cast<int>(std::round(winWidth * scale)));
			volumetric_height = std::max(1, static_cast<int>(std::round(winHeight * scale)));

			glGenFramebuffers(static_cast<GLsizei>(volumetric_fbos.size()), volumetric_fbos.data());
			glGenTextures(static_cast<GLsizei>(volumetric_textures.size()), volumetric_textures.data());

			for (size_t i = 0; i < volumetric_fbos.size(); ++i) {
				glBindFramebuffer(GL_FRAMEBUFFER, volumetric_fbos[i]);
				glBindTexture(GL_TEXTURE_2D, volumetric_textures[i]);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, volumetric_width, volumetric_height, 0,
							 GL_RGBA, GL_FLOAT, nullptr);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
									   volumetric_textures[i], 0);

				if (!checkFramebufferComplete("Volumetric framebuffer")) {
					failInit("Volumetric framebuffer");
					return;
				}
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		// === Final VAO/Texture (final output, for rendering) ===
		{
			glGenFramebuffers(1, &final_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);

			glGenTextures(1, &final_texture);
			if (final_texture == 0) {
				PN_CORE_ERROR("Failed to create final texture");
				failInit("Final color texture allocation");
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
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, winWidth,
							  winHeight);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
							  GL_RENDERBUFFER, final_rbo);

			if (!checkFramebufferComplete("Final framebuffer")) {
				failInit("Final framebuffer");
				return;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// pp_texture for ping-pong if needed in post-processing
			glGenFramebuffers(1, &pp_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, pp_fbo);

			glGenTextures(1, &pp_texture);
			if (pp_texture == 0) {
				PN_CORE_ERROR("Failed to create final texture");
				failInit("Post-process texture allocation");
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

			if (!checkFramebufferComplete("Post-process framebuffer (pp_fbo)")) {
				failInit("Post-process framebuffer (pp_fbo)");
				return;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// pp2_texture for ping-pong if needed in post-processing
			glGenFramebuffers(1, &pp2_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, pp2_fbo);

			glGenTextures(1, &pp2_texture);
			if (pp2_texture == 0) {
				PN_CORE_ERROR("Failed to create final texture");
				failInit("Post-process texture allocation (pp2)");
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

			if (!checkFramebufferComplete("Post-process framebuffer (pp2_fbo)")) {
				failInit("Post-process framebuffer (pp2_fbo)");
				return;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// minimap framebuffer
			const glm::vec2 minimap_size = GraphicsSettings::get().minimap_size_px;
			minimap_width = std::max(64, static_cast<int>(minimap_size.x));
			minimap_height = std::max(64, static_cast<int>(minimap_size.y));

			glGenFramebuffers(1, &minimap_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, minimap_fbo);

			glGenTextures(1, &minimap_texture);
			glBindTexture(GL_TEXTURE_2D, minimap_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, minimap_width, minimap_height, 0,
				GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				minimap_texture, 0);

			glGenRenderbuffers(1, &minimap_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, minimap_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, minimap_width,
				minimap_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
				GL_RENDERBUFFER, minimap_rbo);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("Minimap framebuffer is incomplete");
				failInit("Minimap framebuffer");
				return;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		restoreBindingState();
	}

	void WindowsRenderer::_initGeometryBuffers()
	{
		// === Passthrough quad VAO/VBO ===
		{
			static constexpr float quadVertices[] = {
				-1.0f,  1.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f,
				 1.0f, -1.0f, 1.0f, 0.0f,
				-1.0f,  1.0f, 0.0f, 1.0f,
				 1.0f, -1.0f, 1.0f, 0.0f,
				 1.0f,  1.0f, 1.0f, 1.0f
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

		// === Geometry VAO/VBO/EBO ===
		{
			glGenVertexArrays(1, &geometry_vao);
			glBindVertexArray(geometry_vao);

			glGenBuffers(1, &geometry_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, geometry_vbo);

			glGenBuffers(1, &geometry_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geometry_ebo);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, pos));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, normal));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, uv));
			glEnableVertexAttribArray(2);
			glVertexAttribIPointer(3, 4, GL_INT, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, boneIndices));
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, boneWeights));
			glEnableVertexAttribArray(4);
			glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, tangent));
			glEnableVertexAttribArray(5);
			glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, bitangent));
			glEnableVertexAttribArray(6);

			glBindVertexArray(0);
		}

		// === Shadow VAO/VBO/EBO ===
		{
			glGenVertexArrays(1, &shadow_vao);
			glBindVertexArray(shadow_vao);

			glGenBuffers(1, &shadow_vbo);
			glBindBuffer(GL_ARRAY_BUFFER, shadow_vbo);

			glGenBuffers(1, &shadow_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, shadow_ebo);

			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, pos));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, normal));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Assets::Vertex), (void*)offsetof(Assets::Vertex, uv));
			glEnableVertexAttribArray(2);

			glBindVertexArray(0);
		}

		// === Debug VAO/VBO ===
		{
			glGenVertexArrays(1, &debug_VAO);
			glBindVertexArray(debug_VAO);

			glGenBuffers(1, &debug_VBO);
			glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

			glBindVertexArray(0);
		}
	}

	void WindowsRenderer::Init(std::shared_ptr<Services> app_services) {
		services = app_services;

		auto window_service = services->get<Window::Window>();
		winWidth = window_service->getFrameBuffer().x;
		winHeight = window_service->getFrameBuffer().y;

		initShaders();

		glGenVertexArrays(1, &empty_vao);
		if (empty_vao == 0) {
			PN_CORE_ERROR("Failed to create empty VAO");
			return;
		}

		_initDeferredShadingBuffers(); // FBOs/textures only
		_initGeometryBuffers();        // VAOs/VBOs � called ONCE, never on resize

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

			int bones_skipped{};
			if (!component.boneTransforms.empty()) {

				// Animation calculation now handled in animation system
				const auto& matrices = component.boneTransforms; // OR renderer.boneTransforms, depending on where you stored it

				if (!matrices.empty()) {
					for (size_t i = 0; i < matrices.size() && i < 100; ++i) { // 100 = MAX_BONES
						std::string uniform_name = "u_BoneMatrices[" + std::to_string(i) + "]";
						geometry_shader->SetUniform(uniform_name, matrices[i]);
					}
					geometry_shader->SetUniform("u_Animated", 1.0f);
				}

				//static std::vector<glm::mat4> boneMatrices;
				//static constexpr int MAX_BONES = 100;
				//boneMatrices.resize(MAX_BONES, glm::mat4(1.f));

				//// find local bone xforms relative to parent
				//// these mtx move this particular bone the specific amount RELATIVE to it's parent
				//// eg. how much a finger moves relative to the hand bone (not absolute positioning)
				//std::vector<glm::mat4> relative_poses(modelAsset->skeleton.size(), glm::mat4(1.f));

				//// each track controls a single bone's animation
				//for (const auto& [bone_name, track] : modelAsset->animations[component.currentAnimationIndex].track_map) {
				//	// find the animation keyframe corresponding to current animation time
				//	const auto key_it = std::lower_bound(track.begin(), track.end(), component.animationTime, [](const auto& key, const float t) {return key.time < t; });
				//	if (key_it == track.end())	PN_CORE_ERROR("Invalid iterator key_it in animation block in DrawGeometry in WindowsRenderer.cpp");

				//	// find the bone idx affected by current track
				//	std::string bone_name_copy = bone_name;  // Create copy for lambda
				//	const auto bone_it = std::find_if(modelAsset->skeleton.begin(), modelAsset->skeleton.end(),
				//		[&bone_name_copy](const Assets::Bone& b) {return b.name == bone_name_copy; });
				//	if (bone_it == modelAsset->skeleton.end()) {
				//		//PN_CORE_WARN("Bone does not exist for animation track {} for model {}. Skipped: {}", component.currentAnimationIndex, modelAsset->vpath, ++bones_skipped);
				//		//PN_CORE_ERROR("Invalid iterator bone_it in animation block in DrawGeometry in WindowsRenderer.cpp");
				//		continue;

				//	}
				//	const int bone_idx = std::distance(modelAsset->skeleton.begin(), bone_it);

				//	// get the xform matrix that applies to current vertex from current animation key
				//	const glm::mat4 scale = glm::scale(glm::mat4(1.f), key_it->scale);
				//	const glm::mat4 rotate = glm::mat4_cast(key_it->rotation);
				//	const glm::mat4 translate = glm::translate(glm::mat4(1.f), key_it->translation);
				//	glm::mat4 animated_pose = translate * rotate * scale;

				//	if (GraphicsSettings::get().interpolate_animation) {
				//		auto next_key_it = std::next(key_it);
				//		if (next_key_it != track.end()) {
				//			// interpolate between current keyframe pose and next keyframe pose
				//			const float t = (component.animationTime - key_it->time) / (next_key_it->time - key_it->time);
				//			const glm::vec3 i_scale = glm::mix(key_it->scale, next_key_it->scale, t);
				//			const glm::quat i_rotate = glm::slerp(key_it->rotation, next_key_it->rotation, t);
				//			const glm::vec3 i_translate = glm::mix(key_it->translation, next_key_it->translation, t);

				//			const glm::mat4 i_scale_mtx = glm::scale(glm::mat4(1.f), i_scale);
				//			const glm::mat4 i_rotate_mtx = glm::mat4_cast(i_rotate);
				//			const glm::mat4 i_translate_mtx = glm::translate(glm::mat4(1.f), i_translate);

				//			// update interpolated matrix
				//			animated_pose = i_translate_mtx * i_rotate_mtx * i_scale_mtx;
				//		}
				//	}

				//	relative_poses[bone_idx] = animated_pose;

				//	// multiply with bind pose mtx(the T shape thingy) for final xform matrix
				//	//boneMatrices[bone_idx] = animated_pose * glm::inverse(bone_it->bindPose);
				//}

				//// account for parent bone transformation
				//std::vector<glm::mat4> poses(modelAsset->skeleton.size());
				//for (int i{}; i < modelAsset->skeleton.size(); ++i) {
				//	// if is parent, no need
				//	if (modelAsset->skeleton[i].parent == -1) {
				//		poses[i] = relative_poses[i];
				//		continue;
				//	}

				//	// account for parent's xform
				//	poses[i] = poses[modelAsset->skeleton[i].parent] * relative_poses[i];
				//	//poses[i] = relative_poses[i] * poses[modelAsset->skeleton[i].parent];
				//}

				//// apply to bind pose (T pose)
				//for (int i{}; i < modelAsset->skeleton.size(); ++i) {
				//	boneMatrices[i] = poses[i] * modelAsset->skeleton[i].bindPose;
				//	//boneMatrices[i] = glm::inverse(modelAsset->skeleton[i].bindPose) * poses[i];
				//}

				//// populate animated bone xforms in shader
				//for (size_t i{}; i < boneMatrices.size(); ++i) {
				//	const std::string uniform_name = "u_BoneMatrices[" + std::to_string(i) + "]";
				//	geometry_shader->SetUniform(uniform_name, boneMatrices[i]);
				//}
			}
            else {
                //PN_CORE_TRACE("{} is not playing animation", modelAsset->vpath);
				geometry_shader->SetUniform("u_Animated", 0.0f);
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

	void WindowsRenderer::BeginMinimapPass(const glm::mat4& view,
		const glm::mat4& proj) {
		(void)view;
		(void)proj;

		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &minimap_prev_fbo);
		glGetIntegerv(GL_VIEWPORT, minimap_prev_viewport);
		glGetFloatv(GL_COLOR_CLEAR_VALUE, minimap_prev_clear_color);
		minimap_prev_depth_test = glIsEnabled(GL_DEPTH_TEST);
		minimap_state_saved = true;

		const glm::vec2 minimap_size = GraphicsSettings::get().minimap_size_px;
		const int target_width = std::max(64, static_cast<int>(minimap_size.x));
		const int target_height = std::max(64, static_cast<int>(minimap_size.y));

		if (target_width != minimap_width || target_height != minimap_height) {
			minimap_width = target_width;
			minimap_height = target_height;

			if (minimap_texture != 0) {
				glDeleteTextures(1, &minimap_texture);
				minimap_texture = 0;
			}

			if (minimap_rbo != 0) {
				glDeleteRenderbuffers(1, &minimap_rbo);
				minimap_rbo = 0;
			}

			glBindFramebuffer(GL_FRAMEBUFFER, minimap_fbo);

			glGenTextures(1, &minimap_texture);
			glBindTexture(GL_TEXTURE_2D, minimap_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, minimap_width, minimap_height, 0,
				GL_RGBA, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				minimap_texture, 0);

			glGenRenderbuffers(1, &minimap_rbo);
			glBindRenderbuffer(GL_RENDERBUFFER, minimap_rbo);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, minimap_width,
				minimap_height);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
				GL_RENDERBUFFER, minimap_rbo);

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("Minimap framebuffer is incomplete after resize! Status: 0x{:x}",
					status);
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, minimap_fbo);
		glViewport(0, 0, minimap_width, minimap_height);
		glEnable(GL_DEPTH_TEST);

		const float bg_alpha = glm::clamp(GraphicsSettings::get().minimap_background_alpha,
			0.0f, 1.0f);
		glClearColor(0.02f, 0.02f, 0.02f, bg_alpha);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void WindowsRenderer::EndMinimapPass() {
		if (!minimap_state_saved) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, winWidth, winHeight);
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(minimap_prev_fbo));
		glViewport(minimap_prev_viewport[0], minimap_prev_viewport[1],
			minimap_prev_viewport[2], minimap_prev_viewport[3]);
		glClearColor(minimap_prev_clear_color[0], minimap_prev_clear_color[1],
			minimap_prev_clear_color[2], minimap_prev_clear_color[3]);

		if (minimap_prev_depth_test) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}

		minimap_state_saved = false;
	}

	void WindowsRenderer::UploadMinimapWalls(const std::vector<glm::vec2>& worldXZVertices) {
		// Create VAO/VBO on first call
		if (minimap_wall_vao == 0) {
			glGenVertexArrays(1, &minimap_wall_vao);
			glGenBuffers(1, &minimap_wall_vbo);

			glBindVertexArray(minimap_wall_vao);
			glBindBuffer(GL_ARRAY_BUFFER, minimap_wall_vbo);

			// layout(location = 0) in vec2 aWorldXZ
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);

			glBindVertexArray(0);
		}

		minimap_wall_vertex_count = static_cast<GLsizei>(worldXZVertices.size());

		if (minimap_wall_vertex_count == 0) {
			return;
		}

		// Upload wall XZ data ONCE with GL_STATIC_DRAW (immutable until walls change)
		glBindBuffer(GL_ARRAY_BUFFER, minimap_wall_vbo);
		glBufferData(GL_ARRAY_BUFFER,
			minimap_wall_vertex_count * sizeof(glm::vec2),
			worldXZVertices.data(),
			GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void WindowsRenderer::DrawMinimapWalls(const glm::vec2& playerXZ,
									   const glm::vec2& transformCol0,
									   const glm::vec2& transformCol1,
									   const glm::vec2& invDoubleRadius,
									   const glm::vec2& ndcBase,
									   const glm::vec2& ndcScale,
									   const glm::vec4& color) {
		if (!minimap_wall_shader || minimap_wall_vao == 0 || minimap_wall_vertex_count == 0) {
			return;
		}

		minimap_wall_shader->Bind();

		// Set per-frame uniforms (7 uniforms, no per-vertex CPU work)
		minimap_wall_shader->SetUniform("u_PlayerXZ", playerXZ);
		minimap_wall_shader->SetUniform("u_TransformCol0", transformCol0);
		minimap_wall_shader->SetUniform("u_TransformCol1", transformCol1);
		minimap_wall_shader->SetUniform("u_InvDoubleRadius", invDoubleRadius);
		minimap_wall_shader->SetUniform("u_NdcBase", ndcBase);
		minimap_wall_shader->SetUniform("u_NdcScale", ndcScale);
		minimap_wall_shader->SetUniform("u_Color", color);

		glBindVertexArray(minimap_wall_vao);
		glDrawArrays(GL_TRIANGLES, 0, minimap_wall_vertex_count);
		glBindVertexArray(0);

		glUseProgram(0);
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
		glViewport(0, 0, winWidth, winHeight);
		glDisable(GL_BLEND);

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

			for (int shadowSlot = 0; shadowSlot < kMaxPbrShadowMaps; ++shadowSlot) {
				glActiveTexture(GL_TEXTURE0 + kFixedShadowTextureUnitStart + shadowSlot);
				glBindTexture(GL_TEXTURE_2D, 0);
#ifdef PN_PLATFORM_WINDOWS
				pbr_shader->SetUniform("u_ShadowMaps[" + std::to_string(shadowSlot) + "]",
									   kFixedShadowTextureUnitStart + shadowSlot);
#else
				pbr_shader->SetUniform("u_ShadowMap" + std::to_string(shadowSlot),
									   kFixedShadowTextureUnitStart + shadowSlot);
#endif
			}

			int shadowMapCount = 0;
			int i{};
			for (const Light& l : LightSources::get().getAll()) {
				std::stringstream ss;
				if (l.getShadowType() == Light::SHADOW_TYPES::MAPPED) {
					if (shadowMapCount >= kMaxPbrShadowMaps) {
						PN_CORE_WARN("[GL] Skipping extra mapped shadow light at index {} because PBR shadow map budget is {}",
									 i, kMaxPbrShadowMaps);
						ss << "u_Lights[" << i << "].shadowMapIdx";
						pbr_shader->SetUniform(ss.str(), -1.f);
						ss.str("");
						ss.clear();
					}
					else {
						glActiveTexture(GL_TEXTURE0 + kFixedShadowTextureUnitStart + shadowMapCount);
					glBindTexture(GL_TEXTURE_2D, l.getShadowTexture());

#ifdef PN_PLATFORM_WINDOWS
						ss << "u_ShadowMaps[" << shadowMapCount << "]";
#else
						ss << "u_ShadowMap" << shadowMapCount;
#endif

						pbr_shader->SetUniform(ss.str(), kFixedShadowTextureUnitStart + shadowMapCount);
						ss.str("");
						ss.clear();

						ss << "u_Lights[" << i << "].shadowMapIdx";
						pbr_shader->SetUniform(ss.str(), static_cast<float>(shadowMapCount));
						ss.str("");
						ss.clear();

						++shadowMapCount;
					}
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
								   shadowMapCount * 1.f);

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

			glActiveTexture(GL_TEXTURE0 + kIrradianceTextureUnit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox::get().getIrradianceMap());
			pbr_shader->SetUniform("irradianceMap", kIrradianceTextureUnit);

			glActiveTexture(GL_TEXTURE0 + kPrefilterTextureUnit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox::get().getPrefilterMap());
			pbr_shader->SetUniform("prefilterMap", kPrefilterTextureUnit);

			glActiveTexture(GL_TEXTURE0 + kBrdfLutTextureUnit);
			glBindTexture(GL_TEXTURE_2D, Skybox::get().getBrdfLUT());
			pbr_shader->SetUniform("brdfLut", kBrdfLutTextureUnit);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after setting ibl uniforms: {}", err);
			}

			auto abortLightingPass = [&]() {
				glEnable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE);
			};

			GLint maxFragTextureUnits = 0;
			GLint maxCombinedTextureUnits = 0;
			glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxFragTextureUnits);
			glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits);
			if (kLightingTextureUnitsUsed > maxFragTextureUnits || kLightingTextureUnitsUsed > maxCombinedTextureUnits) {
				PN_CORE_ERROR(
					"[GL] Lighting pass requires {} texture units but device only exposes {} fragment / {} combined texture units",
					kLightingTextureUnitsUsed, maxFragTextureUnits, maxCombinedTextureUnits);
				LogLightingDrawDiagnostics(
					pbr_shader ? pbr_shader->GetRendererID() : 0,
					passthrough_vao,
					final_fbo,
					kLightingTextureUnitsUsed);
				abortLightingPass();
				return;
			}

			static GLuint validatedLightingProgram = 0;
			if (validatedLightingProgram != (pbr_shader ? pbr_shader->GetRendererID() : 0) &&
				!ValidateProgramForDraw(pbr_shader ? pbr_shader->GetRendererID() : 0, "LightingPass")) {
				LogLightingDrawDiagnostics(
					pbr_shader ? pbr_shader->GetRendererID() : 0,
					passthrough_vao,
					final_fbo,
					kLightingTextureUnitsUsed);
				abortLightingPass();
				return;
			}
			validatedLightingProgram = pbr_shader ? pbr_shader->GetRendererID() : 0;

			if (!glIsVertexArray(passthrough_vao)) {
				PN_CORE_ERROR("[GL] LightingPass passthrough VAO is invalid: {}", passthrough_vao);
				LogLightingDrawDiagnostics(
					pbr_shader ? pbr_shader->GetRendererID() : 0,
					passthrough_vao,
					final_fbo,
					kLightingTextureUnitsUsed);
				abortLightingPass();
				return;
			}

			const GLenum finalFboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (finalFboStatus != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("[GL] LightingPass framebuffer {} is incomplete before draw: 0x{:x}",
							  final_fbo, finalFboStatus);
				LogLightingDrawDiagnostics(
					pbr_shader ? pbr_shader->GetRendererID() : 0,
					passthrough_vao,
					final_fbo,
					kLightingTextureUnitsUsed);
				abortLightingPass();
				return;
			}

			// #endif

			glBindVertexArray(passthrough_vao);
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after binding lighting VAO: {} ({})", err, DescribeGlError(err));
				LogLightingDrawDiagnostics(
					pbr_shader ? pbr_shader->GetRendererID() : 0,
					passthrough_vao,
					final_fbo,
					kLightingTextureUnitsUsed);
				abortLightingPass();
				return;
			}

			glDrawArrays(GL_TRIANGLES, 0, 6);

			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after drawing lighting pass: {} ({})", err, DescribeGlError(err));
				LogLightingDrawDiagnostics(
					pbr_shader ? pbr_shader->GetRendererID() : 0,
					passthrough_vao,
					final_fbo,
					kLightingTextureUnitsUsed);
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
			PN_CORE_ERROR("OpenGL err after blitting depth buffer: {} ({})", err, DescribeGlError(err));
			LogDepthBlitDiagnostics(ds_fbo, final_fbo);
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

	// DebugPass2D - Draw 2D rectangle outline for UI debugging
	void WindowsRenderer::DebugPass2D(const glm::vec2& min_p, const glm::vec2& max_p,
							  const glm::vec4& color) {
		if (!debug_VAO || !debug_shader)
			return;

		std::vector<float> verts;
		verts.reserve(8 * 7); // 4 lines * 2 points * 7 floats

		// Define the 4 corners of the rectangle
		glm::vec2 corners[4] = {
			{min_p.x, min_p.y}, // bottom-left
			{max_p.x, min_p.y}, // bottom-right
			{max_p.x, max_p.y}, // top-right
			{min_p.x, max_p.y}  // top-left
		};

		// Edge index list for rectangle (connect corners in order)
		int e[8] = {0, 1, 1, 2, 2, 3, 3, 0};

		// xyz and rgba (z=0 for 2D)
		auto push = [&](const glm::vec2& p, const glm::vec4& c) {
			verts.insert(verts.end(), {p.x, p.y, 0.0f, c.r, c.g, c.b, c.a});
		};

		// Push the 4 edges
		for (int i = 0; i < 8; i += 2) {
			push(corners[e[i]], color);
			push(corners[e[i + 1]], color);
		}

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
					 GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		// For 2D UI, use orthographic projection
		glm::mat4 ortho_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		debug_shader->SetUniform("u_V", glm::mat4(1.0f)); // identity view
		debug_shader->SetUniform("u_P", ortho_proj);

		glDrawArrays(GL_LINES, 0, 8);

		glBindVertexArray(0);
	}

	void WindowsRenderer::DebugPass2DLine(const glm::vec2& start_p,
		const glm::vec2& end_p,
		const glm::vec4& color) {
		if (!debug_VAO || !debug_shader)
			return;

		const std::array<float, 14> verts = {
			start_p.x, start_p.y, 0.0f, color.r, color.g, color.b, color.a,
			end_p.x, end_p.y, 0.0f, color.r, color.g, color.b, color.a,
		};

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
			GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		glm::mat4 ortho_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		debug_shader->SetUniform("u_V", glm::mat4(1.0f));
		debug_shader->SetUniform("u_P", ortho_proj);

		glDrawArrays(GL_LINES, 0, 2);

		glBindVertexArray(0);
	}

	void WindowsRenderer::DebugPass2DLines(
		const std::vector<glm::vec2>& lineVertices,
		const glm::vec4& color) {
		if (!debug_VAO || !debug_shader || lineVertices.size() < 2)
			return;

		const size_t vertexCount = lineVertices.size() - (lineVertices.size() % 2);
		std::vector<float> verts;
		verts.reserve(vertexCount * 7);

		for (size_t i = 0; i < vertexCount; ++i) {
			const glm::vec2& p = lineVertices[i];
			verts.insert(verts.end(), {p.x, p.y, 0.0f, color.r, color.g, color.b, color.a});
		}

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
			GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		glm::mat4 ortho_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		debug_shader->SetUniform("u_V", glm::mat4(1.0f));
		debug_shader->SetUniform("u_P", ortho_proj);

		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertexCount));

		glBindVertexArray(0);
	}

	void WindowsRenderer::DebugPass2DTriangleFilled(const glm::vec2& a,
		const glm::vec2& b,
		const glm::vec2& c,
		const glm::vec4& color) {
		if (!debug_VAO || !debug_shader)
			return;

		const std::array<float, 21> verts = {
			a.x, a.y, 0.0f, color.r, color.g, color.b, color.a,
			b.x, b.y, 0.0f, color.r, color.g, color.b, color.a,
			c.x, c.y, 0.0f, color.r, color.g, color.b, color.a,
		};

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
			GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		glm::mat4 ortho_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		debug_shader->SetUniform("u_V", glm::mat4(1.0f));
		debug_shader->SetUniform("u_P", ortho_proj);

		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindVertexArray(0);
	}

	void WindowsRenderer::DebugPass2DTrianglesFilled(
		const std::vector<glm::vec2>& triangleVertices,
		const glm::vec4& color) {
		if (!debug_VAO || !debug_shader || triangleVertices.size() < 3)
			return;

		const size_t vertexCount = triangleVertices.size() - (triangleVertices.size() % 3);
		std::vector<float> verts;
		verts.reserve(vertexCount * 7);

		for (size_t i = 0; i < vertexCount; ++i) {
			const glm::vec2& p = triangleVertices[i];
			verts.insert(verts.end(), {p.x, p.y, 0.0f, color.r, color.g, color.b, color.a});
		}

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
			GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		glm::mat4 ortho_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		debug_shader->SetUniform("u_V", glm::mat4(1.0f));
		debug_shader->SetUniform("u_P", ortho_proj);

		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexCount));

		glBindVertexArray(0);
	}

	void WindowsRenderer::DebugPass2DCircle(const glm::vec2& center_p,
		const glm::vec2& radius_ndc,
		const glm::vec4& color,
		int segments) {
		if (!debug_VAO || !debug_shader || segments < 8)
			return;

		std::vector<float> verts;
		verts.reserve(static_cast<size_t>(segments) * 14);

		for (int i = 0; i < segments; ++i) {
			const float a0 = (static_cast<float>(i) / static_cast<float>(segments)) * glm::two_pi<float>();
			const float a1 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * glm::two_pi<float>();

			const glm::vec2 p0(
				center_p.x + std::cos(a0) * radius_ndc.x,
				center_p.y + std::sin(a0) * radius_ndc.y);
			const glm::vec2 p1(
				center_p.x + std::cos(a1) * radius_ndc.x,
				center_p.y + std::sin(a1) * radius_ndc.y);

			verts.insert(verts.end(), {p0.x, p0.y, 0.0f, color.r, color.g, color.b, color.a});
			verts.insert(verts.end(), {p1.x, p1.y, 0.0f, color.r, color.g, color.b, color.a});
		}

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(),
			GL_DYNAMIC_DRAW);

		debug_shader->Bind();
		glm::mat4 ortho_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		debug_shader->SetUniform("u_V", glm::mat4(1.0f));
		debug_shader->SetUniform("u_P", ortho_proj);

		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(verts.size() / 7));

		glBindVertexArray(0);
	}

	void WindowsRenderer::VolumetricPass(std::shared_ptr<Scene::SceneManager> scene,
										 const LightSources& lights) {
#ifdef PN_PLATFORM_ANDROID
		(void)scene;
		(void)lights;
		return;
#else
		(void)lights;
		if (!volumetric_shader || volumetric_shader->GetRendererID() == 0)
			return;
		if (!GraphicsSettings::get().volumetric) {
			volumetric_history_valid = false;
			volumetric_selection_ttl.clear();
			return;
		}
		if (!LightSources::get().lightsOn) {
			volumetric_history_valid = false;
			volumetric_selection_ttl.clear();
			return;
		}
		if (volumetric_fbos[0] == 0 || volumetric_fbos[1] == 0 ||
			volumetric_textures[0] == 0 || volumetric_textures[1] == 0 ||
			ds_depth_texture == 0)
			return;

		auto& gs = GraphicsSettings::get();
		const int maxVolumetricLights =
			std::clamp(gs.volumetric_max_lights, 1, kMaxVolumetricLights);
		const int hysteresisFrames = std::max(0, gs.volumetric_selection_hysteresis_frames);

		for (auto it = volumetric_selection_ttl.begin(); it != volumetric_selection_ttl.end();) {
			if (it->second > 0) {
				--it->second;
			}
			if (it->second <= 0) {
				it = volumetric_selection_ttl.erase(it);
			}
			else {
				++it;
			}
		}

		std::vector<CandidateVolumetricLight> candidates;
		candidates.reserve(LightSources::get().getCount());
		auto cam = scene->GetActiveCamera();
		if (!cam) {
			return;
		}
		const Frustum cameraFrustum = cam->getFrustum();

		for (auto& [key, lRef] : LightSources::get().getAllWithKeys()) {
			const Light& l = lRef.get();
			if (key == "world" || key == "cam") {
				continue;
			}
			const bool hasShadowMap =
				l.getShadowType() == Light::SHADOW_TYPES::MAPPED && l.getShadowTexture() != 0;
			const bool supportsUnshadowedVolumetrics =
				l.type == Light::TYPES::SPOTLIGHT || l.type == Light::TYPES::POINT;

			if (!hasShadowMap && !supportsUnshadowedVolumetrics) {
				continue;
			}

			const glm::vec3 influenceCenter = GetVolumetricInfluenceCenter(l, gs.volumetric_max_dist);
			const VolumetricVisibilityMetrics visibility =
				ComputeVolumetricVisibilityMetrics(*cam, cameraFrustum, l, gs.volumetric_max_dist);
			const float distToCamera = glm::distance(cam->pos, influenceCenter);
			const bool hysteresisActive = volumetric_selection_ttl.contains(key);
			const bool effectivelyVisible = visibility.visible || hysteresisActive;

			candidates.push_back({
				key,
				&l,
				hasShadowMap,
				effectivelyVisible,
				hysteresisActive,
				visibility.coverage,
				ComputeVolumetricViewPriority(*cam, l, key, volumetric_selection_ttl, gs.volumetric_max_dist, visibility.viewScore),
				distToCamera
			});
		}

		std::sort(candidates.begin(), candidates.end(),
			[](const CandidateVolumetricLight& lhs, const CandidateVolumetricLight& rhs) {
				if (lhs.inCameraView != rhs.inCameraView) {
					return lhs.inCameraView && !rhs.inCameraView;
				}
				if (lhs.screenCoverage != rhs.screenCoverage) {
					return lhs.screenCoverage > rhs.screenCoverage;
				}
				if (lhs.hysteresisActive != rhs.hysteresisActive) {
					return lhs.hysteresisActive && !rhs.hysteresisActive;
				}
				if (lhs.viewScore != rhs.viewScore) {
					return lhs.viewScore < rhs.viewScore;
				}
				return lhs.distToCamera < rhs.distToCamera;
			});

		std::vector<PackedVolumetricLight> packedLights;
		packedLights.reserve(std::min<int>(maxVolumetricLights, static_cast<int>(candidates.size())));

		int nextShadowTextureUnit = kVolumetricFirstShadowTextureUnit;
		int nextShadowMapIdx = 0;
		for (const CandidateVolumetricLight& candidate : candidates) {
			if (static_cast<int>(packedLights.size()) >= maxVolumetricLights) {
				break;
			}
			if (!candidate.inCameraView) {
				break;
			}

			PackedVolumetricLight packed{};
			packed.light = candidate.light;
			if (candidate.hasShadowMap) {
				packed.shadowTextureUnit = nextShadowTextureUnit;
				packed.shadowMapIdx = nextShadowMapIdx;
				++nextShadowTextureUnit;
				++nextShadowMapIdx;
			}
			packedLights.push_back(packed);
			if (hysteresisFrames > 0) {
				volumetric_selection_ttl[candidate.key] = hysteresisFrames;
			}
		}

		if (packedLights.empty()) {
			volumetric_history_valid = false;
			volumetric_selection_ttl.clear();
			return;
		}

		const auto& uniformNames = GetVolumetricUniformNames();
		const glm::mat4 vp = cam->projection() * cam->view();
		const glm::mat4 invVP = glm::inverse(vp);
		const int previousHistoryIndex = volumetric_history_index;
		const int currentHistoryIndex = 1 - previousHistoryIndex;
		const float cameraPosDelta = glm::length(cam->pos - volumetric_prev_cam_pos);
		const float cameraDirDelta = 1.0f - glm::clamp(
			glm::dot(glm::normalize(cam->forward), glm::normalize(volumetric_prev_cam_forward)),
			-1.0f, 1.0f);
		const float motionPenalty = glm::clamp(cameraPosDelta * 0.08f + cameraDirDelta * 2.5f, 0.0f, 0.75f);
		const float historyBlend = volumetric_history_valid
			? glm::clamp(gs.volumetric_temporal_blend - motionPenalty, 0.0f, 0.95f)
			: 0.0f;

		// Render the expensive march into a low-resolution target first.
		glBindFramebuffer(GL_FRAMEBUFFER, volumetric_fbos[currentHistoryIndex]);
		glViewport(0, 0, volumetric_width, volumetric_height);
		glDisable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		volumetric_shader->Bind();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ds_depth_texture);
		volumetric_shader->SetUniform("u_DepthTex", 0);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, volumetric_textures[previousHistoryIndex]);
		volumetric_shader->SetUniform("u_HistoryTex", 1);

		for (size_t lightIdx = 0; lightIdx < packedLights.size(); ++lightIdx) {
			const PackedVolumetricLight& packed = packedLights[lightIdx];
			const Light& l = *packed.light;
			if (packed.shadowMapIdx >= 0) {
				glActiveTexture(GL_TEXTURE0 + packed.shadowTextureUnit);
				glBindTexture(GL_TEXTURE_2D, l.getShadowTexture());
				volumetric_shader->SetUniform(
					"u_ShadowMaps[" + std::to_string(packed.shadowMapIdx) + "]",
					packed.shadowTextureUnit);
				volumetric_shader->SetUniform(uniformNames.shadowMapIdx[lightIdx],
											  static_cast<float>(packed.shadowMapIdx));
			}
			else {
				volumetric_shader->SetUniform(uniformNames.shadowMapIdx[lightIdx], -1.0f);
			}
			volumetric_shader->SetUniform(uniformNames.position[lightIdx], l.position);
			volumetric_shader->SetUniform(uniformNames.view[lightIdx], l.view());
			volumetric_shader->SetUniform(uniformNames.projection[lightIdx], l.projection());
			volumetric_shader->SetUniform(uniformNames.type[lightIdx], static_cast<float>(l.type));
			volumetric_shader->SetUniform(uniformNames.intensity[lightIdx], l.L_intensity);
			volumetric_shader->SetUniform(uniformNames.direction[lightIdx], l.direction);
			volumetric_shader->SetUniform(uniformNames.innerCutoff[lightIdx],
										  glm::cos(glm::radians(l.inner_angle)));
			volumetric_shader->SetUniform(uniformNames.outerCutoff[lightIdx],
										  glm::cos(glm::radians(l.outer_angle)));
		}

		volumetric_shader->SetUniform("u_CamPos",  cam->pos);
		volumetric_shader->SetUniform("u_InvVP",   invVP);
		volumetric_shader->SetUniform("u_PrevVP", volumetric_prev_vp);
		volumetric_shader->SetUniform("u_NumLights", static_cast<int>(packedLights.size()));
		volumetric_shader->SetUniform("u_HistoryBlend", historyBlend);
		volumetric_shader->SetUniform("u_HistoryClamp", gs.volumetric_history_clamp);
		volumetric_shader->SetUniform("u_HistoryValid", volumetric_history_valid ? 1 : 0);
		volumetric_shader->SetUniform("u_FrameIndex", static_cast<int>(volumetric_frame_index++));

		volumetric_shader->SetUniform("u_VolumetricIntensity", gs.volumetric_intensity);
		volumetric_shader->SetUniform("u_VolumetricSteps", gs.volumetric_steps);
		volumetric_shader->SetUniform("u_VolumetricMaxDist",   gs.volumetric_max_dist);
		volumetric_shader->SetUniform("u_VolumetricScatter",   gs.volumetric_scatter);
		volumetric_shader->SetUniform("u_VolumetricJitterStrength", gs.volumetric_jitter_strength);

		glBindVertexArray(empty_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Composite the low-resolution result into the HDR scene.
		glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
		glViewport(0, 0, winWidth, winHeight);
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		passthrough_shader->Bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, volumetric_textures[currentHistoryIndex]);
		passthrough_shader->SetUniform("tex", 0);
		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glDisable(GL_BLEND);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		volumetric_prev_vp = vp;
		volumetric_prev_cam_pos = cam->pos;
		volumetric_prev_cam_forward = cam->forward;
		volumetric_history_index = currentHistoryIndex;
		volumetric_history_valid = true;

		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after VolumetricPass: {}", err);
		}
#endif
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
			gamma_shader->SetUniform("u_gamma", GraphicsSettings::get().gamma_value);
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

		if (minimap_wall_vao) {
			glDeleteVertexArrays(1, &minimap_wall_vao);
			minimap_wall_vao = 0;
		}

		if (minimap_wall_vbo) {
			glDeleteBuffers(1, &minimap_wall_vbo);
			minimap_wall_vbo = 0;
		}
		minimap_wall_vertex_count = 0;

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

		glDeleteFramebuffers(static_cast<GLsizei>(volumetric_fbos.size()), volumetric_fbos.data());
		glDeleteTextures(static_cast<GLsizei>(volumetric_textures.size()), volumetric_textures.data());
		volumetric_fbos = {0, 0};
		volumetric_textures = {0, 0};
		volumetric_history_index = 0;
		volumetric_history_valid = false;
		volumetric_prev_vp = glm::mat4(1.0f);
		volumetric_prev_cam_pos = glm::vec3(0.0f);
		volumetric_prev_cam_forward = glm::vec3(0.0f, 0.0f, -1.0f);
		volumetric_frame_index = 0;
		volumetric_selection_ttl.clear();

		TextRenderer::shutdown();
	}
} // namespace PAIN

// #endif // PN_PLATFORM_WINDOWS
