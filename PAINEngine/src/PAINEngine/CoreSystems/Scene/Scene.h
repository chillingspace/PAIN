#pragma once
#include "pch.h"

#include "CoreSystems/Renderer/Mesh.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Renderer/Light.h"
#include "Camera.h"

#include "CoreSystems/Assets/Types/Scene.h"

namespace PAIN {
	namespace Scene {
        class SceneManager : public AppSystem {
        private:

            //Curr active camera
            Camera* active_camera = nullptr;

            //Runtime objects
            std::unique_ptr<Camera> editor_camera;

            //Map of cameras
            std::unordered_map<std::string, std::unique_ptr<Camera>> game_cameras;
            std::string active_game_cam;

            //Current scene
            Assets::GUID curr_scene_id;

            //Light sources names
            std::string camera_light_name = "cam";
            std::string world_light_name = "world";

            //Curr Skybox texure id
            Assets::GUID curr_skybox_id;

            //Boolean for checking day time
            bool using_day_time = true;

            //Internal methods
            bool buildEntitiesFromAsset(SceneAsset const& scene_asset);
            void setupCamera(SceneAsset const& scene_asset);
            void setupEnvironment(SceneAsset const& scene_asset);
            nlohmann::json captureCurrentEntities();
            void captureSceneVariables(SceneAsset& scene_asset);
            nlohmann::json convertSceneToJSON(SceneAsset& scn_asset);
            bool saveSceneToPath(SceneAsset& scn_asset, std::filesystem::path const& relative);

            //Scene configuration
            void configScene(SceneAsset const& scn_asset);

            //Internal add object used for testing
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

#ifdef PN_PLATFORM_WINDOWS

            //Delete current active scene
            void deleteScene();

            //Create default scene
            void createScene(std::string const& name);

            //Save curr scene
            void saveActiveScene(Assets::GUID const& scn_id, std::string const& name = "");
#endif

            //Unload scene
            void unloadScene();

            //Accessor
            Assets::GUID getCurrSkyBoxTextureID() const { return curr_skybox_id; }
            void setCurrSkyBoxTexture(Assets::GUID const& skybox_id);
            Assets::GUID getCurrScnID() { return curr_scene_id; }

            // When you need to access the light
            Light* getCameraLight() {
                auto light = LightSources::get().get(camera_light_name);
                return light.has_value() ? &light->get() : nullptr;
            }

            Light* getWorldLight() {
                auto light = LightSources::get().get(world_light_name);
                return light.has_value() ? &light->get() : nullptr;
            }

            bool getUsingDayTime() const { return using_day_time; }
            void setUsingDayTime(bool day_time) { using_day_time = day_time; }

            Camera* GetActiveCamera();
            void SetActiveCamera(Camera* cam);
            void SetEditorCamera();
            void SetGameCamera();
            void ChangeGameCamera(std::string cam_name);
            const std::unordered_map<std::string, std::unique_ptr<Camera>>& GetAllGameCamera() const;
        };
	}

	//class Scene : public AppSystem {
	//public:
	//	Scene() = default;
	//	~Scene() = default;

	//	void onDetach() override;
	//	void onAttach() override;
	//	void onFixedUpdate(AppTiming timing) override {};
	//	void onUpdate(AppTiming timing) override;
 //       void onEvent([[maybe_unused]] Event::Event& e) override;

	//	entt::entity AddObject(const std::shared_ptr<Assets::Model>& mdl, const std::string& name, const glm::vec3& pos, const glm::quat& quat, const glm::vec3& scale, Assets::GUID const& diff_id = Assets::GUID{}, Assets::GUID const& ao_id = Assets::GUID{});

	//	Camera* GetActiveCamera();

	//private:
	//	std::unique_ptr<Camera> camera;

	//	// Audio Demo State Variables
	//	entt::entity m_audioSourceEntity = entt::null;

	//	// Path animation variables
	//	float m_demoTime = 0.0f;
	//	int m_currentPathSegment = 0;
	//	float m_segmentDuration = 4.0f;
	//	std::vector<glm::vec3> m_pathCorners;

	//	// Footstep variables
	//	float m_footstepTimer = 0.0f;
	//	const float m_footstepInterval = 0.4f;
	//};
}