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
#include "CoreSystems/Renderer/TextureUnits.h"
#include "Systems/Render/MinimapStyle.h"
#include "CoreSystems/Windows/Window.h"
#include "ECS/Controller.h"
#include <cstring>
#include <random>

namespace {
	// Import TextureUnits namespace for cleaner code
	using namespace PAIN::TextureUnits;
	
	constexpr int kMaxPbrLights = 16;
	constexpr int kMaxVolumetricLights = 4;
	constexpr int kVolumetricFirstShadowTextureUnit = VolumetricPass::kShadowStart;
	constexpr int kGBufferTextureCount = LightingPass::kGBufferCount;
	constexpr int kFixedShadowTextureUnitStart = LightingPass::kShadowStart;
	constexpr int kMaxPbrShadowMaps = LightingPass::kMaxShadowMaps;
	constexpr int kIrradianceTextureUnit = LightingPass::kIrradiance;
	constexpr int kPrefilterTextureUnit = LightingPass::kPrefilter;
	constexpr int kBrdfLutTextureUnit = LightingPass::kBrdfLUT;
	constexpr int kLightingTextureUnitsUsed = LightingPass::kTotalUnits;
	constexpr GLuint kPbrLightUboBindingPoint = 0;

	struct alignas(16) PbrLightGpuData {
		glm::vec4 position_type = glm::vec4(0.0f); // xyz + light type
		glm::vec4 intensity_shadow = glm::vec4(0.0f, 0.0f, 0.0f, -1.0f); // rgb + shadow map index
		glm::vec4 direction_inner = glm::vec4(0.0f, 0.0f, -1.0f, 0.0f); // xyz + inner cutoff
		glm::vec4 outer_padding = glm::vec4(0.0f); // x = outer cutoff
		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
	};

	const char* DescribeGlError(GLenum err) {
		switch (err) {
		case GL_NO_ERROR: return "GL_NO_ERROR";
		case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
#ifdef GL_STACK_OVERFLOW
		case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
#endif
#ifdef GL_STACK_UNDERFLOW
		case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
#endif
		case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
#ifdef GL_INVALID_FRAMEBUFFER_OPERATION
		case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
#endif
		default: return "GL_UNKNOWN_ERROR";
		}
	}

#if defined(GL_TEXTURE_MAX_ANISOTROPY_EXT) && defined(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT)
	bool IsTextureAnisotropySupported() {
#ifdef PN_PLATFORM_WINDOWS
		return GLEW_EXT_texture_filter_anisotropic == GL_TRUE ||
			   GLEW_ARB_texture_filter_anisotropic == GL_TRUE;
#else
		const char* extensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
		return extensions != nullptr &&
			   std::strstr(extensions, "GL_EXT_texture_filter_anisotropic") != nullptr;
#endif
	}

	void TryApplyMaxAnisotropy(GLenum target) {
		static int supportState = -1;
		static GLfloat cachedMaxAniso = 1.0f;
		if (supportState < 0) {
			supportState = IsTextureAnisotropySupported() ? 1 : 0;
			if (supportState == 1) {
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &cachedMaxAniso);
				cachedMaxAniso = std::max(1.0f, cachedMaxAniso);
			}
		}
		if (supportState == 1) {
			glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, cachedMaxAniso);
		}
	}
#else
	void TryApplyMaxAnisotropy(GLenum) {}
