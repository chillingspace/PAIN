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

#include <unordered_map>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "AssetData.h"

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
		bool decode_albedo_in_shader = true; // Set false when albedo textures are uploaded as sRGB formats with hardware decode.

		struct RenderStats {
			int objects_culled = 0;
			int objects_rendered = 0;
			int shadow_objects_culled = 0;
			int shadow_objects_rendered = 0;
		} stats;

#ifdef PN_PLATFORM_ANDROID
		SHADOW_TYPES shadow_type = SHADOW_TYPES::HARD;
#else
		SHADOW_TYPES shadow_type = SHADOW_TYPES::SOFT;
#endif
		bool gamma_correction = true;
		float gamma_value = 2.2f;
		glm::vec3 AMBIENT_LIGHT = glm::vec3(0.1f);
		bool world_light = true;
		float fov = 60.0f;
		
		int blur_quality = 4;	// number of blur passes. higher = blurrier, BUT SLOWER, REPRESENTS GAUSSIAN BLUR PASSES, SO MINIMALLY 2. too high won't help. sublinear growth.
		// using hdr, so range of [0,inf)
		float blur_strength = 0.f;

		// Master post-process toggle - disables all post-processing effects when false
		bool postprocess = true;

		bool bloom = true;
		float global_light_intensity = 1.5f;

		// Bloom settings - use macros to set platform-specific defaults
		#ifdef PN_PLATFORM_ANDROID
		// OPTIMIZED: Reduced bloom for mobile thermal performance
		int bloom_quality = 2;					// mobile default for lower thermal load
		float bloom_blur_strength = 0.8f;		// reduced from 1.0f for mobile
		float bloom_strength = 0.8f;			// reduced from 1.0f, compensated by threshold
		float bloom_threshold = 1.8f;			// slightly lower to catch more blooms with lower strength
		#else
		// Windows: bloom_quality=4 is 2x more expensive than Android's 2
		// Reduce to 3 for balanced performance/quality
		int bloom_quality = 3;					// number of blur passes for bloom (reduced from 4)
		float bloom_blur_strength = 1.f;		// generally [0.5,10] - higher = bloomier, BUT SLOWER
		float bloom_strength = 1.f;				// generally [0.0,5.0] - bloom strength is how visible the bloom is
		float bloom_threshold = 2.f;			// generally [0.8,1.5] - min brightness to bloom
		#endif

		TONE_MAPPING_TYPES tone_mapping_mode = TONE_MAPPING_TYPES::ACES;
		float tone_mapping_exposure = 1.f;

		// image based lighting
		bool ibl = true;
		float ibl_diffuse_strength = 1.0f;
		float ibl_specular_strength = 1.0f;
		float ibl_max_reflection_lod = 4.0f;
#ifdef PN_PLATFORM_ANDROID
		float ibl_roughness_bias = 0.10f;       // compensates for ASTC vs BC7 roughness compression artifacts
		float ibl_specular_mip_bias = 0.80f;    // reduced - was over-compensating for quantization artifacts
		float ibl_specular_strength_scale = 0.80f;  // slight reduction for GPU sampling differences
		float ibl_specular_prefilter_luma_clamp = 12.0f;
		float ibl_specular_firefly_clamp = 7.5f;
#else
		float ibl_roughness_bias = 0.0f;
		float ibl_specular_mip_bias = 0.0f;
		float ibl_specular_strength_scale = 1.0f;
		float ibl_specular_prefilter_luma_clamp = 0.0f;
		float ibl_specular_firefly_clamp = 32.0f;
#endif

		// Frame pacing and VSync settings
		// swap_interval: 0 = no VSync (lowest latency, possible tearing), 1 = VSync (smooth, ~1 frame latency)
		// For responsive controls, use 0; for smooth visuals without tearing, use 1
#ifdef PN_PLATFORM_ANDROID
		int android_swap_interval = 1; // 1 = vsync, 0 = uncapped.
		int android_target_fps = 0; // 0 = no explicit software cap.
		bool android_battery_saver_mode = false;
		int android_battery_saver_fps = 30;
#else
		// Windows: Default to 0 (no VSync) in Release for lowest latency
		// VSync enabled (1) in Debug for smooth editor experience
		#ifdef _DEBUG
		int swap_interval = 1;  // Debug: VSync enabled for smooth editor
		#else
		int swap_interval = 0;  // Release: No VSync for lowest latency, uncapped FPS
		#endif
