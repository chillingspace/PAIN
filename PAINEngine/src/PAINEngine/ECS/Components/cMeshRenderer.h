#pragma once

#include "pch.h"
// Somehow, if i dont include this, refl macro cannot be foundl, even tho is in pch...
#include "refl.hpp"
#include "LayeredSystems/LevelEditor/EditorAttributes.h"

namespace PAIN {

	/******************************************************************************************
	* Note: When creating components, try to stack them properly to properly optimise memory
	* (Place largest type var (Double) first, then followed by smallest.
	*****************************************************************************************/

    //Material instance
    struct MaterialInstance {

        //Read only
        std::shared_ptr<const Assets::Material> material_asset;

        //Texture paths
        uint32_t albedoTexture = 0;
        uint32_t normalTexture = 0;
        uint32_t metallicTexture = 0;
        uint32_t roughnessTexture = 0;
        uint32_t aoTexture = 0;
        uint32_t emissiveTexture = 0;

        //Color overrides
        glm::vec3 baseColorOverride{ -1.0f };
        float metallicOverride = -1.0f;
        float roughnessOverride = -1.0f;
        glm::vec3 emissiveOverride{ -1.0f };
    };

    struct ModelInstance {

        //Storage for selector
        std::vector<std::string> model_paths_storage;
        std::vector<std::string> materials_path_storage;

        //Read only
        std::shared_ptr<const Assets::Model> model_asset;

        //Materials list
        std::vector<MaterialInstance*> materials;

        int currentAnimationIndex = -1;  // Which animation is playing
        float animationTime = 0.0f;      // Current time in animation
        bool isPlaying = false;          // Is animation playing?
        bool loopAnimation = true;       // Loop animation?
        float playbackSpeed = 1.0f;      // Animation speed multiplier

        // Bone transforms (if animated)
        std::vector<glm::mat4> boneTransforms;

        // Morph target weights (if using blend shapes)
        std::vector<float> morphWeights;

        bool visible = true;
        bool castShadows = true;
        bool receiveShadows = true;
        uint32_t renderLayer = 0;

        int currentLOD = 0;  // Current LOD level

        bool IsAnimated() const {
            return !boneTransforms.empty();
        }

        bool HasMaterials() const {
            return !materials.empty();
        }

        MaterialInstance* GetMaterial(size_t index) const {
            if (index < materials.size()) {
                return materials[index];
            }
            return nullptr;
        }

        void SetVisible(bool isVisible) {
            visible = isVisible;
        }

        void PlayAnimation(int animIndex, bool loop = true) {
            if (animIndex >= 0 && animIndex < model_asset->animations.size()) {
                currentAnimationIndex = animIndex;
                animationTime = 0.0f;
                isPlaying = true;
                loopAnimation = loop;
            }
        }

        void StopAnimation() {
            isPlaying = false;
            animationTime = 0.0f;
        }

        void UpdateAnimation(float deltaTime) {
            if (!isPlaying || currentAnimationIndex < 0) return;

            animationTime += deltaTime * playbackSpeed;

            const auto& anim = model_asset->animations[currentAnimationIndex];
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
    };

	struct ModelRenderer {
		std::vector<std::string> model_paths_storage;
		Assets::GUID selected_model;
		ModelRenderer() = default;
		ModelRenderer(Assets::GUID const& id) : selected_model{ id } {}
	};
}


REFL_TYPE(PAIN::ModelRenderer)
REFL_FIELD(selected_model,
	PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Model),
	PAIN::Editor::Attributes::DisplayName("Model Asset"),
	PAIN::Editor::Attributes::Tooltip("Select a 3D model asset"))
REFL_END

