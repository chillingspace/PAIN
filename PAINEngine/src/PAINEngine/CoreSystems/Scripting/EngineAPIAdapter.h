#pragma once
#include "IEngineAPI.h"

#include <optional>
#include <string>
#include <string_view>

#include "PAINEngine/ECS/Controller.h"
#include "PAINEngine/ECS/Components/cTransform.h"
#include "PAINEngine/ECS/Components/cPhysics.h"
#include "PAINEngine/ECS/Components/cMetadata.h"
#include "PAINEngine/ECS/sMetaData.h"
#include "PAINEngine/CoreSystems/Path/Path.h"
#include "PAINEngine/CoreSystems/Events/Event.h"
#include "PAINEngine/CoreSystems/Assets/sAssets.h"
#include "Common/AssetTypes/src/AssetData.h"
#include "CoreSystems/Scene/Scene.h"
#include "CoreSystems/Windows/Window.h"

namespace PAIN {

    class Camera;

    class EngineAPIAdapter final : public IEngineAPI {
    public:
        EngineAPIAdapter(PAIN::ECS::Controller& ecs,
            PAIN::MetaData::Service& meta,
            PAIN::Assets::Manager* assets,
            //PAIN::Audio::Audio* audio,
            PAIN::Path::Path* fs,
            PAIN::Scene::SceneManager* scene,
            PAIN::Window::Window* window = nullptr)
            //PAIN::Serialization::Service* ser)
                        : ecs_(ecs), meta_(meta), assets_(assets), //audio_(audio), 
                            fs_(fs), scene_(scene), window_(window)
        {
            //PN_CORE_INFO("[LuaAdapter] ecs_ registry @ {}", (void*)&ecs_.getRegistry());
            //PN_CORE_INFO("[LuaAdapter] meta_         @ {}", (void*)&meta_);
        }

        /* =========================================================================== */
        /*                            Entities / Prefabs                               */
        /* =========================================================================== */
        entt::entity  CreateEntity(std::string layer = "", std::string name = "") override;
        void DeleteEntity(entt::entity entityId) override;
        entt::entity  CreatePrefabInstance(std::string prefab,
            std::string layer = "",
            std::string name = "") override;

        /* =========================================================================== */
        /*                                  Lookup                                     */
        /* =========================================================================== */
        std::optional<int> FindEntity(std::string_view name) override;
        std::string GetAssetGUID(std::string_view name, PAIN::Assets::Type want) override;

        std::string GetImageGUID(std::string_view name) override;
        std::string GetScriptGUID(std::string_view name) override;
        std::string GetAudioGUID(std::string_view name) override;
        std::string GetModelGUID(std::string_view name) override;
        std::string GetFontGUID(std::string_view name) override;
        std::string GetScenesGUID(std::string_view name) override;
        std::string GetPrefabsGUID(std::string_view name) override;
        std::string GetDataGUID(std::string_view name) override;
        std::string GetShaderGUID(std::string_view name) override;

        std::string GetEntityTexture(entt::entity entityId) override;
        void SetEntityTexture(entt::entity entity, const std::string& guidStr) override;

        glm::uvec2 GetFrameBufferSize() const override;

        /* =========================================================================== */
        /*                     Metadata (name / tags / groups)                         */
        /* =========================================================================== */
        void SetEntityName(entt::entity entityId, std::string name) override; // change this to entt::entity
        std::optional<std::string> GetEntityName(entt::entity entityId) override;
        void AddTag(entt::entity entityId, std::string tag) override;
        void RemoveTag(entt::entity entityId, std::string tag) override;
        bool HasTag(entt::entity entityId, std::string tag) override;
        /*void AssignGroup(entt::entity entityId, std::string group) override {}
        void UnassignGroup(entt::entity entityId) override {}
        std::optional<std::string> GetGroup(entt::entity entityId) override {}*/
        std::vector<entt::entity> GetEntitiesByTag(const std::string& tag) override;

