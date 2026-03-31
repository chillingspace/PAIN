# Technical Report: IBL Specular Reflection Differences Between Windows and Android (KTX Compressed Textures)

## Executive Summary

This report identifies the most likely causes of Image-Based Lighting (IBL) specular reflection differences between Windows (BC7 compressed) and Android (ASTC compressed) when using KTX textures. The findings are organized by severity and likelihood based on the existing PAIN engine codebase, Khronos specifications, and known GPU hardware behavior.

---

## 1. ASTC sRGB vs Linear Color Space for Data Textures (HIGH SEVERITY / VERY LIKELY)

### Problem
The `GL_COMPRESSED_SRGB8_ALPHA8_ASTC_*` format family causes the GPU to apply sRGB-to-linear conversion during texture sampling. When mistakenly applied to **linear data textures** (roughness, metallic, normal maps), this produces incorrect pixel values that directly corrupt PBR lighting calculations.

### Mechanism
- **sRGB format** (e.g., `GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR` ≈ 0x93D0): GPU applies `sRGB to linear = pow(value, 2.2)` during sampling
- **Linear format** (e.g., `GL_COMPRESSED_RGBA_ASTC_4x4_KHR` ≈ 0x93B0): GPU returns raw decoded bytes
- A roughness value of `0.5` stored linearly would become `pow(0.5, 2.2) ≈ 0.217` when misinterpreted as sRGB, dramatically changing specular response

### Codebase Analysis
The PAIN engine has already implemented a mitigation in `AssetLoader.cpp` (lines 85-130):
```cpp
bool IsLikelyLinearDataTexturePath(std::string const& virtual_path) { ... }
// Checks for: roughness, metalness, metallic, normal, specular, ao, occlusion
// BUT does NOT match: "ibl", "prefilter", "irradiance" → IBL textures stay as-is
```
And:
```cpp
GLenum ToLinearAstcFormat(GLenum format) {
    // Converts sRGB ASTC format enums to linear ASTC format enums
}
```

### Known Issues
- **ASTC sRGB conversion only applies to RGB channels, not Alpha** (KHR spec confirms this). So if roughness is packed in the alpha channel of an sRGB texture, it would NOT be double-corrected — but the RGB channels would still be wrong.
- **The `GL_EXT_texture_compression_astc_decode_mode` extension** (supported on many ARM Mali GPUs) lets engines select decode precision (`GL_RGBA8` vs `GL_RGBA16F`), but this is separate from the sRGB issue.

### Risk Factor
**CRITICAL** for data textures. This is the #1 cause of specular differences when sRGB formats are incorrectly used for roughness/metallic/normal maps in the ASTC pipeline.

---

## 2. OpenGL ES Mipmap Behavior Differences from Desktop OpenGL (MEDIUM-HIGH SEVERITY / LIKELY)

### Problem
OpenGL ES has different rules and constraints around mipmap generation, completeness, and LOD selection compared to desktop OpenGL. These can cause different mipmap levels to be sampled, directly affecting specular IBL which uses `textureLod(samplerCube, vec3, lod)`.

### Key Differences

#### 2.1 Automatic Mipmap Generation
- **Desktop OpenGL**: `glGenerateMipmap()` respects `GL_FRAMEBUFFER_SRGB` state — sRGB-to-linear conversion happens during generation for sRGB framebuffers
- **OpenGL ES 3.x**: Similar behavior via `EXT_sRGB_write_control`, but support varies by vendor

#### 2.2 Texture Completeness Rules
- **Non-power-of-two (NPOT)** textures: OpenGL ES 2.0 doesn't support NPOT with mipmaps. ES 3.0+ does, but some implementations are buggy
- **Minimum dimension = 1**: The KTX spec's mipmap chain must include all levels down to 1x1. Any missing level makes the cubemap "incomplete" on ES, causing sampling to return (0,0,0,1)

#### 2.3 Block-Compressed Mipmap Calculations
For ASTC, the number of blocks is calculated as:
```c
WIDTH_IN_BLOCKS = (WIDTH_IN_PIXELS + 3) >> 2;
HEIGHT_IN_BLOCKS = (HEIGHT_IN_PIXELS + 3) >> 2;
```
At small mipmap levels (e.g., 1x1 or 2x2), the padding within the final compressed block can cause subtle differences in how different GPU hardware interprets the block data, especially with ASTC's variable block sizes.

### Mipmap Dimension Truncation
The KTX spec notes that mip levels follow truncation by 2:
```
100x200 → 50x100 → 25x50 → 12x25 → 6x12 → 3x6 → 1x3 → 1x1
```
For cubemap face sizes < 4x4 ASTC blocks, there are known hardware decoding issues on some mobile GPUs.

---

## 3. textureLod() Implementation Differences in OpenGL ES (MEDIUM SEVERITY / LIKELY)

### Problem
`textureLod()` in fragment shaders behaves differently between desktop OpenGL and OpenGL ES, and even between GPU vendors on ES.

### Known Issues