#endif

	struct SavedGlState {
		GLint framebuffer = 0;
		GLint viewport[4] = {0, 0, 0, 0};
		GLfloat clearColor[4] = {0.f, 0.f, 0.f, 0.f};
		GLboolean depthTest = GL_FALSE;
		GLboolean depthMask = GL_TRUE;
		GLboolean blend = GL_FALSE;
		GLint blendSrcRgb = GL_ONE;
		GLint blendDstRgb = GL_ZERO;
		GLint blendSrcAlpha = GL_ONE;
		GLint blendDstAlpha = GL_ZERO;
	};

	SavedGlState CaptureGlState() {
		SavedGlState state{};
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &state.framebuffer);
		glGetIntegerv(GL_VIEWPORT, state.viewport);
		glGetFloatv(GL_COLOR_CLEAR_VALUE, state.clearColor);
		state.depthTest = glIsEnabled(GL_DEPTH_TEST);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &state.depthMask);
		state.blend = glIsEnabled(GL_BLEND);
		glGetIntegerv(GL_BLEND_SRC_RGB, &state.blendSrcRgb);
		glGetIntegerv(GL_BLEND_DST_RGB, &state.blendDstRgb);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &state.blendSrcAlpha);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &state.blendDstAlpha);
		return state;
	}

	void RestoreGlState(const SavedGlState& state) {
		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(state.framebuffer));
		glViewport(state.viewport[0], state.viewport[1], state.viewport[2], state.viewport[3]);
		glClearColor(state.clearColor[0], state.clearColor[1], state.clearColor[2], state.clearColor[3]);
		if (state.depthTest) {
			glEnable(GL_DEPTH_TEST);
		} else {
			glDisable(GL_DEPTH_TEST);
		}
		glDepthMask(state.depthMask == GL_TRUE ? GL_TRUE : GL_FALSE);
		if (state.blend) {
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
		glBlendFuncSeparate(state.blendSrcRgb, state.blendDstRgb, state.blendSrcAlpha, state.blendDstAlpha);
	}

	bool ValidateFramebufferBinding(GLuint fbo, const char* label) {
		GLint previousFramebuffer = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			PN_CORE_ERROR("{} is incomplete! Status: 0x{:x}", label, status);
			return false;
		}
		return true;
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

	// Pre-cached shadow map uniform names for volumetric pass (avoids per-frame string allocation)
	struct VolumetricShadowUniformNames {
		std::array<std::string, kMaxVolumetricLights> shadowSamplers;
	};

	const VolumetricShadowUniformNames& GetVolumetricShadowUniformNames() {
		static const VolumetricShadowUniformNames names = [] {
			VolumetricShadowUniformNames out{};
			for (int i = 0; i < kMaxVolumetricLights; ++i) {
#ifdef PN_PLATFORM_WINDOWS
				out.shadowSamplers[i] = "u_ShadowMaps[" + std::to_string(i) + "]";
#else
				out.shadowSamplers[i] = "u_ShadowMap" + std::to_string(i);
#endif
			}
			return out;
		}();
		return names;
	}

	struct PbrUniformNames {
		std::array<std::string, kMaxPbrShadowMaps> shadowSamplers;
	};

	const PbrUniformNames& GetPbrUniformNames() {
		static const PbrUniformNames names = [] {
			PbrUniformNames out{};
			for (int i = 0; i < kMaxPbrShadowMaps; ++i) {
#ifdef PN_PLATFORM_WINDOWS
				out.shadowSamplers[i] = "u_ShadowMaps[" + std::to_string(i) + "]";
#else
				out.shadowSamplers[i] = "u_ShadowMap" + std::to_string(i);
#endif
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
	namespace {
		std::string ToLowerAsciiCopy(std::string value) {
			std::transform(value.begin(), value.end(), value.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			return value;
		}

		bool ContainsAnyToken(const std::string& value, std::initializer_list<const char*> needles) {
			for (const char* needle : needles) {
				if (value.find(needle) != std::string::npos) {
					return true;
				}
			}
			return false;
		}

		bool IsLikelyPackedOrmPath(const std::filesystem::path& texturePath) {
			if (texturePath.empty()) {
				return false;
			}
			const std::string lower = ToLowerAsciiCopy(texturePath.lexically_normal().generic_string());
			return ContainsAnyToken(lower, {
				"occlusionroughnessmetallic",
				"metallicroughness",
				"_orm",
				"_mrao",
				"_rma",
				"_arm",
				"_mra"
			});
		}

		struct GeometryMaterialState {
			GLuint albedoTexture = 0;
			GLuint aoTexture = 0;
			GLuint normalTexture = 0;
			GLuint roughnessTexture = 0;
			GLuint metallicTexture = 0;
			GLuint emissionTexture = 0;
			glm::vec3 aoChannelMask = glm::vec3(1.0f, 0.0f, 0.0f);
			glm::vec3 roughnessChannelMask = glm::vec3(1.0f, 0.0f, 0.0f);
			glm::vec3 metallicChannelMask = glm::vec3(1.0f, 0.0f, 0.0f);
			glm::vec3 baseColor = glm::vec3(1.0f);
			float roughness = 0.5f;
			float metallic = 0.0f;
			bool useEmissionOverride = false;
			glm::vec3 emissionOverride = glm::vec3(0.0f);
		};

		GLuint ResolveMaterialTexture(const std::shared_ptr<Assets::Manager>& assetManager,
									 const MaterialInstance& material,
									 const std::filesystem::path& assetPath,
									 const Assets::GUID& overrideGuid,
									 bool enabled) {
			if (!enabled) {
				return 0;
			}

			auto texOpt = material.useOverrides
				? assetManager->getAsset<Assets::Texture>(overrideGuid)
				: assetManager->getAsset<Assets::Texture>(assetPath);
			return texOpt.has_value() && texOpt.value() ? texOpt.value()->gl_texture : 0u;
		}

		GeometryMaterialState BuildGeometryMaterialState(const std::shared_ptr<Assets::Manager>& assetManager,
											 const ModelRenderer& component,
											 size_t submeshIndex) {
			GeometryMaterialState state{};
			if (!component.cachedModelAsset || submeshIndex >= component.cachedModelAsset->submeshes.size()) {
				return state;
			}

			const auto& submesh = component.cachedModelAsset->submeshes[submeshIndex];
			if (submesh.materialIndex >= component.materials.size()) {
				return state;
			}

			const MaterialInstance& material = component.materials[submesh.materialIndex];
			state.useEmissionOverride = material.useOverrides;
			state.emissionOverride = material.emissiveOverride;

			// Resolve live material state every draw. Submesh caches are initialized once
			// and are not currently invalidated on editor/runtime material edits, so using
			// them here can produce stale G-buffer results.

			auto materialAssetOpt = assetManager->getAsset<Assets::Material>(material.materialGUID);
			auto materialAsset = materialAssetOpt.has_value() ? materialAssetOpt.value() : nullptr;
			if (!materialAsset) {
				return state;
			}

			const auto& gs = GraphicsSettings::get();
			state.albedoTexture = ResolveMaterialTexture(assetManager, material, materialAsset->albedoTexturePath,
				material.albedoTextureOverride, gs.DEBUG_USE_DIFFUSE_MAP);
			state.aoTexture = ResolveMaterialTexture(assetManager, material, materialAsset->aoTexturePath,
				material.aoTextureOverride, gs.DEBUG_USE_AO_MAP);
			state.normalTexture = ResolveMaterialTexture(assetManager, material, materialAsset->normalTexturePath,
				material.normalTextureOverride, gs.DEBUG_USE_NORMAL_MAP);
			state.roughnessTexture = ResolveMaterialTexture(assetManager, material, materialAsset->roughnessTexturePath,
				material.roughnessTextureOverride, gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP);
			state.metallicTexture = ResolveMaterialTexture(assetManager, material, materialAsset->metallicTexturePath,
				material.metallicTextureOverride, gs.DEBUG_USE_ROUGHNESSMETALLIC_MAP);
			state.emissionTexture = ResolveMaterialTexture(assetManager, material, materialAsset->emissiveTexturePath,
				material.emissiveTextureOverride, gs.DEBUG_USE_EMISSION_MAP);
			state.baseColor = material.useOverrides ? material.baseColorOverride : materialAsset->baseColor;
			state.metallic = material.useOverrides ? material.metallicOverride : materialAsset->metallic;
			state.roughness = material.useOverrides ? material.roughnessOverride : materialAsset->roughness;

			const std::string roughnessPathLower = ToLowerAsciiCopy(materialAsset->roughnessTexturePath.lexically_normal().generic_string());
			const std::string metallicPathLower = ToLowerAsciiCopy(materialAsset->metallicTexturePath.lexically_normal().generic_string());
			const std::string aoPathLower = ToLowerAsciiCopy(materialAsset->aoTexturePath.lexically_normal().generic_string());

			const bool sameRoughMetalTexture = state.roughnessTexture != 0 && state.roughnessTexture == state.metallicTexture;
			const bool sameAoRoughTexture = state.aoTexture != 0 && state.aoTexture == state.roughnessTexture;
			const bool sameRoughMetalPath = !roughnessPathLower.empty() && roughnessPathLower == metallicPathLower;
			const bool sameAoRoughPath = !aoPathLower.empty() && aoPathLower == roughnessPathLower;

			const bool sameRoughMetalOverride = material.useOverrides &&
				material.roughnessTextureOverride.IsValid() &&
				material.roughnessTextureOverride == material.metallicTextureOverride;
			const bool sameAoRoughOverride = material.useOverrides &&
				material.aoTextureOverride.IsValid() &&
				material.aoTextureOverride == material.roughnessTextureOverride;
			const bool allowPathHeuristics = !material.useOverrides;

			const bool roughMetalLinked = sameRoughMetalTexture ||
				sameRoughMetalOverride ||
				(allowPathHeuristics && sameRoughMetalPath);
			const bool aoRoughLinked = sameAoRoughTexture ||
				sameAoRoughOverride ||
				(allowPathHeuristics && sameAoRoughPath);

			const bool packedHint = (allowPathHeuristics && (
				IsLikelyPackedOrmPath(materialAsset->roughnessTexturePath) ||
				IsLikelyPackedOrmPath(materialAsset->metallicTexturePath) ||
				IsLikelyPackedOrmPath(materialAsset->aoTexturePath)
				)) || aoRoughLinked;

			const bool usePackedMetalRough = state.roughnessTexture != 0 &&
				state.metallicTexture != 0 &&
				roughMetalLinked &&
				(packedHint || aoRoughLinked);

			if (usePackedMetalRough) {
				// glTF ORM convention: R=AO, G=Roughness, B=Metallic.
				state.roughnessChannelMask = glm::vec3(0.0f, 1.0f, 0.0f);
				state.metallicChannelMask = glm::vec3(0.0f, 0.0f, 1.0f);
			}

			const bool usePackedAo = state.aoTexture != 0 &&
				aoRoughLinked &&
				(usePackedMetalRough || packedHint);

			if (usePackedAo) {
				state.aoChannelMask = glm::vec3(1.0f, 0.0f, 0.0f);
			}

			return state;
		}

		void ApplyGeometryMaterialState(const std::shared_ptr<Assets::Shader>& geometry_shader,
									  const GeometryMaterialState& state) {
			geometry_shader->SetUniform("u_DecodeAlbedoInShader",
				GraphicsSettings::get().decode_albedo_in_shader ? 1.0f : 0.0f);
			geometry_shader->SetUniform("material.rough", state.roughness);
			geometry_shader->SetUniform("material.metal", state.metallic);
			geometry_shader->SetUniform("material.color", state.baseColor);
			geometry_shader->SetUniform("u_UseEmissionOverride", state.useEmissionOverride ? 1.0f : 0.0f);
			geometry_shader->SetUniform("u_EmissionOverride",
				state.useEmissionOverride ? state.emissionOverride : glm::vec3(0.0f));
			geometry_shader->SetUniform("material.ao_channel_mask", state.aoChannelMask);
			geometry_shader->SetUniform("material.roughness_channel_mask", state.roughnessChannelMask);
			geometry_shader->SetUniform("material.metallic_channel_mask", state.metallicChannelMask);

			// Use centralized texture unit constants from TextureUnits.h
			// Geometry pass uses units 6-11 for materials
			const bool hasTexture = state.albedoTexture != 0;
			geometry_shader->SetUniform("material.useTex", hasTexture ? 1.0f : 0.0f);
			if (hasTexture) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kAlbedo);
				glBindTexture(GL_TEXTURE_2D, state.albedoTexture);
				geometry_shader->SetUniform("material.tex", GeometryPass::kAlbedo);
			}

			const bool useAo = state.aoTexture != 0;
			geometry_shader->SetUniform("material.use_ao", useAo ? 1.0f : 0.0f);
			if (useAo) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kAo);
				glBindTexture(GL_TEXTURE_2D, state.aoTexture);
				geometry_shader->SetUniform("material.ao_map", GeometryPass::kAo);
			}

			const bool useNormal = state.normalTexture != 0;
			geometry_shader->SetUniform("material.use_normal", useNormal ? 1.0f : 0.0f);
			if (useNormal) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kNormal);
				glBindTexture(GL_TEXTURE_2D, state.normalTexture);
				geometry_shader->SetUniform("material.normal_map", GeometryPass::kNormal);
			}

			const bool useRoughness = state.roughnessTexture != 0;
			geometry_shader->SetUniform("material.use_roughness", useRoughness ? 1.0f : 0.0f);
			if (useRoughness) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kRoughness);
				glBindTexture(GL_TEXTURE_2D, state.roughnessTexture);
				geometry_shader->SetUniform("material.roughness_map", GeometryPass::kRoughness);
			}

			const bool useMetallic = state.metallicTexture != 0;
			geometry_shader->SetUniform("material.use_metallic", useMetallic ? 1.0f : 0.0f);
			if (useMetallic) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kMetallic);
				glBindTexture(GL_TEXTURE_2D, state.metallicTexture);
				geometry_shader->SetUniform("material.metallic_map", GeometryPass::kMetallic);
			}

			const bool useEmission = state.emissionTexture != 0;
			geometry_shader->SetUniform("material.use_emission", useEmission ? 1.0f : 0.0f);
			if (useEmission) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kEmission);
				glBindTexture(GL_TEXTURE_2D, state.emissionTexture);
				geometry_shader->SetUniform("material.emission_map", GeometryPass::kEmission);
			}
		}

		// ========================================
		// OPTIMIZED: ApplyCachedGeometryMaterialState
		// Uses pre-cached SubmeshTextureCache to avoid per-frame asset lookups
		// ========================================
		// NOTE: We cannot use dirty tracking here because the same shader program
		// is shared across multiple submeshes with different materials. When
		// rendering submesh N+1, the shader still has submesh N's uniform values.
		// If we skip uploading, submesh N+1 would incorrectly use submesh N's material.
		void ApplyCachedGeometryMaterialState(const std::shared_ptr<Assets::Shader>& shader,
			const ModelRenderer::SubmeshTextureCache& cache) {
			
			// Always bind textures (must happen every frame for correct rendering)
			const bool hasTexture = cache.albedoTexture != 0;
			if (hasTexture) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kAlbedo);
				glBindTexture(GL_TEXTURE_2D, cache.albedoTexture);
				shader->SetUniform("material.tex", GeometryPass::kAlbedo);  // Always set texture unit
			}

			const bool useAo = cache.aoTexture != 0;
			if (useAo) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kAo);
				glBindTexture(GL_TEXTURE_2D, cache.aoTexture);
				shader->SetUniform("material.ao_map", GeometryPass::kAo);
			}

			const bool useNormal = cache.normalTexture != 0;
			if (useNormal) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kNormal);
				glBindTexture(GL_TEXTURE_2D, cache.normalTexture);
				shader->SetUniform("material.normal_map", GeometryPass::kNormal);
			}

			const bool useRoughness = cache.roughnessTexture != 0;
			if (useRoughness) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kRoughness);
				glBindTexture(GL_TEXTURE_2D, cache.roughnessTexture);
				shader->SetUniform("material.roughness_map", GeometryPass::kRoughness);
			}

			const bool useMetallic = cache.metallicTexture != 0;
			if (useMetallic) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kMetallic);
				glBindTexture(GL_TEXTURE_2D, cache.metallicTexture);
				shader->SetUniform("material.metallic_map", GeometryPass::kMetallic);
			}

			const bool useEmission = cache.emissiveTexture != 0;
			if (useEmission) {
				glActiveTexture(GL_TEXTURE0 + GeometryPass::kEmission);
				glBindTexture(GL_TEXTURE_2D, cache.emissiveTexture);
				shader->SetUniform("material.emission_map", GeometryPass::kEmission);
			}

			// Always set texture enable flags (these are cheap uniforms)
			shader->SetUniform("material.useTex", hasTexture ? 1.0f : 0.0f);
			shader->SetUniform("material.use_ao", useAo ? 1.0f : 0.0f);
			shader->SetUniform("material.use_normal", useNormal ? 1.0f : 0.0f);
			shader->SetUniform("material.use_roughness", useRoughness ? 1.0f : 0.0f);
			shader->SetUniform("material.use_metallic", useMetallic ? 1.0f : 0.0f);
			shader->SetUniform("material.use_emission", useEmission ? 1.0f : 0.0f);

			// Always upload material properties (required for correct per-submesh rendering)
			shader->SetUniform("u_DecodeAlbedoInShader",
				GraphicsSettings::get().decode_albedo_in_shader ? 1.0f : 0.0f);
			shader->SetUniform("material.rough", cache.roughness);
			shader->SetUniform("material.metal", cache.metallic);
			shader->SetUniform("material.color", cache.baseColor);
			shader->SetUniform("u_UseEmissionOverride", cache.useEmissionOverride ? 1.0f : 0.0f);
			shader->SetUniform("u_EmissionOverride",
				cache.useEmissionOverride ? cache.emissionOverride : glm::vec3(0.0f));
			shader->SetUniform("material.ao_channel_mask", cache.aoChannelMask);
			shader->SetUniform("material.roughness_channel_mask", cache.roughnessChannelMask);
			shader->SetUniform("material.metallic_channel_mask", cache.metallicChannelMask);
		}
	}
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
		GLint previousUnpackAlignment = 4;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTextureUnit);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTex2D);
		glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousTexCube);
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousUnpackAlignment);

#ifdef PN_PLATFORM_ANDROID
		if (eglGetCurrentContext() == EGL_NO_CONTEXT ||
			eglGetCurrentSurface(EGL_DRAW) == EGL_NO_SURFACE) {
			PN_CORE_WARN("Deferring texture upload because EGL context/surface is not current yet");
			return;
		}
