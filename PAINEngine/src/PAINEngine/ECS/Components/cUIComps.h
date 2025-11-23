/*****************************************************************//**
 * \file   cUIComps.h
 * \brief  All UI comps
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once
#ifndef C_UICOMPS_H
#define C_UICOMPS_H

#include "pch.h"
#include <refl.hpp>
#include "GLMSerialization.h"
#include "LayeredSystems/LevelEditor/Panels/ReflectionUI.h"
#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h"

namespace PAIN {

	struct UIRectTransform {
		glm::f32vec3 local_position{ 0, 0, 0 };
		glm::f32quat rotation;
		glm::f32vec3 scale{ 1, 1, 1 };

		// Anchor system (normalized 0-1 relative to parent)
		// Bottom-left anchor
		glm::f32vec2 anchor_min{ 0, 0 };
		// Top-right anchor
		glm::f32vec2 anchor_max{ 1, 1 };

		// Center of the entity
		glm::f32vec2 pivot{ 0.5f, 0.5f };

		// Position of pivot relative to anchors
		glm::f32vec2 anchored_position{ 0, 0 };
		// Width/height when anchors together
		glm::f32vec2 size_delta{ 100, 100 };

		// Left, Bottom padding
		glm::f32vec2 offset_min{ 0, 0 };
		// -Right, -Top padding
		glm::f32vec2 offset_max{ 0, 0 };

		glm::f32vec2 calculated_world_size{ 100, 100 };
		glm::f32vec2 calculated_world_position{ 0, 0 };
	};

	struct UIElement {
		// Is the UI element active/visible?
		bool b_is_enabled = true;         
		bool b_is_interactable = true;    
		// UI layering, higher values drawn on top
		int layer = 0;               
	};

	enum class UIButtonState {
		Normal,
		Highlighted,
		Pressed,
		Disabled
	};

	struct UIButton {
		// Current button state
		UIButtonState state = UIButtonState::Normal;    
		// Optional: color tint for each state (RGBA)
		int normal_color = 0xFFFFFFFF;                  
		int highlighted_color = 0xFFAAAAAA;
		int pressed_color = 0xFF888888;
		int disabled_color = 0xFF444444;
		// Name of Lua function to invoke on click
		std::string on_click_callback_lua;              
	};

	enum class CanvasRenderMode {
		ScreenSpaceOverlay,
		ScreenSpaceCamera,
		WorldSpace
	};

	struct UICanvas {
		CanvasRenderMode render_mode = CanvasRenderMode::ScreenSpaceOverlay;
		// Draw order among canvases, higher = front
		int sort_order = 0;     

		// For WorldSpace mode
		float world_scale = 0.001f;  // How big 1 UI unit is in world space
		bool b_face_camera = true;
	};

	enum class AnimationType { Position, Scale, Color, Rotation };

	struct UIAnimation {

		AnimationType anim_type = AnimationType::Position;
		float duration = 1.0f;
		float elapsed = 0.0f;
		bool b_playing = false;
		bool b_loop = false;

		// Start/end values (use based on animation type)
		glm::vec3 start_vec3{ 0 };
		glm::vec3 end_vec3{ 0 };
		glm::vec4 start_color{ 1 };
		glm::vec4 end_color{ 1 };
	};

}

#endif

// Enum Serializers 
NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::UIButtonState, {
	{PAIN::UIButtonState::Normal, "Normal"},
	{PAIN::UIButtonState::Highlighted, "Highlighted"},
	{PAIN::UIButtonState::Pressed, "Pressed"},
	{PAIN::UIButtonState::Disabled, "Disabled"}
	})

NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::CanvasRenderMode, {
	{PAIN::CanvasRenderMode::ScreenSpaceOverlay, "ScreenSpaceOverlay"},
	{PAIN::CanvasRenderMode::ScreenSpaceCamera, "ScreenSpaceCamera"},
	{PAIN::CanvasRenderMode::WorldSpace, "WorldSpace"}
	})

NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::AnimationType, {
{PAIN::AnimationType::Position, "Position"},
{PAIN::AnimationType::Scale, "Scale"},
{PAIN::AnimationType::Color, "Color"},
{PAIN::AnimationType::Rotation, "Rotation"}
	})

REFL_TYPE(PAIN::UIRectTransform)
REFL_FIELD(local_position)
REFL_FIELD(rotation)
REFL_FIELD(scale)
REFL_FIELD(anchor_min)
REFL_FIELD(anchor_max)
REFL_FIELD(pivot)
REFL_FIELD(anchored_position)
REFL_FIELD(size_delta)
REFL_FIELD(offset_min)
REFL_FIELD(offset_max)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UIRectTransform>);

REFL_TYPE(PAIN::UIElement)
REFL_FIELD(b_is_enabled)
REFL_FIELD(b_is_interactable)
REFL_FIELD(layer)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UIElement>);

REFL_TYPE(PAIN::UIButton)
REFL_FIELD(state)
REFL_FIELD(normal_color)
REFL_FIELD(highlighted_color)
REFL_FIELD(pressed_color)
REFL_FIELD(disabled_color)
REFL_FIELD(on_click_callback_lua)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UIButton>);

REFL_TYPE(PAIN::UICanvas)
REFL_FIELD(render_mode)
REFL_FIELD(sort_order)
REFL_FIELD(world_scale)
REFL_FIELD(b_face_camera)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UICanvas>);

REFL_TYPE(PAIN::UIAnimation)
REFL_FIELD(duration)
REFL_FIELD(b_playing)
REFL_FIELD(b_loop)
REFL_FIELD(start_vec3)
REFL_FIELD(end_vec3)
REFL_FIELD(start_color)
REFL_FIELD(end_color)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UIAnimation>);