#pragma once

#ifndef SCENE_ASSET_HPP
#define SCENE_ASSET_HPP

#include "AssetData.h"

namespace PAIN {
    namespace Scene {

        struct Scene : public Assets::IAsset {

            //Sore entity registry
            entt::registry registry;

            //Environment settings
            struct Environment {
                glm::vec3 ambientColor = glm::vec3(0.2f);
                float ambientIntensity = 1.0f;

                //Reference to skybox
                GUID skyboxGUID;

                // Global fog
                bool fogEnabled = false;
                glm::vec3 fogColor = glm::vec3(0.5f);
                float fogDensity = 0.01f;
                float fogStart = 10.0f;
                float fogEnd = 100.0f;
            } environment;

            Scene() = default;
            ~Scene();
        };
    }
}

#endif