#### 3.1 Precision and LOD Rounding
- `textureLod` takes an explicit floating-point LOD value
- Different GPU vendors round LOD differently internally
- On some ES implementations, `textureLod` may clamp or floor the LOD value differently from desktop OpenGL
- **Adreno GPUs**: Known to have slightly different LOD interpolation behavior for cubemaps vs desktop

#### 3.2 Cubemap Seam Handling
OpenGL ES implementations vary in how they handle cubemap face seams at specific LOD levels. This affects the edge transitions in prefiltered environment maps, which become visible as specular artifacts on high-gloss surfaces.

#### 3.3 GLSL Extension Requirements
- Desktop OpenGL 3.3+: `textureLod()` is always available in fragment shaders
- OpenGL ES 3.0+: `textureLod()` is part of core
- OpenGL ES 2.0: Requires `#extension GL_EXT_shader_texture_lod : enable`
  - The PAIN engine correctly uses `#extension GL_EXT_shader_texture_lod : require` in Android-specific shaders (confirmed in `android_prefilter.frag` and `android_pbr.frag`)

### Impact on Specular IBL
The prefiltered environment map's mipmap levels correspond to roughness levels. If the LOD computed from roughness (typically `lod = roughness * maxMipLevel`) maps to slightly different mipmap samples on Android vs Windows, the specular will look different even with identical prefilter data.

---

## 4. KTX Texture Loading Differences Between Platforms (MEDIUM SEVERITY / CONTEXT-DEPENDENT)

### Problem
The KTX container format carries format metadata that must be interpreted correctly by the loader. Different loading paths between Windows (DDS with BC7) and Android (KTX with ASTC) introduce divergence points.

### Codebase Analysis (AssetLoader.cpp)

#### 4.1 KTX1-Only Support
```cpp
if (kTexture->classId == ktxTexture1_c) {
    // Supported
} else {
    ktxTexture_Destroy(kTexture);
    throw std::runtime_error("Only KTX1 format is supported!");
}
```
KTX2 is not supported on Android, while Windows uses DDS. Format conversion happens at build time in `AssetCompiler.cpp`.

#### 4.2 Format Metadata Reading
The Android KTX loader reads format from the KTX1 header's `glInternalformat` field (line 267):
```cpp
tex->glTexFormat = ktx1->glInternalformat;
```
This is the format the original texture was compressed with on the host machine.

#### 4.3 Key Risk: KTX Color Space Metadata
KTX2 has an explicit color space field in the Data Format Descriptor (DFD). KTX1 relies on the OpenGL enum to communicate sRGB vs linear. If the `glInternalformat` in the KTX1 file is `GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR` but the data was actually authored in linear space, the hardware will apply an incorrect sRGB conversion.

#### 4.4 Face/Mip Size Calculation
```cpp
ktx_size_t faceMipSize = ktxTexture_GetImageSize(kTexture, mip);
```
The `ktxTexture_GetImageSize()` returns the size in bytes for the complete mip level. For cubemaps with 6 faces, the loader iterates face-by-face but uses the same size for each face. This works correctly for compressed textures where each face has the same byte count per mip level, but edge cases at the smallest mips can cause issues.

### Windows vs Android Format Mapping
| Purpose | Windows (DDS) | Android (KTX) |
|---------|---------------|---------------|
| Albedo | BC7 sRGB (DXGI 99) | ASTC sRGB (KTX reads `glInternalformat`) |
| Roughness/Metallic | BC7 linear | ASTC linear (via `IsLikelyLinearDataTexturePath`) |
| IBL Prefilter | BC6H float (DXGI 95/96) | ASTC or RGB16F uncompressed |
| IBL Irradiance | BC6H float | ASTC or RGB16F uncompressed |

**Critical note**: The KTX loader correctly leaves IBL textures (prefilter, irradiance) in their original format (not force-converting to linear), since IBL maps were excluded from `IsLikelyLinearDataTexturePath` (line 98).

---

## 5. GPU Hardware Sampling Differences for Compressed Textures (MEDIUM SEVERITY / VENDOR-SPECIFIC)

### ASTC Decode Mode Differences

#### 5.1 Decode Precision: FP16 vs UNORM8
The `GL_EXT_texture_compression_astc_decode_mode` extension allows selecting between:
- `GL_RGBA16F` — decode to FP16 (default, full precision)
- `GL_RGBA8` — decode to UNORM8 (reduced precision, better performance)

**Default is FP16**, and if the extension is not supported by the driver, you always get FP16. However, some vendors default to UNORM8 on certain GPU architectures for power savings.

If the prefiltered environment map was compressed assuming FP16 decode but the hardware uses UNORM8, the dynamic range loss would be noticeable in specular highlights.

#### 5.2 ASTC Encoder Quality Differences
ASTC compression quality varies dramatically based on the encoder used:
- **ARM astcenc** (reference): Best quality, configurable profile (thorough/exhaustive/etc.)
- **Different encoder settings**: The same source image compressed at different quality profiles will have different visual results
- If the Android KTX textures were compressed at a lower quality setting than the Windows BC7 textures, this alone could account for visible specular differences

