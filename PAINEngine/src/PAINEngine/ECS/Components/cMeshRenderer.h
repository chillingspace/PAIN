#pragma once

#include "pch.h"

#include "LayeredSystems/LevelEditor/EditorAttributes.h"
#include "AssetTypes.h"

namespace PAIN {

    //Material instance
    struct MaterialInstance {
        //material GUID
        Assets::GUID materialGUID;

        //Override with different textures
        Assets::GUID albedoTextureOverride;
        Assets::GUID normalTextureOverride;
        Assets::GUID metallicTextureOverride;
        Assets::GUID roughnessTextureOverride;
        Assets::GUID aoTextureOverride;
        Assets::GUID emissiveTextureOverride;
        Assets::GUID heightTextureOverride;
        Assets::GUID opacityTextureOverride;

        // Per-instance overrides
        glm::vec3 baseColorOverride{ 1.0f, 1.0f, 1.0f };
        float metallicOverride = 0.0f;
        float roughnessOverride = 0.5f;
        glm::vec3 emissiveOverride{ 1.0f, 0.0f, 1.0f };

		//bool useEmissiveOverride = false;

        bool useOverrides = false;
    };

    //Texture 2d instance
    struct Texture2D {
        //material GUID
        Assets::GUID texture_guid;
        glm::vec2 pos;
        glm::vec2 texture_scale{ 1 };
    };

    struct ModelRenderer {

        //Serialization flag
        static constexpr bool ShouldSerialize = true;

        // Asset reference
        Assets::GUID prevModelGUID;
        Assets::GUID modelGUID;

        // Per-instance materials (one per submesh)
        std::vector<MaterialInstance> materials;

        // Animation state
        int currentAnimationIndex = -1;
        float animationTime = 0.0f;
        bool isPlaying = false;
        bool loopAnimation = true;
        float playbackSpeed = 1.0f;
        std::vector<glm::mat4> boneTransforms;
        std::vector<float> morphWeights;

        // Rendering state
        bool visible = true;
        bool castShadows = true;
        bool receiveShadows = true;
        uint32_t renderLayer = 0;
        int currentLOD = 0;

        // Cached asset pointer (DO NOT SERIALIZE)
        mutable std::shared_ptr<const Assets::Model> cachedModelAsset;

        // ========================================
        // PERFORMANCE OPTIMIZATION: Buffer Offset Tracking
        // ========================================
        // Tracks where this model's geometry is in the shared VBO/EBO
        struct BufferOffset {
            unsigned int vertexOffset = 0;  // Offset in shared VBO
            unsigned int indexOffset = 0;   // Offset in shared EBO  
            unsigned int indexCount = 0;    // Number of indices
            bool isUploaded = false;
        };
        BufferOffset bufferOffset;

        // ========================================
        // PERFORMANCE OPTIMIZATION: Texture Cache
        // ========================================
        // Caches texture handles to avoid AssetManager lookups every frame
        struct SubmeshTextureCache {
            GLuint albedoTexture = 0;
            GLuint normalTexture = 0;
            GLuint metallicTexture = 0;
            GLuint roughnessTexture = 0;
            GLuint aoTexture = 0;
            GLuint emissiveTexture = 0;
            GLuint opacityTexture = 0;
            
            // Material properties (avoid lookups)
            glm::vec3 baseColor = glm::vec3(1.0f);
            float metallic = 0.0f;
            float roughness = 0.5f;
            
            bool cacheValid = false;
        };
        std::vector<SubmeshTextureCache> submeshCaches;

        // Constructors
        ModelRenderer() = default;
        explicit ModelRenderer(const Assets::GUID& guid) : modelGUID(guid) {}
        ~ModelRenderer() {
            //cleanup();
        }

        MaterialInstance* GetMaterial(size_t index) {
            return (index < materials.size()) ? &materials[index] : nullptr;
        }

    };
}

// ============================================
// REFLECTION (Editor Integration)
// ============================================
REFL_TYPE(PAIN::MaterialInstance)
REFL_FIELD(materialGUID, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Material))
REFL_FIELD(useOverrides, PAIN::Editor::Attributes::Tooltip("Enable to override material properties"))
REFL_FIELD(albedoTextureOverride, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(normalTextureOverride, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(metallicTextureOverride, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(roughnessTextureOverride, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(aoTextureOverride, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(emissiveTextureOverride, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(heightTextureOverride, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(opacityTextureOverride, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(baseColorOverride, PAIN::Editor::Attributes::DisplayName("Base Color"))
REFL_FIELD(metallicOverride, PAIN::Editor::Attributes::Range(0.0f, 1.0f), PAIN::Editor::Attributes::DisplayName("Metallic"))
REFL_FIELD(roughnessOverride, PAIN::Editor::Attributes::Range(0.0f, 1.0f), PAIN::Editor::Attributes::DisplayName("Roughness"))
REFL_FIELD(emissiveOverride, PAIN::Editor::Attributes::DisplayName("Emissive Color"))
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::MaterialInstance>);

REFL_TYPE(PAIN::ModelRenderer)
REFL_FIELD(modelGUID, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Model))
REFL_FIELD(materials)
REFL_FIELD(visible)
REFL_FIELD(castShadows)
REFL_FIELD(receiveShadows)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::ModelRenderer>);

REFL_TYPE(PAIN::Texture2D)
REFL_FIELD(texture_guid, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture))
REFL_FIELD(pos)
REFL_FIELD(texture_scale)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Texture2D>);