#endif

		while (glGetError() != GL_NO_ERROR) {}

		PN_CORE_TRACE("Texture load started, active unit: GL_TEXTURE{}", activeTextureUnit - GL_TEXTURE0);

		// ========================================
		// RESET TO TEXTURE UNIT 0 FOR LOADING
		// ========================================
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, 0);
		if (!tex->is_compressed) {
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		}

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

			TryApplyMaxAnisotropy(GL_TEXTURE_CUBE_MAP);

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

					const uint8_t* uploadData = tex->data.data() + offset;

					if (tex->is_compressed) {
						glCompressedTexImage2D(
							GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
							mip,
							tex->glTexFormat,
							mipW,
							mipH,
							0,
							static_cast<GLsizei>(mipSize),
							uploadData
						);
					}
					else {
						glTexImage2D(
							GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
							mip,
							tex->glTexFormat,
							mipW,
							mipH,
							0,
							tex->glBaseFormat,
							tex->glDataType,
							uploadData
						);
					}

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

			TryApplyMaxAnisotropy(GL_TEXTURE_2D);

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

				if (tex->is_compressed) {
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
				}
				else {
					glTexImage2D(
						GL_TEXTURE_2D,
						mip,
						tex->glTexFormat,
						mipW,
						mipH,
						0,
						tex->glBaseFormat,
						tex->glDataType,
						tex->data.data() + offset
					);
				}

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
		glPixelStorei(GL_UNPACK_ALIGNMENT, previousUnpackAlignment);
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
        auto assetManager = services->get<Assets::Manager>();
        auto sceneManager = services->get<Scene::SceneManager>();

        auto appendModelAsset = [&](const std::shared_ptr<Assets::Model>& modelAsset) {
            if (!modelAsset || modelAsset->type != Assets::Type::Model) {
                return;
            }

            if (instanced_offsets.find(modelAsset->vpath) != instanced_offsets.end()) {
                return;
            }

            instanced_offsets[modelAsset->vpath] = {
                indexOffset, static_cast<unsigned int>(modelAsset->indices.size()) };

            for (const auto& v : modelAsset->vertices) {
                newVertices.push_back(v);
            }

            for (unsigned int idx : modelAsset->indices) {
                newIndices.push_back(vertexOffset + idx);
            }

            vertexOffset += static_cast<unsigned int>(modelAsset->vertices.size());
            indexOffset += static_cast<unsigned int>(modelAsset->indices.size());
        };

		for (auto registryId : ecs->getAllRegistryIDs()) {
			auto& registry = ecs->getRegistry(registryId);
			auto view = registry.view<ModelRenderer>();

			for (auto e : view) {
				auto mdl = ecs->getEntityComponent<ModelRenderer>(e, registryId);
				if (!mdl.has_value())
					continue;

				auto mdl_opt = assetManager->getAsset<Assets::Model>(mdl.value().get().modelGUID);
				if (!mdl_opt.has_value() || mdl_opt.value()->type != Assets::Type::Model)
					continue;
                appendModelAsset(mdl_opt.value());
			}
		}

        if (sceneManager) {
            for (const auto& modelGuid : sceneManager->getPreloadedModelGUIDs()) {
                auto mdlOpt = assetManager->getAsset<Assets::Model>(modelGuid);
                if (mdlOpt.has_value()) {
                    appendModelAsset(mdlOpt.value());
                }
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
		// OPTIMIZATION: Use CPU-side cache to avoid GPU read-back stalls
		// Instead of reading from GPU (which causes sync points), we:
		// 1. Keep all data in CPU cache
		// 2. Orphan the buffer when resizing
		// 3. Re-upload from CPU cache
		auto appendToBuffer = [](GLuint buf, GLenum target,
			unsigned int existingBytes,
			const void* newData, unsigned int newBytes,
			const void* cpuCacheData, unsigned int cpuCacheSize) {
				glBindBuffer(target, buf);

				GLint currentSize = 0;
				glGetBufferParameteriv(target, GL_BUFFER_SIZE, &currentSize);

				unsigned int totalBytes = existingBytes + newBytes;

				if ((unsigned int)currentSize < totalBytes) {
					// OPTIMIZATION: Orphan the buffer and re-upload from CPU cache
					// This avoids the GPU-CPU sync stall that would occur with glMapBufferRange(GL_MAP_READ_BIT)
					// The CPU cache contains all previously uploaded data, so we can safely re-upload it
					glBufferData(target, totalBytes, nullptr, GL_DYNAMIC_DRAW); // orphan
					
					// Re-upload existing data from CPU cache
					if (cpuCacheSize > 0 && cpuCacheData) {
						glBufferSubData(target, 0, cpuCacheSize, cpuCacheData);
					}
				}

				// Append new data at the end
				glBufferSubData(target, existingBytes, newBytes, newData);
			};

		unsigned int existingVertexBytes = currentVertexCount * sizeof(Assets::Vertex);
		unsigned int newVertexBytes = (unsigned int)(newVertices.size() * sizeof(Assets::Vertex));
		unsigned int existingIndexBytes = currentIndexCount * sizeof(unsigned int);
		unsigned int newIndexBytes = (unsigned int)(newIndices.size() * sizeof(unsigned int));

		// OPTIMIZATION: Update CPU-side caches before GPU upload
		// This allows us to re-upload without GPU read-back if buffer needs to grow
		cpu_vertex_cache.insert(cpu_vertex_cache.end(), newVertices.begin(), newVertices.end());
		cpu_index_cache.insert(cpu_index_cache.end(), newIndices.begin(), newIndices.end());
		cpu_cache_valid = true;

		// Append to geometry buffers using CPU cache for re-upload (avoids GPU stalls)
		glBindVertexArray(geometry_vao);
		appendToBuffer(geometry_vbo, GL_ARRAY_BUFFER,
			existingVertexBytes, newVertices.data(), newVertexBytes,
			cpu_vertex_cache.data(), existingVertexBytes);
		appendToBuffer(geometry_ebo, GL_ELEMENT_ARRAY_BUFFER,
			existingIndexBytes, newIndices.data(), newIndexBytes,
			cpu_index_cache.data(), existingIndexBytes);

		// Append to shadow buffers using same CPU cache
		glBindVertexArray(shadow_vao);
		appendToBuffer(shadow_vbo, GL_ARRAY_BUFFER,
			existingVertexBytes, newVertices.data(), newVertexBytes,
			cpu_vertex_cache.data(), existingVertexBytes);
		appendToBuffer(shadow_ebo, GL_ELEMENT_ARRAY_BUFFER,
			existingIndexBytes, newIndices.data(), newIndexBytes,
			cpu_index_cache.data(), existingIndexBytes);

		// Pre-size the instance matrix buffer - filled per-frame in DrawGeometryInstanced.
		// Reserve space for MAX_INSTANCES matrices upfront to avoid per-frame realloc.
		static constexpr int MAX_INSTANCES = 4096;
		if (geometry_ibo) {
			glBindBuffer(GL_ARRAY_BUFFER, geometry_ibo);
			GLint currentIboSize = 0;
			glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &currentIboSize);
			const GLsizeiptr needed = MAX_INSTANCES * sizeof(glm::mat4);
			if (currentIboSize < (GLint)needed) {
				glBufferData(GL_ARRAY_BUFFER, needed, nullptr, GL_DYNAMIC_DRAW);
			}
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

		// OPTIMIZATION: Clear CPU-side caches
		cpu_vertex_cache.clear();
		cpu_index_cache.clear();
		cpu_cache_valid = false;

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
		std::filesystem::path fxaa_path = "engine/shaders/fxaa.vert";
		std::filesystem::path ssao_path = "engine/shaders/ssao.vert";
		std::filesystem::path ssao_blur_path = "engine/shaders/ssao_blur.vert";
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
		std::filesystem::path fxaa_path = "engine\\shaders\\android_fxaa.vert";
		std::filesystem::path ssao_path = "engine\\shaders\\android_ssao.vert";
		std::filesystem::path ssao_blur_path = "engine\\shaders\\android_ssao_blur.vert";
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

		if (pbr_light_ubo == 0) {
			glGenBuffers(1, &pbr_light_ubo);
			glBindBuffer(GL_UNIFORM_BUFFER, pbr_light_ubo);
			glBufferData(
				GL_UNIFORM_BUFFER,
				static_cast<GLsizeiptr>(sizeof(PbrLightGpuData) * kMaxPbrLights),
				nullptr,
				GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		const GLuint pbrProgram = pbr_shader->GetRendererID();
		const GLuint lightBlockIndex = glGetUniformBlockIndex(pbrProgram, "PbrLightBlock");
		if (lightBlockIndex == GL_INVALID_INDEX) {
			PN_CORE_ERROR("PBR shader missing required PbrLightBlock uniform block");
			throw std::runtime_error("");
			return;
		} else {
			glUniformBlockBinding(pbrProgram, lightBlockIndex, kPbrLightUboBindingPoint);
			pbr_light_ubo_bound_program = pbrProgram;
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

		// OPTIMIZATION: Create bone matrix UBO for animated meshes
		// This replaces 100+ individual SetUniform calls with a single buffer update
		if (bone_matrix_ubo == 0) {
			glGenBuffers(1, &bone_matrix_ubo);
			glBindBuffer(GL_UNIFORM_BUFFER, bone_matrix_ubo);
			// MAX_BONES * sizeof(mat4) = 100 * 64 = 6400 bytes
			glBufferData(
				GL_UNIFORM_BUFFER,
				static_cast<GLsizeiptr>(MAX_BONES * sizeof(glm::mat4)),
				nullptr,
				GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}

		// Bind bone UBO to the geometry shader
		const GLuint geometryProgram = geometry_shader->GetRendererID();
		const GLuint boneBlockIndex = glGetUniformBlockIndex(geometryProgram, "BoneBlock");
		if (boneBlockIndex != GL_INVALID_INDEX) {
			glUniformBlockBinding(geometryProgram, boneBlockIndex, 1); // binding = 1 in shader
			bone_matrix_ubo_bound_program = geometryProgram;
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

		// Combined tone + gamma shader (optimization: merges two passes into one)
#ifdef PN_PLATFORM_ANDROID
		std::filesystem::path tone_gamma_path = "engine/shaders/android_tone_gamma.vert";
#else
		std::filesystem::path tone_gamma_path = "engine/shaders/tone_gamma.vert";
#endif
		shader_opt = assets_loader->getAsset<Assets::Shader>(tone_gamma_path);
		tone_gamma_shader = shader_opt.has_value() ? shader_opt.value() : tone_gamma_shader;

		if (!tone_gamma_shader || tone_gamma_shader->GetRendererID() == 0) {
			PN_CORE_WARN("Failed to create tone_gamma shader, falling back to separate tone+gamma passes");
			tone_gamma_shader = nullptr;
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
			PN_CORE_ERROR("Failed to create shader program for volumetric lighting - volumetric effects will be disabled");
			volumetric_shader = nullptr;
		}

		// FXAA shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(fxaa_path);
		fxaa_shader = shader_opt.has_value() ? shader_opt.value() : fxaa_shader;

		if (!fxaa_shader || fxaa_shader->GetRendererID() == 0) {
			PN_CORE_WARN("Failed to create FXAA shader - anti-aliasing will be disabled");
			fxaa_shader = nullptr;
		}

		// SSAO shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(ssao_path);
		ssao_shader = shader_opt.has_value() ? shader_opt.value() : ssao_shader;
		if (!ssao_shader || ssao_shader->GetRendererID() == 0) {
			PN_CORE_WARN("Failed to create SSAO shader - SSAO will be disabled");
			ssao_shader = nullptr;
		}

		// SSAO blur shader
		shader_opt = assets_loader->getAsset<Assets::Shader>(ssao_blur_path);
		ssao_blur_shader = shader_opt.has_value() ? shader_opt.value() : ssao_blur_shader;
		if (!ssao_blur_shader || ssao_blur_shader->GetRendererID() == 0) {
			PN_CORE_WARN("Failed to create SSAO blur shader - SSAO will be disabled");
			ssao_blur_shader = nullptr;
		}

	}

	void WindowsRenderer::_createDeferredShadingBuffer(unsigned int& tex,
													   int num_channels,
													   int gl_color_attachment,
													   GLenum internal_format_override,
													   GLenum format_override,
													   GLenum type_override) {
		glGenTextures(1, &tex);
		if (!tex) {
			PN_CORE_ERROR("Failed to gen texture");
			throw std::runtime_error("");
			return;
		}

		glBindTexture(GL_TEXTURE_2D, tex);

		if (internal_format_override != 0 && format_override != 0 && type_override != 0) {
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				static_cast<GLint>(internal_format_override),
				winWidth,
				winHeight,
				0,
				format_override,
				type_override,
				nullptr);
		} else {
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
		}
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
								  pp2_texture, volumetric_textures[0], volumetric_textures[1],
								  fallback_material_texture, fallback_emission_texture };
			glDeleteTextures(static_cast<GLsizei>(sizeof(textures) / sizeof(textures[0])), textures);
			pos_texture = col_texture = norm_texture = material_properties_texture
				= emission_texture = ds_depth_texture = final_texture = pp_texture
				= pp2_texture = 0;
			fallback_material_texture = fallback_emission_texture = 0;
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

		auto createSolidTexture2D = [](unsigned int& tex, unsigned char r, unsigned char g,
										unsigned char b, unsigned char a) {
			if (tex != 0) {
				glDeleteTextures(1, &tex);
				tex = 0;
			}

			const unsigned char pixel[4] = {r, g, b, a};
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE,
						 pixel);
		};

		// Fallbacks for reduced G-buffer devices.
		// material.rgb = roughness(1), metallic(0), ao(1); emission = black.
		createSolidTexture2D(fallback_material_texture, 255, 0, 255, 255);
		createSolidTexture2D(fallback_emission_texture, 0, 0, 0, 255);

		// === Final FBO/Texture For Deffered Shading ===
		{
			glGenFramebuffers(1, &ds_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, ds_fbo);

#ifdef PN_PLATFORM_ANDROID
			_createDeferredShadingBuffer(pos_texture, 3, GL_COLOR_ATTACHMENT0);
			_createDeferredShadingBuffer(col_texture, 3, GL_COLOR_ATTACHMENT1);  // GL_RGBA16F — matches Windows precision for linear-space albedo after pow(2.2)
			_createDeferredShadingBuffer(norm_texture, 3, GL_COLOR_ATTACHMENT2);
			if (active_gbuffer_count >= 4) {
				_createDeferredShadingBuffer(material_properties_texture, 3, GL_COLOR_ATTACHMENT3);
			}
			if (active_gbuffer_count >= 5) {
				_createDeferredShadingBuffer(emission_texture, 3, GL_COLOR_ATTACHMENT4, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
			}
#else
			_createDeferredShadingBuffer(pos_texture, 3, GL_COLOR_ATTACHMENT0);
			_createDeferredShadingBuffer(col_texture, 3, GL_COLOR_ATTACHMENT1);
			_createDeferredShadingBuffer(norm_texture, 3, GL_COLOR_ATTACHMENT2);
			if (active_gbuffer_count >= 4) {
				_createDeferredShadingBuffer(material_properties_texture, 3, GL_COLOR_ATTACHMENT3);
			}
			if (active_gbuffer_count >= 5) {
				_createDeferredShadingBuffer(emission_texture, 3, GL_COLOR_ATTACHMENT4);
			}
#endif

			unsigned int attachments[5] = {
				GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2,
				GL_COLOR_ATTACHMENT3,
				GL_COLOR_ATTACHMENT4,
			};
			glDrawBuffers(active_gbuffer_count, attachments);
			// Clear all active attachments to ensure clean state
			for (int i = 0; i < active_gbuffer_count; ++i) {
				glDrawBuffers(1, &attachments[i]);
				glClearColor(0.0f, 0.0f, 0.0f, (i == 3) ? 0.0f : 1.0f); // material_properties alpha = 0
				glClear(GL_COLOR_BUFFER_BIT);
			}
			glDrawBuffers(active_gbuffer_count, attachments);

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
			// OPTIMIZATION: Allow post-process resolution scaling on all platforms
			// This significantly reduces GPU load for bloom/blur/tone mapping passes
			const float pp_scale = glm::clamp(GraphicsSettings::get().postprocess_resolution_scale, 0.25f, 1.0f);
			pp_width = std::max(1, static_cast<int>(std::round(winWidth * pp_scale)));
			pp_height = std::max(1, static_cast<int>(std::round(winHeight * pp_scale)));
			PN_CORE_INFO("Post-process buffers at {}x{} ({} scale)", pp_width, pp_height, pp_scale);
			glGenFramebuffers(1, &pp_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, pp_fbo);

			glGenTextures(1, &pp_texture);
			if (pp_texture == 0) {
				PN_CORE_ERROR("Failed to create final texture");
				failInit("Post-process texture allocation");
				return;
			}
			glBindTexture(GL_TEXTURE_2D, pp_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, pp_width, pp_height, 0, GL_RGBA,
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

			// Clear pp_texture to black to avoid garbage data on first frame
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

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
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, pp_width, pp_height, 0, GL_RGBA,
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

			// Clear pp2_texture to black to avoid garbage data on first frame
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// === SSAO resources ===
			// Generate hemisphere kernel (16 samples)
			{
				std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
				std::default_random_engine generator(42u); // fixed seed for determinism
				ssao_kernel.clear();
				ssao_kernel.reserve(16);
				for (int i = 0; i < 16; ++i) {
					glm::vec3 sample(
						randomFloats(generator) * 2.0f - 1.0f,
						randomFloats(generator) * 2.0f - 1.0f,
						randomFloats(generator)
					);
					sample = glm::normalize(sample) * randomFloats(generator);
					float scale = float(i) / 16.0f;
					scale = glm::mix(0.1f, 1.0f, scale * scale);
					ssao_kernel.push_back(sample * scale);
				}

				// 4×4 noise texture (random XY rotation vectors)
				std::vector<glm::vec2> ssao_noise;
				ssao_noise.reserve(16);
				for (int i = 0; i < 16; ++i) {
					ssao_noise.push_back(glm::vec2(
						randomFloats(generator) * 2.0f - 1.0f,
						randomFloats(generator) * 2.0f - 1.0f
					));
				}
				glGenTextures(1, &ssao_noise_texture);
				glBindTexture(GL_TEXTURE_2D, ssao_noise_texture);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 4, 4, 0, GL_RG, GL_FLOAT, ssao_noise.data());
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			// SSAO raw FBO (R8, full resolution)
			glGenFramebuffers(1, &ssao_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
			glGenTextures(1, &ssao_texture);
			glBindTexture(GL_TEXTURE_2D, ssao_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, winWidth, winHeight, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_texture, 0);
			if (!checkFramebufferComplete("SSAO framebuffer")) {
				PN_CORE_WARN("SSAO framebuffer incomplete - SSAO will be disabled");
			}
			// Clear to white (no occlusion) for safe first frame
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// SSAO blur FBO (R8, full resolution)
			glGenFramebuffers(1, &ssao_blur_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_fbo);
			glGenTextures(1, &ssao_blur_texture);
			glBindTexture(GL_TEXTURE_2D, ssao_blur_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, winWidth, winHeight, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ssao_blur_texture, 0);
			if (!checkFramebufferComplete("SSAO blur framebuffer")) {
				PN_CORE_WARN("SSAO blur framebuffer incomplete - SSAO will be disabled");
			}
			// Clear to white (no occlusion) for safe first frame
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// minimap framebuffer
			const glm::vec2 minimap_size = GraphicsSettings::get().minimap_size_px;
			const PAIN::Render::MinimapBackgroundTargetSpec minimapTargetSpec =
				PAIN::Render::BuildMinimapBackgroundTargetSpec();
			const GLint minimapInternalFormat =
				minimapTargetSpec.colorFormat == PAIN::Render::MinimapColorFormat::RGBA16F
				? GL_RGBA16F
				: GL_RGBA8;
			const GLenum minimapPixelType =
				minimapTargetSpec.colorFormat == PAIN::Render::MinimapColorFormat::RGBA16F
				? GL_FLOAT
				: GL_UNSIGNED_BYTE;
			minimap_width = std::max(64, static_cast<int>(minimap_size.x));
			// Runtime validation: for circular minimap, clamp height to width for perfect circle
			if (GraphicsSettings::get().minimap_shape == GraphicsSettings::MINIMAP_SHAPE_CIRCLE) {
				minimap_height = std::max(64, static_cast<int>(glm::min(minimap_size.x, minimap_size.y)));
			} else {
				minimap_height = std::max(64, static_cast<int>(minimap_size.y));
			}

			glGenFramebuffers(1, &minimap_fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, minimap_fbo);

			glGenTextures(1, &minimap_texture);
			glBindTexture(GL_TEXTURE_2D, minimap_texture);
			glTexImage2D(GL_TEXTURE_2D, 0, minimapInternalFormat, minimap_width, minimap_height, 0,
				GL_RGBA, minimapPixelType, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				minimap_texture, 0);

			if (minimapTargetSpec.needsDepthStencil) {
				glGenRenderbuffers(1, &minimap_rbo);
				glBindRenderbuffer(GL_RENDERBUFFER, minimap_rbo);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, minimap_width,
					minimap_height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
					GL_RENDERBUFFER, minimap_rbo);
			}
			else {
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
					GL_RENDERBUFFER, 0);
			}

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

			// Instance model matrix buffer (locations 7-10, one mat4 per instance, divisor=1)
			glGenBuffers(1, &geometry_ibo);
			glBindBuffer(GL_ARRAY_BUFFER, geometry_ibo);
			for (int col = 0; col < 4; ++col) {
				glEnableVertexAttribArray(7 + col);
				glVertexAttribPointer(7 + col, 4, GL_FLOAT, GL_FALSE,
					sizeof(glm::mat4), (void*)(col * sizeof(glm::vec4)));
				glVertexAttribDivisor(7 + col, 1);
			}

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
		// PERFORMANCE: Pre-allocate debug VBO with sufficient capacity
		// This avoids per-draw reallocation checks which can cause micro-stutter
		// 256KB buffer = ~36K vertices (7 floats/vertex * 4 bytes/float)
		// Increased from 64KB to handle complex debug scenes without reallocation
		{
			glGenVertexArrays(1, &debug_VAO);
			glBindVertexArray(debug_VAO);

			glGenBuffers(1, &debug_VBO);
			glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
			
			// Pre-allocate buffer for debug drawing (reduces per-frame allocation overhead)
			static constexpr GLsizeiptr kDebugVboInitialCapacity = 256 * 1024;
			glBufferData(GL_ARRAY_BUFFER, kDebugVboInitialCapacity, nullptr, GL_DYNAMIC_DRAW);
			debug_vbo_capacity = kDebugVboInitialCapacity;

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
		glGetIntegerv(GL_MAX_DRAW_BUFFERS, &max_draw_buffers);
		glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &max_color_attachments);
		const int maxGBufferAttachments = static_cast<int>(std::min(max_draw_buffers, max_color_attachments));
		active_gbuffer_count = std::clamp(maxGBufferAttachments, 3, 5);
		if (active_gbuffer_count < 5) {
			PN_CORE_WARN("Reducing active G-buffer draw attachments to {} (caps: drawBuffers={}, colorAttachments={})",
				active_gbuffer_count, max_draw_buffers, max_color_attachments);
			if (active_gbuffer_count < 4) {
				PN_CORE_WARN("G-buffer material attachment unavailable; lighting will use fallback material parameters.");
			}
		}

		// Validate texture unit availability
		{
			GLint maxTextureUnits = 0;
			glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
			GLint maxCombinedTextureUnits = 0;
			glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxCombinedTextureUnits);
			
			const int unitsNeeded = TextureUnits::GetMaxUnitsNeeded();
			
			if (!TextureUnits::HasSufficientTextureUnits(maxTextureUnits)) {
				PN_CORE_ERROR("Insufficient texture units! Device has {} fragment texture units, need {}",
					maxTextureUnits, unitsNeeded);
				PN_CORE_ERROR("Rendering may fail or produce incorrect results.");
			} else {
				PN_CORE_INFO("Texture units: {} available (fragment), {} combined, {} needed",
					maxTextureUnits, maxCombinedTextureUnits, unitsNeeded);
			}
		}

		initShaders();

		glGenVertexArrays(1, &empty_vao);
		if (empty_vao == 0) {
			PN_CORE_ERROR("Failed to create empty VAO");
			return;
		}

		_initDeferredShadingBuffers(); // FBOs/textures only
		_initGeometryBuffers();        // VAOs/VBOs called ONCE, never on resize

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}

	void WindowsRenderer::Render2DTexture(GLuint texture_id, const glm::vec2& pos,
										  glm::vec2& scale,
										  const glm::vec4& uv_transform, float opacity) {
		if (texture_id == 0) {
			PN_CORE_ERROR("Invalid texture_id in Render2DTexture");
			return;
		}

		if (!texture2d_shader) {
			PN_CORE_ERROR("Unable to find texture2d_shader");
			return;
		}

		#ifdef _DEBUG
		GLint currentActiveTexture;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &currentActiveTexture);

		GLint currentTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);

		GLint currentVAO;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);

		GLint currentProgram;
		glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
		#endif

		auto window_service = services->get<Window::Window>();
		auto framebuffer = window_service->getFrameBuffer();

		float aspect_ratio =
			static_cast<float>(framebuffer.x) / static_cast<float>(framebuffer.y);
		glm::vec2 corrected_scale = glm::vec2(scale.x / aspect_ratio, scale.y);

		texture2d_shader->Bind();

		texture2d_shader->SetUniform("pos", pos);
		texture2d_shader->SetUniform("ndc_scale", corrected_scale);
		texture2d_shader->SetUniform("uv_transform",
									 uv_transform); // Pass UV transform
		texture2d_shader->SetUniform("u_Opacity", opacity);

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		if (clamp_configured_textures.find(texture_id) == clamp_configured_textures.end()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			clamp_configured_textures.insert(texture_id);
		}

		texture2d_shader->SetUniform("tex", 6);

		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		#ifdef _DEBUG
		glActiveTexture(currentActiveTexture);
		glBindTexture(GL_TEXTURE_2D, currentTexture);
		glBindVertexArray(currentVAO);
		glUseProgram(currentProgram);
		#endif
	}

	void WindowsRenderer::Render2DTextureCircular(GLuint texture_id, const glm::vec2& pos,
										  glm::vec2& scale,
										  const glm::vec4& uv_transform, float opacity) {
		if (texture_id == 0) {
			PN_CORE_ERROR("Invalid texture_id in Render2DTextureCircular");
			return;
		}

		if (!texture2d_shader) {
			PN_CORE_ERROR("Unable to find texture2d_shader for circular minimap");
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

		auto window_service = services->get<Window::Window>();
		auto framebuffer = window_service->getFrameBuffer();
		float aspect_ratio = static_cast<float>(framebuffer.x) / static_cast<float>(framebuffer.y);
		glm::vec2 corrected_scale = glm::vec2(scale.x / aspect_ratio, scale.y);

		// ========================================
		// RENDER TEXTURE WITH CIRCULAR CLIPPING (shader-based)
		// ========================================
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		texture2d_shader->Bind();
		texture2d_shader->SetUniform("pos", pos);
		texture2d_shader->SetUniform("ndc_scale", corrected_scale);
		texture2d_shader->SetUniform("uv_transform", uv_transform);
		texture2d_shader->SetUniform("u_Opacity", opacity);
		
		// Enable circular clipping in shader (shader clips based on UV coords, center=0.5, radius=0.5)
		texture2d_shader->SetUniform("u_ClipCircle", 1);

		glActiveTexture(GL_TEXTURE6);
		glBindTexture(GL_TEXTURE_2D, texture_id);
		if (clamp_configured_textures.find(texture_id) == clamp_configured_textures.end()) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			clamp_configured_textures.insert(texture_id);
		}
		texture2d_shader->SetUniform("tex", 6);

		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// Reset clip uniform for subsequent draws
		texture2d_shader->SetUniform("u_ClipCircle", 0);

		// ========================================
		// RESTORE STATE
		// ========================================
		glDepthMask(GL_TRUE);
		glActiveTexture(currentActiveTexture);
		glBindTexture(GL_TEXTURE_2D, currentTexture);
		glBindVertexArray(currentVAO);
		glUseProgram(currentProgram);
	}

	// Shadow pass entry point: sysRender selects which light to render,
	// while the renderer owns framebuffer binding and per-pass GPU state.
	// clearDepth: false when rendering dynamic shadows over cached static shadows
	void WindowsRenderer::BeginShadowPass(const Light& l, bool clearDepth) {
		if (l.getShadowFbo() == 0 || l.getShadowTexture() == 0) {
			PN_CORE_WARN("[GL] Skipping shadow pass because the mapped light has no valid shadow targets.");
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, l.getShadowFbo());
		glViewport(0, 0, l.getShadowResolution(), l.getShadowResolution());
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		// OPTIMIZATION: Use front-face culling for shadows by default.
		// This reduces overdraw and shadow acne. Objects with doubleSidedShadows
		// will temporarily disable culling to handle thin geometry.
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
#ifdef PN_PLATFORM_ANDROID
		// critical for Mali GPU on android depth-only shadow rendering
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
#endif
		// Only clear depth buffer if this is a full shadow pass (static + dynamic)
		// For dynamic-only passes over cached static shadows, skip clearing
		if (clearDepth) {
#ifdef PN_PLATFORM_ANDROID
			glClearDepthf(1.0f);
#else
			glClearDepth(1.0f);
#endif
			glClear(GL_DEPTH_BUFFER_BIT);
		}
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
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
#ifdef PN_PLATFORM_ANDROID
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
#endif
	}

	// Geometry pass entry point: sysRender has already selected the scene and
	// camera; the renderer owns G-buffer binding, viewport, and draw state.
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

		geometry_shader->Bind();
		geometry_shader->SetUniform("u_V", scene->GetActiveCamera()->view());
		geometry_shader->SetUniform("u_P", scene->GetActiveCamera()->projection());
		geometry_shader->SetUniform("u_Instanced", 0.f);
		geometry_shader->SetUniform("u_Animated", 0.f);
		geometry_shader->SetUniform("u_InvertUvY", 0.f);
		geometry_shader->SetUniform("u_UseEmissionOverride", 0.f);
		geometry_shader->SetUniform("u_EmissionOverride", glm::vec3(0.0f));
	}

	void WindowsRenderer::DrawGeometry(std::shared_ptr<Scene::SceneManager> scene,
									   ModelRenderer& component,
									   const glm::mat4& M) {

		if (!geometry_shader || !component.cachedModelAsset) {
			return;
		}

#ifdef _DEBUG
		// DEBUG: Verify shader program is valid
		const GLuint shaderProgram = geometry_shader->GetRendererID();
		if (shaderProgram == 0 || !glIsProgram(shaderProgram)) {
			PN_CORE_ERROR("[GL ERROR] DrawGeometry: Invalid shader program {}", shaderProgram);
			return;
		}

		// DEBUG: Helper lambda for GL error checking within geometry pass
		auto checkGLError = [](const char* context) -> bool {
			const GLenum err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("[GL ERROR] {} - Error 0x{:x} ({})", context, err, DescribeGlError(err));
				return true;
			}
			return false;
		};
