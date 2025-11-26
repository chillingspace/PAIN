#pragma once

#include "pch.h"

#include "LayeredSystems/LevelEditor/EditorAttributes.h"

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
        glm::vec3 emissiveOverride{ 0.0f, 0.0f, 0.0f };

        bool useOverrides = false;
    };

    //Texture 2d instance
    struct Texture2D {
        //material GUID
        Assets::GUID texture_guid;
        float texture_scale;
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

        // Constructors
        ModelRenderer() = default;
        explicit ModelRenderer(const Assets::GUID& guid) : modelGUID(guid) {}
        ~ModelRenderer() {
            //cleanup();
        }

        MaterialInstance* GetMaterial(size_t index) {
            return (index < materials.size()) ? &materials[index] : nullptr;
        }

        void PlayAnimation(int animIndex, bool loop = true, float speed = 1.0f) {
            currentAnimationIndex = animIndex;
            animationTime = 0.0f;
            isPlaying = true;
            loopAnimation = loop;
            playbackSpeed = speed;
        }

        void UpdateAnimation(float deltaTime) {
            if (!isPlaying || currentAnimationIndex < 0 || !cachedModelAsset) return;

            if (currentAnimationIndex >= cachedModelAsset->animations.size()) return;

            const auto& anim = cachedModelAsset->animations[currentAnimationIndex];
            animationTime += deltaTime * playbackSpeed;

            if (animationTime >= anim.duration) {
                if (loopAnimation) {
                    animationTime = fmod(animationTime, anim.duration);
                    // currentAnimationIndex++;             // don't change animation type, is a loop
                }
                else {
                    animationTime = anim.duration;
                    isPlaying = false;
                }
            }
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
REFL_FIELD(texture_scale)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Texture2D>);
