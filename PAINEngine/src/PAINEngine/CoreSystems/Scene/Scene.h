#pragma once

#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Renderer/Light.h"

#include "Camera.h"

#include "CoreSystems/Assets/Types/Scene.h"

#include "LoadingScreen.h"

namespace PAIN {
	namespace Scene {
        class SceneManager : public AppSystem {
        private:

            /* =========================================================================== */
            /*                                CAMERAS                                      */
            /* =========================================================================== */
            //Curr active camera
            Camera* active_camera = nullptr;

            //editor camera
            std::unique_ptr<Camera> editor_camera;

            // map of camera
            std::unordered_map<std::string, std::unique_ptr<Camera>> game_cameras;
            std::unordered_set<std::string> active_cameras;
            std::string active_game_cam;

            /* =========================================================================== */
            /*                                SCENES                                       */
            /* =========================================================================== */
            //Current scene id
            Assets::GUID curr_scene_id;
            Assets::GUID prev_scene_id;

            // Change scene variables
            bool pending_scene_change = false;
            Assets::GUID next_scene_guid;

            //Scene Snapshot for play/stop scene (DEBUG ONLY)
            SceneAsset scene_snapshot;
            Assets::GUID guid_snapshot;
            bool is_playing = false;
            bool is_game_paused = false;

            /* =========================================================================== */
            /*                            ENVIRONMENT                                      */
            /* =========================================================================== */
            //Light sources names
            std::string camera_light_name = "cam";
            std::string world_light_name = "world";

            //Curr Skybox texure id
            Assets::GUID curr_skybox_id;

            //Graphics settings
            GraphicsSettings& gs = GraphicsSettings::get();

            //Layers management
            std::vector<Layer> layers;
            std::vector<std::vector<bool>> mask_matrix;

            /* =========================================================================== */
            /*                                FLOOR                                        */
            /* =========================================================================== */
            bool floor_enabled = true;
            glm::vec3 floor_position{ 0.0f, -1.0f, 0.0f };
            glm::vec3 floor_extents{ 100.0f, 1.0f, 100.0f };

            // Internal methods
            // LOADING
            bool buildEntitiesFromAsset(SceneAsset const& scene_asset);
            void setupCamera(SceneAsset const& scene_asset);
            void setupEnvironment(SceneAsset const& scene_asset);
            void setupFloor(SceneAsset const& scene_asset);
            void setupLayers(SceneAsset const& scene_asset);
            void setupLoadingScreen(SceneAsset const& scene_asset);

            // SAVING
            void cacheSceneAssets(SceneAsset const& scene_asset);
            nlohmann::json captureCurrentEntities();
            void recursiveCapture(entt::entity entity, nlohmann::json& jsonArray);
            void captureCachedAssets(SceneAsset& scene_asset);
            void captureSceneVariables(SceneAsset& scene_asset);
            nlohmann::json convertSceneToJSON(SceneAsset& scn_asset);
#ifdef PN_PLATFORM_WINDOWS
            bool saveSceneToPath(SceneAsset& scn_asset, std::filesystem::path const& relative);
#endif

            //Scene configuration
            void configScene(SceneAsset const& scn_asset);

            // Internal add object used for testing
            entt::entity AddObject(const std::shared_ptr<Assets::Model>& mdl, const std::string& name, const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale, Assets::GUID const& diff_id = Assets::GUID{}, Assets::GUID const& ao_id = Assets::GUID{});

        public:
            SceneManager() = default;
            ~SceneManager() override = default;

            // AppSystem lifecycle
            void onAttach() override;
            void onDetach() override;
            void onUpdate(AppTiming timing) override;
            void onFixedUpdate(AppTiming timing) override {}
            void onEvent(Event::Event& e) override{}
            void onAppPause() override {}
            void onAppResume() override {}

            //Load scene through scene asset GUID
            void loadScene(const Assets::GUID& sceneGUID);

            //Load scene through relative path
            void loadScene(std::filesystem::path const& relative_path);

            //Change scene 
            void changeScene(const Assets::GUID& sceneGUID);

            void processPendingSceneChange();

#ifdef PN_PLATFORM_WINDOWS

            //Delete current active scene
            void deleteScene(Assets::GUID const& id);

#ifdef _DEBUG
            //Create default scene
            void createScene(std::string const& name);
#endif
            //Save curr scene
            void saveActiveScene(Assets::GUID const& scn_id, std::string const& name = "");
#endif
            //Unload scene
            void unloadScene();

            //Play / Stop scene
            bool isPlaying() const { return is_playing; }
            void onPlay();
            void onStop();
            void setPlaying(bool playing);
            bool isGamePaused() const { return is_game_paused;  }
            void setGamePaused(bool paused);

            //Accessor
            Assets::GUID getCurrSkyBoxTextureID() const { return curr_skybox_id; }
            void setCurrSkyBoxTexture(Assets::GUID const& skybox_id);
            Assets::GUID getCurrScnID() { return curr_scene_id; }
            Assets::GUID getPrevScnID() { return prev_scene_id; }

            // When you need to access the light
            Light* getCameraLight() {
                auto light = LightSources::get().get(camera_light_name);
                return light.has_value() ? &light->get() : nullptr;
            }

            Light* getWorldLight() {
                auto light = LightSources::get().get(world_light_name);
                return light.has_value() ? &light->get() : nullptr;
            }

            // Floor settings
            bool isFloorEnabled() const { return floor_enabled; }
            void setFloorEnabled(bool enabled) { floor_enabled = enabled; }
            glm::vec3 getFloorPosition() const { return floor_position; }
            void setFloorPosition(const glm::vec3& pos) { floor_position = pos; }
            glm::vec3 getFloorExtents() const { return floor_extents; }
            void setFloorExtents(const glm::vec3& extents) { floor_extents = extents; }

            //Loading screen
            std::unique_ptr<LoadingScreen> loadingScreen;

            // Cameras
            Camera* GetActiveCamera();
            Camera* GetGameCamera();
            void SetActiveCamera(Camera* cam);
            void SetEditorCamera();
            void SetGameCamera();
            void ChangeGameCamera(std::string cam_name);
            const std::string& GetActiveGameCamera();
            const std::unordered_map<std::string, std::unique_ptr<Camera>>& GetAllGameCamera() const;

            // Get current scene layers
            const std::vector<Scene::Layer>& getLayers() const {
                return layers;
            }

            // Get mutable reference for editor
            std::vector<Scene::Layer>& getLayers() {
                return layers;
            }

            bool isLayerEnabled(int layer_id) {
                for (const auto& layer : layers) {
                    if (layer.id == layer_id) {
                        return layer.enabled;
                    }
                }
                return true;
            }

            // Get mask matrix
            const std::vector<std::vector<bool>>& getMaskMatrix() const {
                return mask_matrix;
            }

            // Get mutable reference for editor
            std::vector<std::vector<bool>>& getMaskMatrix() {
                return mask_matrix;
            }

            // Check if two layers can interact
            bool canLayersInteract(int layer1, int layer2) const {
                if (layer1 < 0 || layer2 < 0) return true;  // Invalid = allow
                if (layer1 >= mask_matrix.size()) return true;
                if (layer2 >= mask_matrix[layer1].size()) return true;

                return mask_matrix[layer1][layer2];
            }

            // Get Picking mask
            int getPickingMask() const;
        };
	}
}