#else
		auto checkGLError = [](const char*) -> bool {
			return false;
		};
#endif

		auto assetManager = services->get<Assets::Manager>();

		const auto& modelAsset = component.cachedModelAsset;

		geometry_shader->SetUniform("u_M", M);
		checkGLError("DrawGeometry: After SetUniform u_M");
		geometry_shader->SetUniform("u_Instanced", 0.f);
		geometry_shader->SetUniform("u_InvertUvY", 0.f);
		
		// Pre-compute normal matrix on CPU instead of in shader for non-instanced meshes
		// This saves ~10-15 ALU instructions per vertex
		glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(M));
		geometry_shader->SetUniform("u_NormalMatrix", normalMatrix);
		checkGLError("DrawGeometry: After SetUniform u_NormalMatrix");

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
		checkGLError("DrawGeometry: After glBindVertexArray");

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

			// ========================================
			// PERFORMANCE OPTIMIZATION: Use Cached Material State
			// submeshCaches are populated once in InitializeModelRenderer
			// Avoids per-frame BuildGeometryMaterialState asset lookups
			// ========================================
			const bool hasRuntimeOverrides = component.materials[submesh.materialIndex].useOverrides;
			if (!hasRuntimeOverrides &&
				i < component.submeshCaches.size() &&
				component.submeshCaches[i].cacheValid) {
				ApplyCachedGeometryMaterialState(geometry_shader, component.submeshCaches[i]);
				checkGLError("DrawGeometry: After ApplyCachedGeometryMaterialState");
			} else {
				// Fallback: build material state per-frame (shouldn't happen normally)
				const GeometryMaterialState materialState =
					BuildGeometryMaterialState(assetManager, component, i);
				ApplyGeometryMaterialState(geometry_shader, materialState);
				checkGLError("DrawGeometry: After ApplyGeometryMaterialState");
			}

			// animation

			int bones_skipped{};
			if (!component.boneTransforms.empty()) {

				// Animation calculation now handled in animation system
				const auto& matrices = component.boneTransforms;

				if (!matrices.empty()) {
					// OPTIMIZATION: Use UBO for bone matrices instead of 100+ SetUniform calls
					// Single buffer update is much faster than individual uniform calls
					
					// Rebind UBO if geometry shader program changed (hot-reload support)
					const GLuint geometryProgram = geometry_shader->GetRendererID();
					if (geometryProgram != bone_matrix_ubo_bound_program) {
						const GLuint boneBlockIndex = glGetUniformBlockIndex(geometryProgram, "BoneBlock");
						if (boneBlockIndex != GL_INVALID_INDEX) {
							glUniformBlockBinding(geometryProgram, boneBlockIndex, 1); // binding = 1
							bone_matrix_ubo_bound_program = geometryProgram;
						}
					}
					
					const size_t boneCount = std::min(matrices.size(), static_cast<size_t>(MAX_BONES));
					const GLsizeiptr dataSize = static_cast<GLsizeiptr>(boneCount * sizeof(glm::mat4));
					
					glBindBuffer(GL_UNIFORM_BUFFER, bone_matrix_ubo);
					glBufferSubData(GL_UNIFORM_BUFFER, 0, dataSize, matrices.data());
					glBindBufferBase(GL_UNIFORM_BUFFER, 1, bone_matrix_ubo); // binding = 1
					glBindBuffer(GL_UNIFORM_BUFFER, 0);
					
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
			checkGLError("DrawGeometry: After glDrawElements");
		}

		// LogMemoryFullDiagnostic("After Rendering Sub Meshes.");

		glBindVertexArray(0);
	}

	void WindowsRenderer::DrawGeometryInstanced(
		std::shared_ptr<Scene::SceneManager> scene,
		ModelRenderer& component,
		const std::vector<glm::mat4>& matrices)
	{
		if (!geometry_shader || !component.cachedModelAsset || matrices.empty())
			return;
		if (!component.bufferOffset.isUploaded)
			return;

		const int instanceCount = static_cast<int>(matrices.size());

		glBindBuffer(GL_ARRAY_BUFFER, geometry_ibo);
		const GLsizeiptr needed = instanceCount * sizeof(glm::mat4);
		if (geometry_ibo_capacity < needed) {
			geometry_ibo_capacity = needed * 2;
			glBufferData(GL_ARRAY_BUFFER, geometry_ibo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, needed, matrices.data());

		geometry_shader->SetUniform("u_Instanced", 1.0f);
		geometry_shader->SetUniform("u_Animated",  0.0f);
		geometry_shader->SetUniform("u_InvertUvY", 0.0f);
		geometry_shader->SetUniform("DEBUG_TYPE", (float)GraphicsSettings::get().DEBUG_PBR_MAP_TYPE);

		auto assetManager = services->get<Assets::Manager>();
		const auto& modelAsset = component.cachedModelAsset;

		glBindVertexArray(geometry_vao);

		for (size_t i = 0; i < modelAsset->submeshes.size(); ++i) {
			const auto& submesh = modelAsset->submeshes[i];
			if (submesh.materialIndex >= component.materials.size())
				continue;

			// ========================================
			// PERFORMANCE OPTIMIZATION: Use Cached Material State
			// ========================================
			const bool hasRuntimeOverrides = component.materials[submesh.materialIndex].useOverrides;
			if (!hasRuntimeOverrides &&
				i < component.submeshCaches.size() &&
				component.submeshCaches[i].cacheValid) {
				ApplyCachedGeometryMaterialState(geometry_shader, component.submeshCaches[i]);
			} else {
				// Fallback: build material state per-frame
				const GeometryMaterialState state =
					BuildGeometryMaterialState(assetManager, component, i);
				ApplyGeometryMaterialState(geometry_shader, state);
			}

			glDrawElementsInstanced(
				GL_TRIANGLES, submesh.indexCount, GL_UNSIGNED_INT,
				(void*)((component.bufferOffset.indexOffset + submesh.firstIndex) * sizeof(unsigned int)),
				instanceCount);
		}

		// Restore non-instanced mode for subsequent DrawGeometry calls
		geometry_shader->SetUniform("u_Instanced", 0.0f);
		geometry_shader->SetUniform("u_Animated", 0.0f);
		geometry_shader->SetUniform("u_UseEmissionOverride", 0.0f);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void WindowsRenderer::EndGeometryPass() {
		if (geometry_shader) {
			geometry_shader->Bind();
			geometry_shader->SetUniform("u_Instanced", 0.0f);
			geometry_shader->SetUniform("u_Animated", 0.0f);
			geometry_shader->SetUniform("u_InvertUvY", 0.0f);
			geometry_shader->SetUniform("u_UseEmissionOverride", 0.0f);
			geometry_shader->SetUniform("u_EmissionOverride", glm::vec3(0.0f));
		}
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void WindowsRenderer::BeginMinimapPass(const glm::mat4& view,
		const glm::mat4& proj) {
		(void)view;
		(void)proj;
		const PAIN::Render::MinimapBackgroundTargetSpec minimapTargetSpec =
			PAIN::Render::BuildMinimapBackgroundTargetSpec();
		const GLint minimapInternalFormat =
			minimapTargetSpec.colorFormat == PAIN::Render::MinimapColorFormat::RGBA16F
			? GL_RGBA16F
			: GL_RGBA8;
		const GLenum minimapPixelType =
			minimapTargetSpec.colorFormat == PAIN::Render::MinimapColorFormat::RGBA16F
			? GL_FLOAT
			: GL_UNSIGNED_BYTE;

		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &minimap_prev_fbo);
		glGetIntegerv(GL_VIEWPORT, minimap_prev_viewport);
		glGetFloatv(GL_COLOR_CLEAR_VALUE, minimap_prev_clear_color);
		minimap_prev_depth_test = glIsEnabled(GL_DEPTH_TEST);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &minimap_prev_depth_mask);
		minimap_prev_blend = glIsEnabled(GL_BLEND);
		glGetIntegerv(GL_BLEND_SRC_RGB, &minimap_prev_blend_src_rgb);
		glGetIntegerv(GL_BLEND_DST_RGB, &minimap_prev_blend_dst_rgb);
		glGetIntegerv(GL_BLEND_SRC_ALPHA, &minimap_prev_blend_src_alpha);
		glGetIntegerv(GL_BLEND_DST_ALPHA, &minimap_prev_blend_dst_alpha);
		minimap_state_saved = true;

		const glm::vec2 minimap_size = GraphicsSettings::get().minimap_size_px;
		const int target_width = std::max(64, static_cast<int>(minimap_size.x));
		// Runtime validation: for circular minimap, clamp height to width for perfect circle
		int target_height;
		if (GraphicsSettings::get().minimap_shape == GraphicsSettings::MINIMAP_SHAPE_CIRCLE) {
			target_height = std::max(64, static_cast<int>(glm::min(minimap_size.x, minimap_size.y)));
		} else {
			target_height = std::max(64, static_cast<int>(minimap_size.y));
		}

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
			glTexImage2D(GL_TEXTURE_2D, 0, minimapInternalFormat, minimap_width, minimap_height, 0,
				GL_RGBA, minimapPixelType, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
				minimap_texture, 0);

			if (minimapTargetSpec.needsDepthStencil) {
				glGenRenderbuffers(1, &minimap_rbo);
				glBindRenderbuffer(GL_RENDERBUFFER, minimap_rbo);
				glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, minimap_width,
					minimap_height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
					GL_RENDERBUFFER, minimap_rbo);
			}
			else {
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
					GL_RENDERBUFFER, 0);
			}

			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (status != GL_FRAMEBUFFER_COMPLETE) {
				PN_CORE_ERROR("Minimap framebuffer is incomplete after resize! Status: 0x{:x}",
					status);
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, minimap_fbo);
		glViewport(0, 0, minimap_width, minimap_height);
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);

		const float bg_alpha = glm::clamp(GraphicsSettings::get().minimap_background_alpha,
			0.0f, 1.0f);
		glClearColor(0.02f, 0.02f, 0.02f, bg_alpha);
		glClear(GL_COLOR_BUFFER_BIT);
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
		glDepthMask(minimap_prev_depth_mask == GL_TRUE ? GL_TRUE : GL_FALSE);
		if (minimap_prev_blend) {
			glEnable(GL_BLEND);
		} else {
			glDisable(GL_BLEND);
		}
		glBlendFuncSeparate(minimap_prev_blend_src_rgb, minimap_prev_blend_dst_rgb,
			minimap_prev_blend_src_alpha, minimap_prev_blend_dst_alpha);

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
									   const glm::vec4& color,
									   const glm::vec4& accentColor,
									   float patternStrength,
									   float patternScale,
									   float patternPhase) {
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
		minimap_wall_shader->SetUniform("u_AccentColor", accentColor);
		minimap_wall_shader->SetUniform("u_PatternStrength", patternStrength);
		minimap_wall_shader->SetUniform("u_PatternScale", patternScale);
		minimap_wall_shader->SetUniform("u_PatternPhase", patternPhase);

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

	// Lighting pass entry point: sysRender dispatches the frame stage, while the
	// renderer consumes the G-buffer, light data, and IBL inputs to fill final_fbo.
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

#ifdef _DEBUG
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err before lighting pass: {}", err);
		}
