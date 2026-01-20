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
#include "refl.hpp"
#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h"

namespace PAIN {

    // ═══════════════════════════════════════════════════════════════════════
    // UIRectTransform - Controls UI element position and size
    // ═══════════════════════════════════════════════════════════════════════
    // 
    // QUICK START - Bottom-left anchored button:
    //   anchor_min = (0, 0), anchor_max = (0, 0)
    //   anchored_position = (50, 50)  // 50 pixels from bottom-left
    //   size_delta = (200, 60)        // 200x60 pixel button
    //   pivot = (0.5, 0.5)            // Center pivot
    //
    // QUICK START - Center of screen:
    //   anchor_min = (0.5, 0.5), anchor_max = (0.5, 0.5)
    //   anchored_position = (0, 0)
    //   size_delta = (200, 60)
    //   pivot = (0.5, 0.5)
    //
    // QUICK START - Full screen stretch:
    //   anchor_min = (0, 0), anchor_max = (1, 1)
    //   offset_min = (0, 0), offset_max = (0, 0)  // No padding
    //   pivot = (0.5, 0.5)
    // ═══════════════════════════════════════════════════════════════════════
    struct UIRectTransform {
        // ─── Anchors (0-1 normalized space relative to parent) ───
        // (0,0) = bottom-left, (1,1) = top-right, (0.5,0.5) = center
        glm::vec2 anchor_min{ 0.5f, 0.5f };  // Default: center anchor
        glm::vec2 anchor_max{ 0.5f, 0.5f };  // If same as min = point anchor, if different = stretch
        
        // ─── Positioning ───
        glm::vec2 anchored_position{ 0, 0 };  // Offset in pixels from anchor point
        glm::vec2 size_delta{ 100, 100 };     // Width/height in pixels (when anchors are together)
        
        // ─── Pivot point (0-1 normalized within the rect itself) ───
        // (0,0) = bottom-left corner, (0.5,0.5) = center, (1,1) = top-right corner
        glm::vec2 pivot{ 0.5f, 0.5f };  // Default: center pivot
        
        // ─── Advanced: Stretch mode offsets (only used when anchor_min != anchor_max) ───
        // Use these to add padding when stretching between anchors
        glm::vec2 offset_min{ 0, 0 };  // Left/Bottom padding in pixels
        glm::vec2 offset_max{ 0, 0 };  // Right/Top padding in pixels (negative values)
        
        // ─── Transform (rarely needed for UI, usually leave at defaults) ───
        glm::vec3 local_position{ 0, 0, 0 };  // Extra offset (usually leave at 0)
        glm::quat rotation{ 1, 0, 0, 0 };     // Rotation (usually not used for UI)
        glm::vec3 scale{ 1, 1, 1 };           // Scale multiplier
        
        // ─── Calculated values (DO NOT SET - computed by layout system) ───
        glm::vec2 calculated_world_position{ 0, 0 };
        glm::vec2 calculated_world_size{ 100, 100 };
    };

    // ═══════════════════════════════════════════════════════════════════════
    // UIElement - Basic visibility and interaction flags
    // ═══════════════════════════════════════════════════════════════════════
    struct UIElement {
        bool b_is_enabled = true;       // Visible and rendered?
        bool b_is_interactable = true;  // Can receive mouse/touch input?
        int layer = 0;                  // Draw order (higher = front, rarely needed)
    };

    // ═══════════════════════════════════════════════════════════════════════
    // UIButton - Interactive button with state-based colors
    // ═══════════════════════════════════════════════════════════════════════
    // USAGE:
    //   1. Add UIButton + UIElement + UIRectTransform + Texture2D
    //   2. Set on_click_callback_lua to your Lua function name (e.g., "OnPlayButtonClick")
    //   3. Optionally customize colors for each state
    // ═══════════════════════════════════════════════════════════════════════
    enum class UIButtonState {
        Normal,       // Default state
        Highlighted,  // Mouse hovering
        Pressed,      // Mouse down
        Disabled      // Not interactable
    };

    struct UIButton {
        UIButtonState state = UIButtonState::Normal;
        
        // State colors (ARGB format: 0xAARRGGBB)
        int normal_color = 0xFFFFFFFF;       // White
        int highlighted_color = 0xFFCCCCCC;  // Light gray
        int pressed_color = 0xFFAAAAAA;      // Darker gray
        int disabled_color = 0xFF666666;     // Very dark gray
        
        // Lua callback - function name to call on click (e.g., "OnButtonClick")
        std::string on_click_callback_lua;
    };

    // ═══════════════════════════════════════════════════════════════════════
    // UICanvas - Root container for UI hierarchies (SCREEN SPACE ONLY)
    // ═══════════════════════════════════════════════════════════════════════
    // USAGE:
    //   - Add ONE canvas per UI hierarchy as the root
    //   - All UI elements must be children of a canvas
    //   - Canvas automatically fills the screen
    //   - Use sort_order if you have multiple canvases (e.g., HUD=0, Menus=10)
    // ═══════════════════════════════════════════════════════════════════════
    struct UICanvas {
        int sort_order = 0;  // Draw order among canvases (higher = drawn on top)
        // Note: Canvas is always screen-space and fills the viewport
    };

