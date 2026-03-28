/**
 * @file TextureUnits.h
 * @brief Centralized texture unit allocation for the renderer.
 * 
 * This file defines all texture unit bindings used across the rendering pipeline.
 * Texture units are a limited resource (16-32 on mobile, 80+ on desktop), so careful
 * management is required to avoid exhaustion.
 * 
 * PASS-SPECIFIC USAGE:
 * - Geometry Pass: Material textures (units 0-5, can reuse since different pass)
 * - Lighting Pass: G-Buffer + Shadows + IBL (units 0-11)
 * - Volumetric Pass: Depth + History + Shadows (units 0-5)
 * - Post-Process: Source textures (units 0-1)
 * 
 * IMPORTANT: Units are pass-local. Different passes can reuse units since they
 * don't execute simultaneously.
 */

#pragma once

namespace PAIN {
namespace TextureUnits {

// ============================================================================
// GEOMETRY PASS - Material Textures
// ============================================================================
// These are bound during the geometry pass when rendering meshes.
// Using units 6-11 to avoid conflicts with common bindings.
// Note: These are separate from lighting pass textures (different passes).
namespace GeometryPass {
    constexpr int kAlbedo    = 6;
    constexpr int kAo        = 7;
    constexpr int kNormal    = 8;
    constexpr int kRoughness = 9;
    constexpr int kMetallic  = 10;
    constexpr int kEmission  = 11;
    constexpr int kCount     = 6;
}

// ============================================================================
// LIGHTING PASS - G-Buffer + Shadows + IBL
// ============================================================================
namespace LightingPass {
    // G-Buffer textures (read from geometry pass output)
    constexpr int kGPos       = 0;
    constexpr int kGCol       = 1;
    constexpr int kGNorm      = 2;
    constexpr int kGMaterial  = 3;
    constexpr int kGEmission  = 4;
    constexpr int kGBufferCount = 5;
    
    // Shadow maps (up to 4 shadow-mapped lights)
    constexpr int kShadowStart = kGBufferCount;  // 5
    constexpr int kMaxShadowMaps = 4;
    // Shadow units: 5, 6, 7, 8
    
    // IBL textures
    constexpr int kIrradiance = kShadowStart + kMaxShadowMaps;  // 9
    constexpr int kPrefilter  = kIrradiance + 1;  // 10
    constexpr int kBrdfLUT    = kPrefilter + 1;   // 11

    // SSAO occlusion texture
    constexpr int kSsao       = kBrdfLUT + 1;    // 12

    constexpr int kTotalUnits = kSsao + 1;  // 13 total units used in lighting pass
}

// ============================================================================
// VOLUMETRIC PASS - Depth + History + Shadows
// ============================================================================
namespace VolumetricPass {
    constexpr int kDepth   = 0;
    constexpr int kHistory = 1;
    
    // Shadow maps for volumetric lights (start at 2)
    constexpr int kShadowStart = 2;
    constexpr int kMaxShadowMaps = 4;
    
    constexpr int kTotalUnits = kShadowStart + kMaxShadowMaps;  // 6 total
}

// ============================================================================
// POST-PROCESS PASS
// ============================================================================
namespace PostProcessPass {
    constexpr int kSource = 0;
    constexpr int kBloom  = 1;  // For bloom blend
    
    constexpr int kTotalUnits = 2;
}

// ============================================================================
// 2D UI RENDERING
// ============================================================================
namespace UI2D {
    constexpr int kTexture = 6;  // Historical value, can be changed
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Get the maximum texture units needed across all passes.
 * Used for runtime validation against device limits.
 */
inline constexpr int GetMaxUnitsNeeded() {
    return LightingPass::kTotalUnits;  // 12 units - lighting pass is the most demanding
}

/**
 * @brief Check if the device has enough texture units for the renderer.
 * @param maxUnits Device's GL_MAX_TEXTURE_IMAGE_UNITS value
 * @return true if sufficient, false if renderer may fail
 */
inline bool HasSufficientTextureUnits(int maxUnits) {
    return maxUnits >= GetMaxUnitsNeeded();
}

/**
 * @brief Log texture unit allocation for debugging.
 */
inline void LogTextureUnitLayout() {
    // This would require including the logging header, so we'll use a macro
    // PN_CORE_INFO("Texture Unit Layout:");
    // PN_CORE_INFO("  Geometry Pass: 0-5 (materials)");
    // PN_CORE_INFO("  Lighting Pass: 0-4 (G-buffer), 5-8 (shadows), 9-11 (IBL)");
    // PN_CORE_INFO("  Volumetric Pass: 0-1 (depth/history), 2-5 (shadows)");
    // PN_CORE_INFO("  Post-Process: 0-1");
    // PN_CORE_INFO("  Max units needed: {}", GetMaxUnitsNeeded());
}

} // namespace TextureUnits
} // namespace PAIN
