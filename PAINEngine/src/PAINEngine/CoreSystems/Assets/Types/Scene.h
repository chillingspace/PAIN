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
            bool pickable = true;
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
                
                // Camera collision settings
                bool collisionEnabled = true;
                float collisionRadius = 0.5f;
                float collisionOffset = 0.1f;
                float capsuleHeight = 1.8f;
                bool useCapsuleCollision = false;
                bool showCollisionGizmo = false;
            } camera;

            //Floor settings
            struct FloorSettings {
                bool enabled = true;
                glm::vec3 position{ 0.0f, -1.0f, 0.0f };
                glm::vec3 halfExtents{ 100.0f, 1.0f, 100.0f };
            } floor;

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

            //Loading screen settings
            struct LoadingScreenSettings {
                // Background
                Assets::GUID backgroundTextureGUID;
                glm::vec3 backgroundColor{ 0.1f, 0.1f, 0.1f };
                float bgScale = 1.0f;
                bool showBackground = true;
                bool showOverlay = false;
                
                // Progress Bar
                glm::vec2 progressBarPosition{ 0.0f, 0.0f };
                glm::vec2 progressBarSize{ 600.0f, 40.0f };
                glm::vec3 fillColor{ 0.2f, 0.8f, 0.9f };
                glm::vec3 glowColor{ 0.4f, 0.9f, 1.0f };
                float glowIntensity = 0.5f;
                bool showProgressBar = false;
                
                // Status Text
                glm::vec2 statusTextPosition{ 0.0f, 0.0f };
                float statusTextScale = 0.03f;
                bool showStatusText = false;
                
                // Spritesheet Animation
                int frameCount = 1;
                int framesPerRow = 1;
                float frameTime = 0.1f;
                bool animationEnabled = false;
            } loadingScreen;

            //Layers
            std::vector<Layer> layers;
            std::vector<std::vector<bool>> mask_matrix;

            //Set of asset GUID to cache
            std::unordered_set<Assets::GUID> assets_to_cache;

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