#endif

		// ========================================
		// FRAME PACING SYSTEM (SIMPLIFIED)
		// Only tracks frame time statistics for debugging - NO frame skipping
		// Previous implementation caused input lag at high FPS due to
		// accumulated time tracking creating disconnect between input and rendering
		// ========================================
		struct FramePacingSettings {
			bool enabled = true;                     // Enable to track frame time stats
			float target_fps = 60.0f;                // Target frame rate (for reference)
			float max_accumulated_frames = 1.5f;     // DEPRECATED - not used
			bool enable_frame_skip = false;          // DEPRECATED - not used
			float spike_threshold_ms = 8.0f;         // Consider frame a "spike" if > this duration
			
			// Runtime state (don't modify in settings UI)
			float accumulated_time = 0.0f;            // DEPRECATED
			int frames_skipped = 0;                   // DEPRECATED
			int frames_rendered = 0;                  // DEPRECATED
			float last_frame_time_ms = 0.0f;
			float avg_frame_time_ms = 16.67f;        // Rolling average
			int spike_count = 0;                     // Count of frames exceeding spike_threshold
		} frame_pacing;

		// OPTIMIZED: Post-process at reduced resolution to reduce GPU load
		// Renders bloom/blur/tone-map at scale*full_resolution, then upscales to full
		// Default 1.0 (full resolution) for desktop, 0.75 for mobile
		// Use 0.5-0.75 for performance gains on lower-end hardware
#ifdef PN_PLATFORM_ANDROID
		float postprocess_resolution_scale = 0.75f;
#else
		float postprocess_resolution_scale = 1.0f;  // Windows: full resolution by default, can be lowered for performance
#endif

		// volumetric lighting (god rays / light shafts)
#ifdef PN_PLATFORM_ANDROID
		// OPTIMIZED: Reduced volumetric steps for mobile thermal performance
		// Previous: volumetric_steps = 10, temporal_blend = 0.85
		// Now: volumetric_steps = 6, temporal_blend = 0.90 (smoother at lower steps)
		bool volumetric = true;
		float volumetric_intensity = 0.5f;
		int   volumetric_steps = 6;           // reduced from 10 for mobile GPU
		float volumetric_max_dist = 40.0f;
		float volumetric_scatter = 0.f;
		float volumetric_resolution_scale = 0.4f; // already conservative
		int   volumetric_max_lights = 4;      // mobile preset cap
		float volumetric_temporal_blend = 0.90f; // increased from 0.85 for smoother at lower steps
		float volumetric_jitter_strength = 1.0f;
		float volumetric_history_clamp = 0.35f;
		int   volumetric_selection_hysteresis_frames = 3;
#else
		// Windows: Balanced performance - reduced from 24 steps (4x GPU cost) to 12
		// 24 steps is overkill for most scenes; 12 provides good quality with 2x performance
		bool volumetric = true;
		float volumetric_intensity = 0.5f;
		int   volumetric_steps = 12;          // reduced from 24 for better performance
		float volumetric_max_dist = 40.0f;
		float volumetric_scatter = 0.f;
		float volumetric_resolution_scale = 0.5f;
		int   volumetric_max_lights = 4;
		float volumetric_temporal_blend = 0.85f;
		float volumetric_jitter_strength = 1.0f;
		float volumetric_history_clamp = 0.35f;
		int   volumetric_selection_hysteresis_frames = 3;
