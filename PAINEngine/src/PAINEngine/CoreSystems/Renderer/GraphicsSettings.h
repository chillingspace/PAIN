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

		struct RenderStats {
			int objects_culled = 0;
			int objects_rendered = 0;
			int shadow_objects_culled = 0;
			int shadow_objects_rendered = 0;
		} stats;

		SHADOW_TYPES shadow_type = SHADOW_TYPES::SOFTEST;
		bool gamma_correction = true;
		glm::vec3 AMBIENT_LIGHT = glm::vec3(0.f);
		bool world_light = true;
		float fov = 60.0f;
		
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

		// volumetric lighting (god rays / light shafts)
		bool volumetric = true;
		float volumetric_intensity = 0.05f;   // overall brightness; start low
		int   volumetric_steps = 64;           // ray march steps; 32-128 typical
		float volumetric_max_dist = 50.0f;     // max ray length in world units
		float volumetric_scatter = 0.7f;       // Mie g: 0=uniform, 1=pure forward

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
			"NUM_PBR_MAP_TYPES",
		};

		DEBUG_PBR_MAP_TYPES DEBUG_PBR_MAP_TYPE = DEBUG_PBR_MAP_TYPES::NONE;

		// optimisation
		bool use_instanced_rendering = false;

		// minimap
		bool minimap_enabled = false;
		enum MINIMAP_RECOMMENDED_POSITION {
			TOP_LEFT = 0,
			TOP_RIGHT,
			BOTTOM_LEFT,
			BOTTOM_RIGHT,
			TOP_MIDDLE,
			BOTTOM_MIDDLE,
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
		std::string minimap_icon_player_path;
		std::string minimap_icon_danger_path;
		std::string minimap_icon_item_path;
		std::string minimap_icon_objective_path;
		std::string minimap_icon_wall_path;
		float minimap_background_alpha = 0.5f;
		float minimap_border_thickness = 2.0f;
		glm::vec4 minimap_border_color = glm::vec4(1.0f);
		float minimap_camera_height = 30.0f;
	};
}