#endif

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
				const GLuint materialTextureForLighting =
					(active_gbuffer_count >= 4 && material_properties_texture != 0)
						? material_properties_texture
						: fallback_material_texture;
				const GLuint emissionTextureForLighting =
					(active_gbuffer_count >= 5 && emission_texture != 0)
						? emission_texture
						: fallback_emission_texture;

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, pos_texture);

				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, col_texture);

				glActiveTexture(GL_TEXTURE2);
				glBindTexture(GL_TEXTURE_2D, norm_texture);

				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, materialTextureForLighting);

				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, emissionTextureForLighting);
			}

#ifdef _DEBUG
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after binding gbuffer textures: {}", err);
			}
#endif

			// OPTIMIZATION: Don't pre-bind all shadow slots to 0.
			// Instead, only bind shadow textures that are actually used.
			// The shader will use the shadowMapIdx uniform to determine which slots have shadows.

			struct ShadowMapCandidate {
				int gpuLightIndex = -1;
				std::string key;
				GLuint shadowTexture = 0;
				int priority = 0;
				int insertionOrder = 0;
			};

			static std::unordered_set<std::string> loggedShadowBudgetDrops;

			int shadowMapCount = 0;
			int i{};
			int mappedLightOrder = 0;
			std::array<PbrLightGpuData, kMaxPbrLights> gpuLights{};
			std::vector<ShadowMapCandidate> shadowCandidates;
			shadowCandidates.reserve(kMaxPbrLights);

			const auto allLightsWithKeys = lights.getAllWithKeys();
			for (const auto& [lightKey, lightRef] : allLightsWithKeys) {
				const Light& l = lightRef.get();
				if (i >= kMaxPbrLights) {
					PN_CORE_WARN("[GL] Skipping extra light because shader budget is {}", kMaxPbrLights);
					break;
				}

				PbrLightGpuData& gpuLight = gpuLights[i];
				gpuLight.position_type = glm::vec4(l.position, static_cast<float>(l.type));
				gpuLight.intensity_shadow = glm::vec4(
					lights.lightsOn ? l.L_intensity : glm::vec3(0.0f),
					-1.0f);
				gpuLight.direction_inner = glm::vec4(
					l.direction,
					glm::cos(glm::radians(l.inner_angle)));
				gpuLight.outer_padding = glm::vec4(
					glm::cos(glm::radians(l.outer_angle)),
					0.0f,
					0.0f,
					0.0f);
				gpuLight.view = l.view();
				gpuLight.projection = l.projection();

				if (l.getShadowType() == Light::SHADOW_TYPES::MAPPED &&
					l.getShadowTexture() != 0) {
					int priority = 0;
					if (l.type == Light::TYPES::SPOTLIGHT) priority += 60;
					if (l.volumetric) priority += 30;
					if (l.volumetric && l.type == Light::TYPES::SPOTLIGHT) priority += 40;
					if (lightKey == "world") priority += 5;

					shadowCandidates.push_back({
						i,
						lightKey,
						l.getShadowTexture(),
						priority,
						mappedLightOrder++
					});
				}

				i++;
			}

			std::sort(shadowCandidates.begin(), shadowCandidates.end(),
				[](const ShadowMapCandidate& lhs, const ShadowMapCandidate& rhs) {
					if (lhs.priority != rhs.priority) {
						return lhs.priority > rhs.priority;
					}
					return lhs.insertionOrder < rhs.insertionOrder;
				});

			for (const ShadowMapCandidate& candidate : shadowCandidates) {
				if (shadowMapCount >= kMaxPbrShadowMaps) {
					if (loggedShadowBudgetDrops.insert(candidate.key).second) {
						PN_CORE_WARN(
							"[GL] Shadow budget drop: light '{}' did not get a shadow map slot (budget={})",
							candidate.key,
							kMaxPbrShadowMaps);
					}
					continue;
				}

				const int textureUnit = kFixedShadowTextureUnitStart + shadowMapCount;
				glActiveTexture(GL_TEXTURE0 + textureUnit);
				glBindTexture(GL_TEXTURE_2D, candidate.shadowTexture);
				// Set shadow sampler uniform only for slots we actually use
				pbr_shader->SetUniform(GetPbrUniformNames().shadowSamplers[shadowMapCount],
									   textureUnit);
				gpuLights[candidate.gpuLightIndex].intensity_shadow.w =
					static_cast<float>(shadowMapCount);
				loggedShadowBudgetDrops.erase(candidate.key);
				++shadowMapCount;
			}

			if (pbr_light_ubo != 0) {
				const GLuint pbrProgram = pbr_shader->GetRendererID();
				if (pbrProgram != pbr_light_ubo_bound_program) {
					const GLuint lightBlockIndex = glGetUniformBlockIndex(pbrProgram, "PbrLightBlock");
					if (lightBlockIndex == GL_INVALID_INDEX) {
						PN_CORE_ERROR("PBR shader program {} missing PbrLightBlock; aborting LightingPass", pbrProgram);
						glEnable(GL_DEPTH_TEST);
						glDepthMask(GL_TRUE);
						return;
					}
					glUniformBlockBinding(pbrProgram, lightBlockIndex, kPbrLightUboBindingPoint);
					pbr_light_ubo_bound_program = pbrProgram;
				}
				glBindBuffer(GL_UNIFORM_BUFFER, pbr_light_ubo);
				glBufferSubData(
					GL_UNIFORM_BUFFER,
					0,
					static_cast<GLsizeiptr>(sizeof(PbrLightGpuData) * kMaxPbrLights),
					gpuLights.data());
				glBindBufferBase(GL_UNIFORM_BUFFER, kPbrLightUboBindingPoint, pbr_light_ubo);
				glBindBuffer(GL_UNIFORM_BUFFER, 0);
			}