#endif

		// animation
		bool interpolate_animation{ true };		// smoother animations at the expense of performance

		// debug settings
		bool DEBUG_USE_DIFFUSE_MAP{ true };
		bool DEBUG_USE_AO_MAP{ true };
		bool DEBUG_USE_NORMAL_MAP{ true };
		bool DEBUG_USE_ROUGHNESSMETALLIC_MAP{ true };
		bool DEBUG_USE_EMISSION_MAP{ true };
		bool DEBUG_DRAW_UI_HITBOXES{ false };

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
			IBL_DIFFUSE,
			IBL_SPECULAR,
			DIRECT_LIGHTING,
			NUM_PBR_MAP_TYPES,
		};

		std::vector<const char*> DEBUG_PBR_MAP_STRING{
			"NONE",
			"OBJECT_ONLY",
			"DIFFUSE",
			"AO",
			"NORMAL",
			"ROUGHNESS",
			"METALLIC",
			"EMISSION",
			"IBL_IRRADIANCE",
			"IBL_PREFILTER",
			"IBL_BRDFLUT",
			"IBL_DIFFUSE",
			"IBL_SPECULAR",
			"DIRECT_LIGHTING",
			"NUM_PBR_MAP_TYPES",
		};

		DEBUG_PBR_MAP_TYPES DEBUG_PBR_MAP_TYPE = DEBUG_PBR_MAP_TYPES::NONE;

		// optimisation
		bool use_instanced_rendering = true;

		// ========================================
		// THERMAL THROTTLE SYSTEM
		// Dynamic quality scaling based on GPU thermal state
		// ========================================
		struct ThermalThrottleState {
			bool enabled = false;
			int frame_count = 0;
			float gpu_time_avg = 0.0f;
			float threshold_ms = 4.0f;  // If frame > 4ms, reduce quality
			
			enum class QualityLevel : int {
				ULTRA = 0,   // Full quality (default)
				HIGH = 1,     // -1 bloom quality, reduced volumetric
				MEDIUM = 2,   // -2 bloom quality, minimal volumetric  
				LOW = 3,      // No bloom, disabled volumetric
			};
			QualityLevel currentLevel = QualityLevel::ULTRA;
			
			// Quality level presets
			struct QualityPreset {
				int bloom_quality_override = -1;  // -1 = use default
				int volumetric_steps_override = -1;
				float volumetric_resolution_scale_override = -1.0f;
				bool volumetric_enabled = true;
			};
			
			// Static presets that don't change at runtime
			static constexpr QualityPreset presets[4] = {
				{-1, -1, -1.0f, true},           // ULTRA: use defaults
				{1, 4, 0.3f, true},              // HIGH: reduced bloom, volumetric
				{1, 2, 0.25f, true},            // MEDIUM: minimal bloom and volumetric
				{0, 2, 0.2f, false}             // LOW: no bloom, minimal volumetric
			};
			
			// Runtime state for smooth transitions
			float transition_timer = 0.0f;
			static constexpr float MIN_STABLE_FRAMES_BEFORE_RECOVERY = 120.0f;  // 2 seconds at 60fps
			static constexpr float RECOVERY_THRESHOLD_MS = 3.0f;  // Must be below this for 2 seconds to recover
		} thermalThrottle;

		// minimap
		bool minimap_enabled = false;
		enum MINIMAP_SHAPE {
			MINIMAP_SHAPE_SQUARE = 0,
			MINIMAP_SHAPE_CIRCLE,
		};
		MINIMAP_SHAPE minimap_shape = MINIMAP_SHAPE_SQUARE;
		enum MINIMAP_RECOMMENDED_POSITION {
			TOP_LEFT = 0,
			TOP_RIGHT,
			BOTTOM_LEFT,
			BOTTOM_RIGHT,
			TOP_MIDDLE,
			BOTTOM_MIDDLE,
			LEFT_MIDDLE,
			RIGHT_MIDDLE,
		};
		float minimap_radius = 15.0f;
		glm::vec2 minimap_size_px = glm::vec2(200.0f, 200.0f);
		glm::vec2 minimap_pos_px = glm::vec2(20.0f, 20.0f);
		bool minimap_override_position = false;
		MINIMAP_RECOMMENDED_POSITION minimap_recommended_position = MINIMAP_RECOMMENDED_POSITION::BOTTOM_RIGHT;
		bool minimap_rotate_with_player = true;
		bool minimap_show_player = true;
		bool minimap_show_danger = true;
		bool minimap_show_items = true;
		bool minimap_show_objective = true;
		bool minimap_show_walls = true;
		bool minimap_show_route = true;
		enum MINIMAP_ROUTE_MODE {
			ROUTE_NEAREST_LINE = 0,
			ROUTE_BREADCRUMB_DOTS,
			ROUTE_EDGE_ARROW,
			ROUTE_LINE_AND_EDGE_ARROW,
		};
		MINIMAP_ROUTE_MODE minimap_route_mode = MINIMAP_ROUTE_MODE::ROUTE_NEAREST_LINE;
		bool minimap_use_icon_textures = false;
		float minimap_icon_scale = 1.0f;
		bool minimap_show_legend = false;
		Assets::GUID minimap_icon_player_guid;
		Assets::GUID minimap_icon_danger_guid;
		Assets::GUID minimap_icon_item_guid;
		Assets::GUID minimap_icon_objective_guid;
		Assets::GUID minimap_icon_wall_guid;
		float minimap_background_alpha = 0.5f;
		float minimap_border_thickness = 2.0f;
		glm::vec4 minimap_border_color = glm::vec4(1.0f);
		float minimap_camera_height = 30.0f;
		
		// OPTIMIZATION: Minimap update rate
		// Update minimap every N frames to reduce CPU load
		// 1 = every frame (default), 2 = every other frame, 3 = every 3rd frame
		// Higher values = better performance but less responsive minimap
		int minimap_update_interval = 1;
	};
}