    // ═══════════════════════════════════════════════════════════════════════
    // UIText - Text rendering component
    // ═══════════════════════════════════════════════════════════════════════
    // USAGE:
    //   1. Add UIText + UIElement + UIRectTransform
    //   2. Set display_text to your text content
    //   3. Set font_guid to your font asset
    //   4. Adjust font_size and color as needed
    // ═══════════════════════════════════════════════════════════════════════
    enum class TextAlignment { Left, Center, Right };

    struct UIText {
        std::string display_text;             // The text to display
        Assets::GUID font_guid;               // Font asset to use
        
        // ─── Appearance ───
        glm::vec3 color{ 1, 1, 1 };          // Text color (RGB, white by default)
        float font_size = 24.0f;              // Font size in pixels
        TextAlignment alignment = TextAlignment::Left;
        
        // ─── Layout ───
        float wrap_width = 0.0f;              // 0 = no wrapping, >0 = wrap at this pixel width
        bool word_wrap = true;                // Wrap at word boundaries?
        float line_height = 1.2f;             // Line spacing multiplier
        
        // ─── Effects (optional) ───
        float outline_thickness = 0.0f;       // 0 = no outline
        glm::vec4 outline_color{ 0,0,0,1 };   // Outline color (RGBA)
        glm::vec2 shadow_offset{ 0, 0 };      // Shadow offset in pixels (0,0 = no shadow)
        glm::vec4 shadow_color{ 0,0,0,0.5f }; // Shadow color (RGBA)
        
        // ─── Advanced ───
        int max_length = 0;                   // 0 = unlimited, >0 = truncate text
        
        // ─── Internal (do not set manually) ───
        glm::vec2 text_pos{ 0, 0 };          // Calculated by renderer
        float scale_factor = 1.0f;            // Calculated by layout system
    };

    // ═══════════════════════════════════════════════════════════════════════
    // UIFollowsWorldEntity - Makes UI element follow a 3D entity on screen
    // ═══════════════════════════════════════════════════════════════════════
    // USAGE:
    //   1. Add UIFollowsWorldEntity + UIText + UIElement + UIRectTransform
    //   2. Set entity_target_guid to the 3D entity GUID to follow
    //   3. Optionally set world_offset to position above/below entity
    //   4. Use for floating labels, health bars, nametags, etc.
    // ═══════════════════════════════════════════════════════════════════════
    struct UIFollowsWorldEntity {
        Assets::GUID entity_target_guid;     // The 3D entity to follow
        glm::vec3 world_offset{ 0, 2, 0 };   // Offset in world space (e.g., 2 units above entity)
    };

    // ═══════════════════════════════════════════════════════════════════════
    // UIAnimation - Simple UI animations (EXPERIMENTAL - may change)
    // ═══════════════════════════════════════════════════════════════════════
    enum class AnimationType { Position, Scale, Color, Rotation };

    struct UIAnimation {
        AnimationType anim_type = AnimationType::Position;
        float duration = 1.0f;
        bool b_loop = false;
        
        // Start/end values (use based on animation type)
        glm::vec2 start_vec2{ 0 };
        glm::vec2 end_vec2{ 0 };
        
        // Runtime state (do not set)
        float elapsed = 0.0f;
        bool b_playing = false;
    };

} // namespace PAIN


#endif

// Enum Serializers 
NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::UIButtonState, {
	{PAIN::UIButtonState::Normal, "Normal"},
	{PAIN::UIButtonState::Highlighted, "Highlighted"},
	{PAIN::UIButtonState::Pressed, "Pressed"},
	{PAIN::UIButtonState::Disabled, "Disabled"}
	})

NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::AnimationType, {
{PAIN::AnimationType::Position, "Position"},
{PAIN::AnimationType::Scale, "Scale"},
{PAIN::AnimationType::Color, "Color"},
{PAIN::AnimationType::Rotation, "Rotation"}
	})

NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::TextAlignment, {
{PAIN::TextAlignment::Left, "Left"},
{PAIN::TextAlignment::Center, "Center"},
{PAIN::TextAlignment::Right, "Right"}})

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
REFL_FIELD(sort_order)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UICanvas>);

REFL_TYPE(PAIN::UIAnimation)
REFL_FIELD(duration)
REFL_FIELD(b_playing)
REFL_FIELD(b_loop)
REFL_FIELD(start_vec2)
REFL_FIELD(end_vec2)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UIAnimation>);

REFL_TYPE(PAIN::UIText)
REFL_FIELD(text_pos)
REFL_FIELD(display_text)
REFL_FIELD(color)
REFL_FIELD(font_guid, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Font))
REFL_FIELD(font_size)
REFL_FIELD(alignment)
REFL_FIELD(word_wrap)
REFL_FIELD(line_height)
REFL_FIELD(outline_thickness)
REFL_FIELD(outline_color)
REFL_FIELD(shadow_offset)
REFL_FIELD(shadow_color)
REFL_FIELD(max_length)
REFL_FIELD(wrap_width)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UIText>);

REFL_TYPE(PAIN::UIFollowsWorldEntity)
REFL_FIELD(entity_target_guid)
REFL_FIELD(world_offset)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::UIFollowsWorldEntity>);