#ifdef _DEBUG
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after setting light uniforms: {}", err);
			}
#endif

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
			pbr_shader->SetUniform("u_NumLights", static_cast<float>(i));
			pbr_shader->SetUniform("u_AmbientLight", GraphicsSettings::get().AMBIENT_LIGHT);

#ifdef _DEBUG
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after setting lighting pbr uniforms: {}", err);
			}
#endif

			// for image based lighting
			pbr_shader->SetUniform("u_CamPos", scene->GetActiveCamera()->pos);
			const bool iblAvailable = GraphicsSettings::get().ibl
				&& Skybox::get().getIrradianceMap() != 0
				&& Skybox::get().getPrefilterMap() != 0
				&& Skybox::get().getBrdfLUT() != 0;
			static bool loggedMissingIbl = false;
			if (GraphicsSettings::get().ibl && !iblAvailable && !loggedMissingIbl) {
				PN_CORE_WARN("[GL] IBL requested but irradiance/prefilter/BRDF resources are incomplete. Falling back to ambient lighting.");
				loggedMissingIbl = true;
			}
			if (iblAvailable) {
				loggedMissingIbl = false;
			}
			pbr_shader->SetUniform("u_UseIbl", iblAvailable ? 1.f : 0.f);
			pbr_shader->SetUniform("u_IblDiffuseStrength", GraphicsSettings::get().ibl_diffuse_strength);
			pbr_shader->SetUniform("u_IblSpecularStrength", GraphicsSettings::get().ibl_specular_strength);
			pbr_shader->SetUniform("u_IblRoughnessBias", GraphicsSettings::get().ibl_roughness_bias);
			pbr_shader->SetUniform("u_IblSpecularMipBias", GraphicsSettings::get().ibl_specular_mip_bias);
			pbr_shader->SetUniform("u_IblSpecularStrengthScale", GraphicsSettings::get().ibl_specular_strength_scale);
			pbr_shader->SetUniform("u_IblSpecularPrefilterLumaClamp", GraphicsSettings::get().ibl_specular_prefilter_luma_clamp);
			pbr_shader->SetUniform("u_IblSpecularFireflyClamp", GraphicsSettings::get().ibl_specular_firefly_clamp);
			const float maxReflectionLod = std::min(
				GraphicsSettings::get().ibl_max_reflection_lod,
				Skybox::get().getPrefilterMaxReflectionLod());
			pbr_shader->SetUniform("u_IblMaxReflectionLod", maxReflectionLod);
			pbr_shader->SetUniform("u_ViewportWidth", static_cast<float>(winWidth));

			glActiveTexture(GL_TEXTURE0 + kIrradianceTextureUnit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox::get().getIrradianceMap());
			pbr_shader->SetUniform("irradianceMap", kIrradianceTextureUnit);

			glActiveTexture(GL_TEXTURE0 + kPrefilterTextureUnit);
			glBindTexture(GL_TEXTURE_CUBE_MAP, Skybox::get().getPrefilterMap());
			pbr_shader->SetUniform("prefilterMap", kPrefilterTextureUnit);

			glActiveTexture(GL_TEXTURE0 + kBrdfLutTextureUnit);
			glBindTexture(GL_TEXTURE_2D, Skybox::get().getBrdfLUT());
			pbr_shader->SetUniform("brdfLut", kBrdfLutTextureUnit);

			// SSAO
			const bool useSsao = GraphicsSettings::get().ssao && ssao_blur_texture != 0;
			glActiveTexture(GL_TEXTURE0 + LightingPass::kSsao);
			glBindTexture(GL_TEXTURE_2D, useSsao ? ssao_blur_texture : 0);
			pbr_shader->SetUniform("u_SsaoTex", LightingPass::kSsao);
			pbr_shader->SetUniform("u_UseSsao", useSsao ? 1.0f : 0.0f);