        /* =========================================================================== */
        /*                                Transform                                    */
        /* =========================================================================== */
        glm::vec3 GetPosition(entt::entity entityId) override;
        void SetPosition(entt::entity entityId, glm::vec3 p) override;
        glm::vec2 Get2DPosition(entt::entity entityId) override;
        void Set2DPosition(entt::entity entityId, glm::vec2 p) override;

        glm::vec3 GetScale(entt::entity entityId) override;
        void SetScale(entt::entity entityId, glm::vec3 s) override;
        glm::vec3 GetRotation(entt::entity entityId) override;
        void SetRotation(entt::entity entityId, glm::vec3 r) override;

        /* =========================================================================== */
        /*                                  Physics                                    */
        /* =========================================================================== */
        glm::vec3 GetVelocity(entt::entity entityId) override;
        void SetVelocity(entt::entity entityId, glm::vec3 v) override;
        void AddForce(entt::entity e, glm::vec3 force) override;
        void AddImpulse(entt::entity e, glm::vec3 impulse) override;
		bool IsGrounded(entt::entity entityId, float maxDistance = .25f) override;
        std::tuple<bool, glm::vec3> GetWallNormal(entt::entity entityId, glm::vec3 direction, float checkDistance) override;
        void DisablePhysics(entt::entity e) override;
        void EnablePhysics(entt::entity e) override;

        /* =========================================================================== */
        /*                                   Audio                                     */
        /* =========================================================================== */
        void Audio_Play(entt::entity entityId) override;
        void Audio_Stop(entt::entity entityId) override;
        void Audio_SetVolumeDb(entt::entity entityId, float db) override;
        void Audio_SetGroup(entt::entity entityId, std::string group) override;
        void Audio_SetLooping(entt::entity entityId, bool looping) override;

        /* =========================================================================== */
        /*                           Scene / System state                              */
        /* =========================================================================== */
        bool  ChangeScene(std::string name) override;
        std::string GetCurrentSceneName() override;
        std::string GetPreviousSceneName() override;
        void  SetGamePaused(bool pause) override;
        bool  IsGamePaused() const override;

        float GetFps() const override;
        void  SetDeltaMultiplier(float m) override;
        float GetDeltaMultiplier() const override;
        void QuitApplication() override;

        /* =========================================================================== */
        /*                              Layer Control                                  */
        /* =========================================================================== */
        bool SetLayerEnabled(int layerId, bool enabled) override;
        bool GetLayerEnabled(int layerId) override;

        /* =========================================================================== */
        /*                              Graphics / FX                                  */
        /* =========================================================================== */
        void ShakeCamera(float duration, float amplitude) override;

        Camera* Camera_GetActive() override;
        void Camera_SetTransform(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up = { 0.f, 1.f, 0.f }) override;
        std::pair<glm::vec3, glm::vec3> GetCameraOffsets(entt::entity entityId) override;
        glm::vec3 Camera_ResolveCollision(const glm::vec3& proposedPos, const glm::vec3& currentPos) override;
        glm::vec3 Camera_GetPositionWithCollision(const glm::vec3& playerPos, const glm::vec3& desiredPos, const entt::entity& e) override;

        /* =========================================================================== */
        /*                                Particles                                    */
        /* =========================================================================== */
        void SpawnParticles(int entityId, int count, bool ignoreRotation) override;

        /* =========================================================================== */
        /*                              Input Helpers                                  */
        /* =========================================================================== */
        // --- per-frame input queries ---
        bool Input_IsKeyDown(int key) override;
        bool Input_WasKeyPressed(int key) override;
        bool Input_WasKeyReleased(int key) override;
        bool Input_IsMouseDown(int button) override;
        bool Input_WasMousePressed(int button) override;
        bool Input_WasMouseReleased(int button) override;
        glm::vec2 Input_GetMousePos() override;
        glm::vec2 Input_GetScrollDelta() override;
        bool Input_IsCursorInWindow() override;