### Mobile GPU-Specific Issues

#### Adreno GPUs (Qualcomm)
- BC7 transcoding from ASTC/BasisUniversal is NOT supported natively — driver emulates or transcodes in software
- Cubemap seam handling can differ between Adreno generations
- Some Adreno drivers have known bugs with `glGenerateMipmap()` on compressed cube textures

#### Mali GPUs (ARM)
- ASTC implementation is the most reliable since ARM created the format
- However, BCn formats are NOT supported on Mali — any BC7 content must be transcoded or emulated
- Mali's ASTC texture cache can behave differently at certain LOD ranges

#### PowerVR GPUs
- ASTC texture decoding can have edge effects at block boundaries, especially with non-square block sizes (e.g., 8x6 vs 6x8)
- Some PowerVR Series6XE implementations have known cubemap sampling bugs at mip level transitions

---

## 6. Additional Factors

### 6.1 HDR/Linear Color Pipeline
PBR specular IBL uses an HDR environment map (RGB16F or BC6H) which should be in linear space. If any sRGB conversion happens in the pipeline (at loading, at filtering, at the shader), the specular response curve changes non-linearly.

**The PBI pipeline in the engine:**
1. Environment HDR → Prefilter convolution (offline) → Prefilter cubemap (stored)
2. Prefilter cubemap + BRDF LUT → Final specular in `pbr.frag`
3. If the prefilter output was stored as ASTC-sRGB instead of ASTC-linear, the convolution results would be gamma-corrected, wrong.

### 6.2 Block Size Impact on Specular
ASTC 4x4 blocks = 8 bpp. ASTC 8x6 blocks = 2.67 bpp. Using a lower bitrate on the prefiltered environment map can cause:
- Blocky artifacts at high gloss (low roughness) levels
- Loss of fine specular detail in lower mips
- Different behavior on mobile vs desktop due to different compression formats (ASTC vs BC7)

---

## Summary: Most Likely Causes (Ranked)

| Rank | Cause | Likelihood | Impact |
|------|-------|-----------|--------|
| 1 | sRGB ASTC format accidentally used for roughness/metallic data textures | HIGH | Specular completely wrong |
| 2 | HDR prefiltered environment map stored with incorrect color space (sRGB vs linear) | HIGH | Specular intensity/color shifted |
| 3 | textureLod() LOD calculation yielding different mipmap levels across platforms | MEDIUM-HIGH | Roughness mapping mismatch |
| 4 | ASTC compression quality/bitrate differences vs BC7 (different visual fidelity) | MEDIUM | Specular detail/blockiness differences |
| 5 | ASTC decode mode (FP16 vs UNORM8) affecting HDR prefilter mip range | MEDIUM | Specular highlight intensity differences |
| 6 | Cubemap seam handling differences at specific LOD levels | LOW-MEDIUM | Visible seams at grazing angles |
| 7 | Smallest mipmap levels (≤4x4) having ASTC block padding artifacts | LOW | Rough = 1.0 specular incorrect |

---

## Recommended Actions for PAIN Engine

1. **Verify IBL texture format at runtime**: Print the actual `glInternalformat` from the KTX header for prefilter/irradiance cubemaps — confirm they are `RGB16F` or linear ASTC, NOT sRGB ASTC.

2. **Check roughness/metallic textures**: Confirm `IsLikelyLinearDataTexturePath` catches ALL data texture paths used in the specular IBL pipeline.

3. **Compare prefilter mips visually**: Dump the prefiltered environment map mips on both platforms and compare. If Windows mips look different from Android mips at the same roughness, the issue is in storage/loading, not shaders.

4. **Test textureLod behavior**: Use a debug shader that outputs the computed LOD value as color — verify both platforms select the same mips for the same roughness inputs.

5. **Verify ASTC compression settings**: Check `AssetCompiler.cpp` to confirm what quality profile and block size are used for ASTC on Android vs BC7 on Windows.

---

## References

1. Khronos KTX File Format Specification v2.0: https://registry.khronos.org/KTX/specs/2.0/ktxspec.v2.html
2. Khronos 3D-Formats-Guidelines (KTX Developer Guide): https://github.com/KhronosGroup/3D-Formats-Guidelines/blob/main/KTXDeveloperGuide.md
3. KHR_texture_compression_astc_hdr extension spec: https://registry.khronos.org/OpenGL/extensions/KHR/KHR_texture_compression_astc_hdr.txt
4. EXT_texture_compression_astc_decode_mode extension spec: https://registry.khronos.org/OpenGL/extensions/EXT/EXT_texture_compression_astc_decode_mode.txt
5. NVIDIA — Using ASTC Texture Compression for Game Assets: https://developer.nvidia.com/astc-texture-compression-for-game-assets
6. ARM ASTC Encoder: https://github.com/ARM-software/astc-encoder
7. OpenGL Wiki — ASTC Texture Compression: https://www.khronos.org/opengl/wiki/ASTC_Texture_Compression
8. LearnOpenGL — Specular IBL: https://learnopengl.com/PBR/IBL/Specular-IBL