#ifdef _DEBUG
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after setting ibl uniforms: {}", err);
			}
#endif

#ifdef _DEBUG
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
#endif

			// #endif

			glBindVertexArray(passthrough_vao);
#ifdef _DEBUG
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
#endif

			glDrawArrays(GL_TRIANGLES, 0, 6);

#ifdef _DEBUG
			err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after drawing lighting pass: {} ({})", err, DescribeGlError(err));
				LogLightingDrawDiagnostics(
					pbr_shader ? pbr_shader->GetRendererID() : 0,
					passthrough_vao,
					final_fbo,
					kLightingTextureUnitsUsed);
			}
#endif
		}

		glEnable(GL_DEPTH_TEST);

		// After lighting pass, final_fbo has the lit scene but NO depth buffer yet
		// So we copy it:
		glBindFramebuffer(GL_READ_FRAMEBUFFER, ds_fbo);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, final_fbo);
		glBlitFramebuffer(0, 0, winWidth, winHeight, 0, 0, winWidth, winHeight,
						  GL_DEPTH_BUFFER_BIT, GL_NEAREST); // Copy depth only

#ifdef _DEBUG
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after blitting depth buffer: {} ({})", err, DescribeGlError(err));
			LogDepthBlitDiagnostics(ds_fbo, final_fbo);
		}
#endif

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

#ifdef _DEBUG
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after drawing skybox in lighting pass: {}", err);
		}
#endif
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
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

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
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

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
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

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
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

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
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

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
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

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
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

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
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

		debug_shader->Bind();
		glm::mat4 ortho_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		debug_shader->SetUniform("u_V", glm::mat4(1.0f));
		debug_shader->SetUniform("u_P", ortho_proj);

		glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(verts.size() / 7));

		glBindVertexArray(0);
	}

	// Volumetric pass entry point: the renderer accumulates screen-space fog/light
	// history after direct lighting and before particles, debug overlays, and post FX.
	void WindowsRenderer::VolumetricPass(std::shared_ptr<Scene::SceneManager> scene,
										 const LightSources& lights) {
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
			if (!l.volumetric) {
				continue;
			}
			const bool hasShadowMap =
				l.getShadowType() == Light::SHADOW_TYPES::MAPPED && l.getShadowTexture() != 0;
			
			// REQUIRE shadow map for all volumetric lights to prevent light leaking
			// through geometry. Without a shadow map, we cannot determine occlusion.
			if (!hasShadowMap) {
				continue;
			}

			const glm::vec3 influenceCenter = GetVolumetricInfluenceCenter(l, gs.volumetric_max_dist);
			const VolumetricVisibilityMetrics visibility =
				ComputeVolumetricVisibilityMetrics(*cam, cameraFrustum, l, gs.volumetric_max_dist);
			const float distToCamera = glm::distance(cam->pos, influenceCenter);
			const bool hysteresisActive = volumetric_selection_ttl.count(key) > 0;
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

		// Use pre-cached shadow uniform names to avoid per-frame string allocation
		const auto& shadowUniformNames = GetVolumetricShadowUniformNames();

		for (size_t lightIdx = 0; lightIdx < packedLights.size(); ++lightIdx) {
			const PackedVolumetricLight& packed = packedLights[lightIdx];
			const Light& l = *packed.light;
			if (packed.shadowMapIdx >= 0) {
				glActiveTexture(GL_TEXTURE0 + packed.shadowTextureUnit);
				glBindTexture(GL_TEXTURE_2D, l.getShadowTexture());
				// Use pre-cached uniform name instead of string concatenation
				volumetric_shader->SetUniform(
					shadowUniformNames.shadowSamplers[packed.shadowMapIdx],
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

#ifdef _DEBUG
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after VolumetricPass: {}", err);
		}
#endif
	}

	void WindowsRenderer::SsaoPass(std::shared_ptr<Scene::SceneManager> scene) {
		const auto& gs = GraphicsSettings::get();
		if (!gs.ssao || !ssao_shader || !ssao_blur_shader || !scene) return;
		if (ssao_fbo == 0 || ssao_blur_fbo == 0 || ssao_noise_texture == 0) return;

		auto* cam = scene->GetActiveCamera();
		if (!cam) return;

		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_BLEND);

		// --- Raw SSAO pass ---
		glBindFramebuffer(GL_FRAMEBUFFER, ssao_fbo);
		glViewport(0, 0, winWidth, winHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		ssao_shader->Bind();
		ssao_shader->SetUniform("gPos",  0);
		ssao_shader->SetUniform("gNorm", 1);
		ssao_shader->SetUniform("u_Noise", 2);
		ssao_shader->SetUniform("u_V", cam->view());
		ssao_shader->SetUniform("u_P", cam->projection());
		ssao_shader->SetUniform("u_ScreenSize", glm::vec2(float(winWidth), float(winHeight)));
		ssao_shader->SetUniform("u_Radius", gs.ssao_radius);
		ssao_shader->SetUniform("u_Bias",   gs.ssao_bias);

		{
			GLint kernelLoc = glGetUniformLocation(ssao_shader->GetRendererID(), "u_Kernel");
			if (kernelLoc != -1) {
				glUniform3fv(kernelLoc, static_cast<GLsizei>(ssao_kernel.size()), glm::value_ptr(ssao_kernel[0]));
			}
		}

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, pos_texture);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, norm_texture);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, ssao_noise_texture);

		glBindVertexArray(empty_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		// --- SSAO blur pass ---
		glBindFramebuffer(GL_FRAMEBUFFER, ssao_blur_fbo);
		glViewport(0, 0, winWidth, winHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		ssao_blur_shader->Bind();
		ssao_blur_shader->SetUniform("u_Ssao", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, ssao_texture);

		glBindVertexArray(empty_vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}

	// Post-process entry point: final_texture remains the renderer-owned scene output,
	// while sysRender decides whether the frame is ultimately presented to editor or swapchain.
	void WindowsRenderer::PostProcessPass(bool presentToSwapchain) {
#ifdef _DEBUG
		const auto savedState = CaptureGlState();
		auto restorePassState = [&]() {
			RestoreGlState(savedState);
		};
#else
		auto restorePassState = []() {};
#endif

		if (final_texture == 0) {
			PN_CORE_ERROR("[GL] PostProcessPass aborted because final_texture is invalid.");
			restorePassState();
			return;
		}
#ifdef _DEBUG
		if (!ValidateFramebufferBinding(final_fbo, "Post-process final framebuffer") ||
			!ValidateFramebufferBinding(pp_fbo, "Post-process ping framebuffer") ||
			!ValidateFramebufferBinding(pp2_fbo, "Post-process pong framebuffer")) {
			restorePassState();
			return;
		}
#endif

		// ========================================
		// POST-PROCESS: ALWAYS DISABLE DEPTH TEST
		// Full-screen quads must not be rejected by
		// scene geometry depth stored in final_fbo
		// ========================================
		glDisable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glDisable(GL_BLEND);

#ifdef _DEBUG
		GLenum err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err before tone mapping pass: {}", err);
		}
#endif

		int postprocess_passes = 0;

		// bloom pass
		if (GraphicsSettings::get().bloom) {
			// save scene_tex to final_texture first
			if (postprocess_passes % 2) {
				glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
				glViewport(0, 0, winWidth, winHeight);
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
				glViewport(0, 0, pp_width, pp_height);
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
					glViewport(0, 0, pp_width, pp_height);

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
				glViewport(0, 0, pp_width, pp_height);
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
				glViewport(0, 0, winWidth, winHeight);
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
		// OPTIMIZATION: Combine tone mapping + gamma into single pass when tone_gamma_shader available
		{
			const unsigned int dest_fbo =
				postprocess_passes % 2 == 0 ? pp_fbo : final_fbo;
			const unsigned int src_tex =
				postprocess_passes % 2 == 0 ? final_texture : pp_texture;

			glCheck(glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo));
			// Set viewport: use pp resolution for pp_fbo, full resolution for final_fbo
			glCheck(glViewport(0, 0, dest_fbo == pp_fbo ? pp_width : winWidth, 
							  dest_fbo == pp_fbo ? pp_height : winHeight));
			
			// Use combined tone+gamma shader if available (eliminates one full-screen pass)
			if (tone_gamma_shader) {
				const auto& gs = GraphicsSettings::get();
				const float safeGamma = glm::max(0.001f, gs.gamma_value);
				const float combinedGamma = gs.gamma_correction ? safeGamma : 1.0f;
				tone_gamma_shader->Bind();
				glCheck(glActiveTexture(GL_TEXTURE0));
				glCheck(glBindTexture(GL_TEXTURE_2D, src_tex));
				glCheck(tone_gamma_shader->SetUniform("tex", 0));
				glCheck(tone_gamma_shader->SetUniform(
					"exposure", gs.tone_mapping_exposure));
				glCheck(tone_gamma_shader->SetUniform(
					"toneMapMode",
					static_cast<float>(gs.tone_mapping_mode)));
				glCheck(tone_gamma_shader->SetUniform(
					"u_gamma", combinedGamma));
			}
			else {
				tone_shader->Bind();
				glCheck(glActiveTexture(GL_TEXTURE0));
				glCheck(glBindTexture(GL_TEXTURE_2D, src_tex));
				glCheck(tone_shader->SetUniform("tex", 0));
				glCheck(tone_shader->SetUniform(
					"exposure", GraphicsSettings::get().tone_mapping_exposure));
				glCheck(tone_shader->SetUniform(
					"toneMapMode",
					static_cast<float>(GraphicsSettings::get().tone_mapping_mode)));
			}
			
			glCheck(glBindVertexArray(empty_vao));
			glCheck(glDrawArrays(GL_TRIANGLE_STRIP, 0, 4));
			++postprocess_passes;
		}
#ifdef _DEBUG
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after tone mapping pass: {}", err);
		}
#endif

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
				glViewport(0, 0, dest_fbo == pp_fbo ? pp_width : winWidth, 
						  dest_fbo == pp_fbo ? pp_height : winHeight);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, src_tex);
				blur_shader->SetUniform("is_horizontal_pass", i % 2 ? 0.f : 1.f);
				glBindVertexArray(empty_vao);
				glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
				++postprocess_passes;
			}
		}