        void Input_EndFrame() override; // call once per frame to clear edges
        void Input_OnEvent(PAIN::Event::Event& e) override; // feed events into the adapter

        void HideCursor(bool hidden) override;


        /* =========================================================================== */
        /*                                ModelRenderer                                 */
        /* =========================================================================== */
        std::optional<uint32_t> GetMeshId(entt::entity entityId) override;
        void SetMeshId(entt::entity entityId, uint32_t meshId) override;

        void SetUITexture(entt::entity entityId, const std::string& textureGuidStr) override;
        void SetUITextureScale(entt::entity e, glm::vec2 s) override;
        glm::vec2 GetUITextureScale(entt::entity e) override;
        void SetVisibility(entt::entity entityId, bool visible) override;

        /* =========================================================================== */
        /*                                  Lighting                                   */
        /* =========================================================================== */
        bool HasLight(entt::entity entityId) override;
        void AddLight(entt::entity entityId) override;
        void RemoveLight(entt::entity entityId) override;
        void SetLightPosition(entt::entity entityId, float x, float y, float z) override;
        void SetLightIntensity(entt::entity entityId, float r, float g, float b) override;
        void SetLightType(entt::entity entityId, int typeEnum /*0:POINT,1:DIRECTIONAL,2:SPOTLIGHT*/) override;
        void SetLightDirection(entt::entity entityId, float x, float y, float z) override;
        void SetShadowType(entt::entity entityId, int shadowEnum /*0:NONE,1:MAPPED,2:SCREEN_SPACE*/) override;

        /* =========================================================================== */
        /*                                  Animation                                  */
        /* =========================================================================== */
        void Animation_Play(entt::entity entityId, std::string animName) override;
        void Animation_CrossFade(entt::entity entityId, std::string animName, float duration) override;
        void Animation_SetSpeed(entt::entity entityId, float speed) override;
        void Animation_SetLoop(entt::entity entityId, bool loop) override;
        bool Animation_IsPlaying(entt::entity entityId, std::string animName) override;
        float GetAnimationTime(entt::entity entityId) override;
        float GetAnimationDuration(entt::entity entityId) override;
        bool Animation_HasFinished(entt::entity entityId) override;
        void SetAnimationTime(entt::entity entityId, float time) override;

    private:
        using EntityType = decltype(std::declval<PAIN::ECS::Controller&>().createEntity());
        inline static EntityType asEntity(int id) { return static_cast<EntityType>(id); }
        inline static int asInt(EntityType e) { return static_cast<int>(entt::to_integral(e)); }

    private:
        template <typename T>
        bool try_get(entt::entity entityId, T*& out) {
            if (auto opt = ecs_.getEntityComponent<T>(entityId)) { out = &opt->get(); return true; }
            return false;
        }

        template <typename T, typename... Args>
        T& ensure(entt::entity entityId, Args&&... args) {
            // create the component if missing, or return existing
            if (auto opt = ecs_.getEntityComponent<T>(entityId)) return opt->get();
            ecs_.addEntityComponent<T>(entityId, T{ std::forward<Args>(args)... });
            return ecs_.getEntityComponent<T>(entityId)->get();
        }

    private:
        PAIN::ECS::Controller& ecs_;
        PAIN::MetaData::Service& meta_;
        //PAIN::Audio::Audio* audio_ = nullptr;
        PAIN::Path::Path* fs_ = nullptr;
        PAIN::Scene::SceneManager* scene_ = nullptr;
        PAIN::Window::Window* window_ = nullptr;
        PAIN::Serialization::Service* ser_ = nullptr;
        PAIN::Assets::Manager* assets_ = nullptr;
        std::unordered_set<int> keysDown_, mouseDown_;
        std::unordered_set<int> keysPressed_, keysReleased_;
        std::unordered_set<int> mousePressed_, mouseReleased_;
        glm::vec2 mousePos_{ 0.0f }, scrollDelta_{ 0.0f };
        bool cursorIn_{ true };
    };

}