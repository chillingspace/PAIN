#pragma once

#ifndef SCENE_ASSET_HPP
#define SCENE_ASSET_HPP

#include "AssetData.h"

namespace PAIN {
    namespace Scene {
        struct SceneAsset : public Assets::IAsset {

            //Default camera settings
            struct CameraSettings {
                glm::vec3 position{ 0.f, 2.f, 4.f };
                glm::vec3 forward{ 0.f, 0.f, -1.f };
                glm::vec3 up{ 0.f, 1.f, 0.f };
                float fov = 60.0f;
                float nearPlane{ 0.1f };
                float farPlane{ 100.f };
                float aspectRatioW{ 16.f };
                float aspectRatioH{ 9.f };
            } camera;

            //Environment settings
            struct Environment {
                glm::vec3 ambientColor{ 0.2f, 0.2f, 0.2f };
                float ambientIntensity = 1.0f;
                Assets::GUID skyboxGUID;
                bool useDaytime = true;
                float globalLightIntensity = 1.0f;
            } environment;

            //Scene Layers
            struct Layer {
                int id = 0;
                int mask = 1;
                bool enabled = true;
                // @todo add entities later
            };
            std::vector<Layer> layers;
            std::vector<std::vector<bool>> mask_matrix;

            //Entity data
            nlohmann::json entityData;

            //Default constructor
            SceneAsset() {
                layers.push_back(Layer{ 0, 1, true });
            }
            ~SceneAsset() = default;
        };
    }
}

#endif