#ifdef _DEBUG
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after blur pass: {}", err);
		}
#endif

		// gamma correction
		// OPTIMIZATION: Skip if using combined tone+gamma shader
		if (GraphicsSettings::get().gamma_correction && !tone_gamma_shader) {
			const unsigned int dest_fbo =
				postprocess_passes % 2 == 0 ? pp_fbo : final_fbo;
			const unsigned int src_tex =
				postprocess_passes % 2 == 0 ? final_texture : pp_texture;

			glBindFramebuffer(GL_FRAMEBUFFER, dest_fbo);
			glViewport(0, 0, dest_fbo == pp_fbo ? pp_width : winWidth, 
					  dest_fbo == pp_fbo ? pp_height : winHeight);
			gamma_shader->Bind();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, src_tex);
			gamma_shader->SetUniform("tex", 0);
			gamma_shader->SetUniform("u_gamma", glm::max(0.001f, GraphicsSettings::get().gamma_value));
			glBindVertexArray(empty_vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			++postprocess_passes;
		}

#ifdef _DEBUG
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after gamma pass: {}", err);
		}
#endif

		// make sure final_texture now holds the gamma corrected texture
		// use passthrough to render pp_texture to final_texture if odd number of
		// passes
		if (postprocess_passes % 2) {
			glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
			glViewport(0, 0, winWidth, winHeight);
			passthrough_shader->Bind();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, pp_texture);
			passthrough_shader->SetUniform("tex", 0);
			glBindVertexArray(passthrough_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
#ifdef _DEBUG
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after finalizing post process pass: {}", err);
		}
#endif

		// FXAA pass (runs at full resolution on LDR output, after tone mapping + gamma)
		if (GraphicsSettings::get().fxaa && fxaa_shader) {
			// Read final_texture → write to pp_fbo
			glBindFramebuffer(GL_FRAMEBUFFER, pp_fbo);
			glViewport(0, 0, pp_width, pp_height);
			fxaa_shader->Bind();
			fxaa_shader->SetUniform("tex", 0);
			fxaa_shader->SetUniform("u_texel_size", glm::vec2(1.0f / pp_width, 1.0f / pp_height));
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, final_texture);
			glBindVertexArray(empty_vao);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			// Copy pp_texture back into final_fbo so final_texture holds the result
			glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);
			glViewport(0, 0, winWidth, winHeight);
			passthrough_shader->Bind();
			passthrough_shader->SetUniform("tex", 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, pp_texture);
			glBindVertexArray(passthrough_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
#ifdef _DEBUG
		{
			GLenum err = glGetError();
			if (err != GL_NO_ERROR) {
				PN_CORE_ERROR("OpenGL err after FXAA pass: {}", err);
			}
		}
#endif

		// Keep final_fbo/final_texture as the renderer-owned postprocess output.
		glBindFramebuffer(GL_FRAMEBUFFER, final_fbo);

		if (presentToSwapchain) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, winWidth, winHeight);
			passthrough_shader->Bind();
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, final_texture);
			passthrough_shader->SetUniform("tex", 0);
			glBindVertexArray(passthrough_vao);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		restorePassState();

		// OPTIMIZATION: Invalidate framebuffer attachments to help GPU memory bandwidth
		// This tells the GPU we don't need to preserve these textures for future use
		InvalidateFramebufferAttachments(pp_fbo, true, false, false);
		InvalidateFramebufferAttachments(pp2_fbo, true, false, false);

#ifdef _DEBUG
		err = glGetError();
		if (err != GL_NO_ERROR) {
			PN_CORE_ERROR("OpenGL err after PostProcessPass: {}", err);
		}
#endif
	}

	void WindowsRenderer::BlitFinalToScreen()
	{
		// Simple blit from final_fbo to screen without any post-processing
		// Used when postprocess is disabled
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, winWidth, winHeight);
		
		passthrough_shader->Bind();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, final_texture);
		passthrough_shader->SetUniform("tex", 0);
		
		glBindVertexArray(passthrough_vao);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	void WindowsRenderer::InvalidateFramebufferAttachments(GLuint fbo, bool invalidateColor,
											 bool invalidateDepth,
											 bool invalidateStencil) {
#ifdef PN_PLATFORM_ANDROID
		if (fbo == 0) {
			return;
		}

		GLenum attachments[3];
		int count = 0;

		if (invalidateColor) {
			attachments[count++] = GL_COLOR_ATTACHMENT0;
		}
		if (invalidateDepth) {
			attachments[count++] = GL_DEPTH_ATTACHMENT;
		}
		if (invalidateStencil) {
			attachments[count++] = GL_STENCIL_ATTACHMENT;
		}

		if (count == 0) {
			return;
		}

		GLint prevFbo = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glInvalidateFramebuffer(GL_FRAMEBUFFER, count, attachments);
		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
#else
		// Windows/Desktop OpenGL: glInvalidateFramebuffer is available in OpenGL 4.3+
		// (ARB_invalidate_subdata extension). Most modern GPUs support this.
		// If not available, this becomes a no-op which is safe.
		(void)fbo;
		(void)invalidateColor;
		(void)invalidateDepth;
		(void)invalidateStencil;
		
		// Try to use glInvalidateFramebuffer if available (OpenGL 4.3+)
		// This is a performance optimization that helps GPU memory bandwidth
#if defined(GL_VERSION_4_3) || defined(GL_ARB_invalidate_subdata)
#ifdef PN_PLATFORM_WINDOWS
		if (!(GLEW_VERSION_4_3 || GLEW_ARB_invalidate_subdata)) {
			return;
		}
#endif
		if (fbo == 0) {
			return;
		}

		GLenum attachments[3];
		int count = 0;

		if (invalidateColor) {
			attachments[count++] = GL_COLOR_ATTACHMENT0;
		}
		if (invalidateDepth) {
			attachments[count++] = GL_DEPTH_ATTACHMENT;
		}
		if (invalidateStencil) {
			attachments[count++] = GL_STENCIL_ATTACHMENT;
		}

		if (count == 0) {
			return;
		}

		GLint prevFbo = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glInvalidateFramebuffer(GL_FRAMEBUFFER, count, attachments);
		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
#endif
#endif
	}

	void WindowsRenderer::BeginCircularStencilClip(const glm::vec2& center_ndc, const glm::vec2& radius_ndc) {
		if (!debug_shader) return;

		// Save current state
		GLboolean colorMask[4];
		glGetBooleanv(GL_COLOR_WRITEMASK, colorMask);

		// Clear stencil
		glClear(GL_STENCIL_BUFFER_BIT);

		// Draw filled circle to stencil buffer
		const int segments = 64;
		std::vector<float> verts;
		verts.reserve(static_cast<size_t>(segments) * 3 * 7);

		for (int i = 0; i < segments; ++i) {
			const float a0 = (static_cast<float>(i) / static_cast<float>(segments)) * glm::two_pi<float>();
			const float a1 = (static_cast<float>(i + 1) / static_cast<float>(segments)) * glm::two_pi<float>();

			const glm::vec2 p0(center_ndc.x + std::cos(a0) * radius_ndc.x,
							   center_ndc.y + std::sin(a0) * radius_ndc.y);
			const glm::vec2 p1(center_ndc.x + std::cos(a1) * radius_ndc.x,
							   center_ndc.y + std::sin(a1) * radius_ndc.y);

			// Triangle: center, p0, p1
			verts.insert(verts.end(), {center_ndc.x, center_ndc.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f});
			verts.insert(verts.end(), {p0.x, p0.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f});
			verts.insert(verts.end(), {p1.x, p1.y, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f});
		}

		glBindVertexArray(debug_VAO);
		glBindBuffer(GL_ARRAY_BUFFER, debug_VBO);
		const GLsizeiptr debugBytes = static_cast<GLsizeiptr>(verts.size() * sizeof(float));
		if (debug_vbo_capacity < debugBytes) {
			debug_vbo_capacity = debugBytes * 2;
			glBufferData(GL_ARRAY_BUFFER, debug_vbo_capacity, nullptr, GL_DYNAMIC_DRAW);
		}
		glBufferSubData(GL_ARRAY_BUFFER, 0, debugBytes, verts.data());

		// Color mask off - only writing to stencil
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		// Setup stencil to write 1 where the circle triangles pass
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilMask(0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		debug_shader->Bind();
		glm::mat4 ortho_proj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
		debug_shader->SetUniform("u_V", glm::mat4(1.0f));
		debug_shader->SetUniform("u_P", ortho_proj);

		glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(verts.size() / 7));

		// Restore color mask
		glColorMask(colorMask[0], colorMask[1], colorMask[2], colorMask[3]);

		// Enable stencil test for subsequent draws (only pass where stencil == 1)
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilMask(0x00); // Don't modify stencil
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}

	void WindowsRenderer::EndCircularStencilClip() {
		glDisable(GL_STENCIL_TEST);
	}

	void WindowsRenderer::Cleanup() {
		if (passthrough_vao != 0) {
			glDeleteVertexArrays(1, &passthrough_vao);
			passthrough_vao = 0;
		}

		if (passthrough_vbo != 0) {
			glDeleteBuffers(1, &passthrough_vbo);
			passthrough_vbo = 0;
		}

		if (pbr_light_ubo != 0) {
			glDeleteBuffers(1, &pbr_light_ubo);
			pbr_light_ubo = 0;
			pbr_light_ubo_bound_program = 0;
		}

		if (bone_matrix_ubo != 0) {
			glDeleteBuffers(1, &bone_matrix_ubo);
			bone_matrix_ubo = 0;
			bone_matrix_ubo_bound_program = 0;
		}

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
		clamp_configured_textures.clear();

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


