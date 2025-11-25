#pragma once

#ifndef SCENE_ASSET_HPP
#define SCENE_ASSET_HPP

#include "AssetData.h"

namespace PAIN {
    namespace Scene {

        struct Scene : public Assets::IAsset {

            //Default camera settings
            struct Camera{


            } camera;

            //Scene Layers
            struct Layer {
                int id = 0;         
                int mask = 1;        
                bool enabled = true; // B_State (visibility)
                // @todo add entities later
            };
            std::vector<Layer> layers;
            std::vector<std::vector<bool>> mask_matrix;

            //Environment settings
            struct Environment {
                glm::vec3 ambientColor = glm::vec3(0.2f);
                float ambientIntensity = 1.0f;
                GUID skyboxGUID;
            } environment;

            Scene() = default;
            ~Scene();
        };
    }
}

#endif
