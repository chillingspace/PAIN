/**
 * @file GraphicsSettings.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2025-10-02
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#pragma once


namespace PAIN {
	class GraphicsSettings {
	private:
		GraphicsSettings() = default;
		~GraphicsSettings() = default;
	public:
		static GraphicsSettings& get() {
			static GraphicsSettings instance;
			return instance;
		}

		// softer shadows look better, but requires more VRAM
		enum class SHADOW_TYPES {
			SOFTEST,
			SOFT,
			HARD
		};

		enum class TONE_MAPPING_TYPES {
			NONE = 0,
			ACES,
			REINHARD,
			UNCHARTED2,
			NUM_TONE_MAPPING_TYPES
		};

		// width = height for shadow maps. dont follow screen resolution
		const std::unordered_map<SHADOW_TYPES, int> SHADOW_MAP_WIDTHS{
			{SHADOW_TYPES::SOFTEST, 4096},
			{SHADOW_TYPES::SOFT, 2048},
			{SHADOW_TYPES::HARD, 1024}
		};

		int getShadowMapWidth() const {
			return SHADOW_MAP_WIDTHS.at(shadow_type);
		}

		// actual settings
		bool draw_floor = false;

		SHADOW_TYPES shadow_type = SHADOW_TYPES::SOFTEST;
		bool gamma_correction = true;
		glm::vec3 AMBIENT_LIGHT = glm::vec3(0.f);
		bool world_light = true;
		float fov = 90.f;
		
		int blur_quality = 4;	// number of blur passes. higher = blurrier, BUT SLOWER, REPRESENTS GAUSSIAN BLUR PASSES, SO MINIMALLY 2. too high won't help. sublinear growth.
		// using hdr, so range of [0,inf)
		float blur_strength = 0.f;

		bool bloom = true;
		float bloom_threshold = 2.f;			// generally [0.8,1.5] - min brightness to bloom. 0 = disabled(technically blooms everything but why would we want that)
		float bloom_blur_strength = 1.f;	// generally [0.5,10] - higher = bloomier, BUT SLOWER. bloom blur strength is how big the blur radius is when blurring the bright areas.
		float bloom_strength = 1.f;		// generally [0.0,5.0] - bloom strength is how visible the bloom is when blended back onto the scene. 
		float global_light_intensity = 1.5f;
		int bloom_quality = 4;		// number of blur passes for bloom. higher = bloomier, REPRESENTS GAUSSIAN BLUR PASSES, SO MINIMALLY 2

		TONE_MAPPING_TYPES tone_mapping_mode = TONE_MAPPING_TYPES::ACES;
		float tone_mapping_exposure = 1.f;

		// image based lighting
		bool ibl = true;

		// animation
		bool interpolate_animation{ true };		// smoother animations at the expense of performance

		
		// debug settings
		bool DEBUG_USE_DIFFUSE_MAP{ true };
		bool DEBUG_USE_AO_MAP{ true };
		bool DEBUG_USE_NORMAL_MAP{ true };
		bool DEBUG_USE_ROUGHNESSMETALLIC_MAP{ true };
		bool DEBUG_USE_EMISSION_MAP{ true };

		enum DEBUG_PBR_MAP_TYPES {
			NONE = 0,
			OBJECT_ONLY,
			DIFFUSE,
			AO,
			NORMAL,
			ROUGHNESS,
			METALLIC,
			EMISSION,
			IBL_IRRADIANCE,
			IBL_PREFILTER,
			IBL_BRDFLUT,
			NUM_PBR_MAP_TYPES,
		};

		DEBUG_PBR_MAP_TYPES DEBUG_PBR_MAP_TYPE = DEBUG_PBR_MAP_TYPES::NONE;
	};
}