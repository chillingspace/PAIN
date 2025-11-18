#pragma once

#include "pch.h"
// Somehow, if i dont include this, refl macro cannot be foundl, even tho is in pch...
#include "refl.hpp"
#include "LayeredSystems/LevelEditor/EditorAttributes.h"

namespace PAIN {

    //Material instance
    struct MaterialInstance {
        //material GUID
        Assets::GUID materialGUID;

        // GPU texture handles (uploaded once, reused)
        unsigned int albedoTexture = 0;
        unsigned int normalTexture = 0;
        unsigned int metallicTexture = 0;
        unsigned int roughnessTexture = 0;
        unsigned int aoTexture = 0;
        unsigned int emissiveTexture = 0;
        unsigned int heightTexture = 0;
        unsigned int opacityTexture = 0;

        // ADVANCED PBR TEXTURES (Optional)
        //unsigned int sheenTexture = 0;
        //unsigned int clearCoatTexture = 0;
        //unsigned int transmissionTexture = 0;

        //// LEGACY TEXTURES (For older formats)
        //unsigned int specularTexture = 0;
        //unsigned int glossinessTexture = 0; 
        //unsigned int ambientTexture = 0;

        //// SPECIAL TEXTURES
        //unsigned int lightmapTexture = 0;
        //unsigned int reflectionTexture = 0;
        //unsigned int displacementTexture = 0;

        // Per-instance overrides
        glm::vec3 baseColorOverride{ -1.0f };
        float metallicOverride = -1.0f;
        float roughnessOverride = -1.0f;
        glm::vec3 emissiveOverride{ -1.0f };

        bool useOverrides = false;
    };

    struct ModelRenderer {
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

        // GPU handles (uploaded once, stored here)
        uint32_t vaoHandle = 0;
        uint32_t vboHandle = 0;
        uint32_t iboHandle = 0;

        // Cached asset pointer (DO NOT SERIALIZE)
        mutable std::shared_ptr<const Assets::Model> cachedModelAsset;

        // Constructors
        ModelRenderer() = default;
        explicit ModelRenderer(const Assets::GUID& guid) : modelGUID(guid) {}
        ~ModelRenderer() {
            cleanup();
        }

        // Helper methods
        bool IsGPUReady() const { return vaoHandle != 0; }

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

        void UpdateAnimation(float deltaTime, const Assets::Model* modelAsset) {
            if (!isPlaying || currentAnimationIndex < 0 || !modelAsset) return;

            if (currentAnimationIndex >= modelAsset->animations.size()) return;

            const auto& anim = modelAsset->animations[currentAnimationIndex];
            animationTime += deltaTime * playbackSpeed;

            if (animationTime >= anim.duration) {
                if (loopAnimation) {
                    animationTime = fmod(animationTime, anim.duration);
                }
                else {
                    animationTime = anim.duration;
                    isPlaying = false;
                }
            }
        }

        void cleanup() {
            if (vaoHandle != 0) {
                glDeleteVertexArrays(1, &vaoHandle);
                vaoHandle = 0;
            }
            if (vboHandle != 0) {
                glDeleteBuffers(1, &vboHandle);
                vboHandle = 0;
            }
            if (iboHandle != 0) {
                glDeleteBuffers(1, &iboHandle);
                iboHandle = 0;
            }
        }
    };
}

// ============================================
// REFLECTION (Editor Integration)
// ============================================
REFL_TYPE(PAIN::MaterialInstance)
REFL_FIELD(materialGUID)
REFL_FIELD(baseColorOverride)
REFL_FIELD(metallicOverride)
REFL_FIELD(roughnessOverride)
REFL_FIELD(emissiveOverride)
REFL_FIELD(useOverrides)
REFL_END

REFL_TYPE(PAIN::ModelRenderer)
REFL_FIELD(modelGUID, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Model))
//REFL_FIELD(materials)
REFL_FIELD(visible)
REFL_FIELD(castShadows)
REFL_FIELD(receiveShadows)
REFL_END

