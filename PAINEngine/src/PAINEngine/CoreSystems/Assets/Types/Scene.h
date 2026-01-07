#pragma once

#ifndef SCENE_ASSET_HPP
#define SCENE_ASSET_HPP

#include "AssetData.h"
#include "CoreSystems/Renderer/GraphicsSettings.h"
#include "CoreSystems/Renderer/Light.h"

namespace PAIN {
    namespace Scene {

        //Scene layering
        struct Layer {
            int id = 0;
            int mask = 1;
            bool enabled = true;
            std::string name = "Layer " + std::to_string(id);
            glm::vec3 color = glm::vec3(1.0f);
        };

        struct SceneAsset : public Assets::IAsset {

            //Default camera settings
            struct CameraSettings {
                std::string active_game_cam;
                glm::vec3 position{ 0.f, 2.f, 4.f };
                glm::vec3 forward{ 0.f, 0.f, -1.f };
                glm::vec3 up{ 0.f, 1.f, 0.f };
                glm::vec3 right{ 0.f, 1.f, 0.f };
                float fov = GraphicsSettings::get().fov;
                float nearPlane{ 0.1f };
                float farPlane{ 100.f };
                float aspectRatioW{ 16.f };
                float aspectRatioH{ 9.f };
                float speed{ 15.f };
                float sensitivity{ 0.1f };
            } camera;

            //Environment settings
            struct Environment {
                Assets::GUID skyboxGUID;
                glm::vec3 cameraLightIntensity{ 0.01f };
                glm::vec3 worldLightIntensity{ GraphicsSettings::get().global_light_intensity };
                bool useWorldLight = true;
                bool useIBL = true;
                bool useDiffuseMap = true;
                bool useAOMap = true ;
                bool useNormalMap = true;
                bool useRoughnessMetallicMap = true;
                bool useEmissionMap = true;
                GraphicsSettings::DEBUG_PBR_MAP_TYPES pbr_map = GraphicsSettings::DEBUG_PBR_MAP_TYPES::NONE;
            } environment;

            //Layers
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
