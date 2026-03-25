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

        struct CameraBookmark {
            glm::vec3 pos = {};
            glm::vec3 forward = {};
            glm::vec3 up = {};
            bool occupied = false;
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

            // Camera bookmarks (editor only)
            std::array<CameraBookmark, 5> cameraBookmarks = {};

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
                // World light shadow settings
                glm::vec3 worldLightDirection{ -0.5f, -0.8f, -0.2f };  // Direction of sunlight (normalized on load)
                glm::vec3 worldLightPosition{ 0.0f, 30.0f, 0.0f };      // Shadow camera position (follows player in game)
                float worldLightShadowFollowDistance{ 22.5f };          // Distance shadow camera follows player
                int worldLightShadowResolution{ 1024 };                  // Shadow map resolution
                float worldLightFarPlane{ 30.0f };                       // Shadow rendering distance
                bool worldLightShadowsEnabled{ true };                   // Enable/disable world light shadows
                bool useIBL = true;
                float iblDiffuseStrength = 1.0f;
                float iblSpecularStrength = 1.0f;
                float iblMaxReflectionLod = 4.0f;
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

            // Minimap settings
            struct MinimapSettings {
                bool enabled = false;
                float radius = 15.0f;
                glm::vec2 size_px{ 200.0f, 200.0f };
                glm::vec2 pos_px{ 20.0f, 20.0f };
                bool override_position = false;
                GraphicsSettings::MINIMAP_RECOMMENDED_POSITION recommended_position =
                    GraphicsSettings::MINIMAP_RECOMMENDED_POSITION::BOTTOM_RIGHT;
                GraphicsSettings::MINIMAP_SHAPE shape = GraphicsSettings::MINIMAP_SHAPE_SQUARE;
                bool rotate_with_player = true;
                bool show_player = true;
                bool show_danger = true;
                bool show_items = true;
                bool show_objective = true;
                bool show_walls = true;
                bool show_route = true;
                GraphicsSettings::MINIMAP_ROUTE_MODE route_mode =
                    GraphicsSettings::MINIMAP_ROUTE_MODE::ROUTE_NEAREST_LINE;
                bool use_icon_textures = false;
                float icon_scale = 1.0f;
                bool show_legend = false;
                Assets::GUID icon_player_guid;
                Assets::GUID icon_danger_guid;
                Assets::GUID icon_item_guid;
                Assets::GUID icon_objective_guid;
                Assets::GUID icon_wall_guid;
                float background_alpha = 0.5f;
                float border_thickness = 2.0f;
                glm::vec4 border_color{ 1.0f, 1.0f, 1.0f, 1.0f };
                float camera_height = 30.0f;
            } minimap;

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
