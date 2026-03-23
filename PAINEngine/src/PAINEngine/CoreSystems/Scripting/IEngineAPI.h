#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include "Common/AssetTypes/src/AssetData.h"
#include <entt/entity/entity.hpp>

namespace PAIN {
    namespace Serialization { class Service; }

    namespace Event { class Event; } 
    class Camera;


    /**
     * @brief Interface exposing high-level engine operations to scripting systems
     *
     * This allows LuaManager to interact with the engine
     * without directly depending on ECS, AssetManager, or System internals.
     */
    struct IEngineAPI {
        virtual ~IEngineAPI() = default;

        /* =========================================================================== */
        /*                            Entities / Prefabs                               */
        /* =========================================================================== */
        virtual entt::entity  CreateEntity(std::string layer = "", std::string name = "") = 0;
        virtual void DeleteEntity(entt::entity entityId) = 0;
        virtual entt::entity  CreatePrefabInstance(std::string prefab,
            std::string layer = "",
            std::string name = "") = 0;

        /* =========================================================================== */
        /*                                  Lookup                                     */
        /* =========================================================================== */
        virtual std::optional<int> FindEntity(std::string_view name) = 0;

        virtual std::string GetAssetGUID(std::string_view name, PAIN::Assets::Type want) = 0;

        virtual std::string GetImageGUID(std::string_view name) = 0;
        virtual std::string GetScriptGUID(std::string_view name) = 0;
        virtual std::string GetAudioGUID(std::string_view name) = 0;
        virtual std::string GetModelGUID(std::string_view name) = 0;
        virtual std::string GetFontGUID(std::string_view name) = 0;
        virtual std::string GetScenesGUID(std::string_view name) = 0;
        virtual std::string GetPrefabsGUID(std::string_view name) = 0;
        virtual std::string GetDataGUID(std::string_view name) = 0;
        virtual std::string GetShaderGUID(std::string_view name) = 0;

        virtual std::string GetEntityTexture(entt::entity entityId) = 0;
        virtual void SetEntityTexture(entt::entity entity, const std::string& guidStr) = 0;

        virtual glm::uvec2 GetFrameBufferSize() const = 0;

        /* =========================================================================== */
        /*                     Metadata (name / tags / groups)                         */
        /* =========================================================================== */
        virtual void SetEntityName(entt::entity entityId, std::string name) = 0;
        virtual std::optional<std::string> GetEntityName(entt::entity entityId) = 0;
        virtual void AddTag(entt::entity entityId, std::string tag) = 0;
        virtual void RemoveTag(entt::entity entityId, std::string tag) = 0;
        virtual bool HasTag(entt::entity entityId, std::string tag) = 0;
        /*virtual void AssignGroup(entt::entity entityId, std::string group) = 0;
        virtual void UnassignGroup(entt::entity entityId) = 0;
        virtual std::optional<std::string> GetGroup(entt::entity entityId) = 0;*/
        virtual std::vector<entt::entity> GetEntitiesByTag(const std::string& tag) = 0;
		virtual void SetVisibility(entt::entity entityId, bool visible) = 0;

        /* =========================================================================== */
        /*                                Transform                                    */
        /* =========================================================================== */
        virtual glm::vec3 GetPosition(entt::entity entityId) = 0;
        virtual glm::vec3 GetWorldPosition(entt::entity entityId) = 0;
        virtual void SetPosition(entt::entity entityId, glm::vec3 p) = 0;
        virtual glm::vec2 Get2DPosition(entt::entity entityId) = 0;
        virtual void Set2DPosition(entt::entity entityId, glm::vec2 p) = 0;
        virtual glm::vec3 GetScale(entt::entity entityId) = 0;
        virtual void SetScale(entt::entity entityId, glm::vec3 s) = 0;
        virtual glm::vec3 GetRotation(entt::entity entityId) = 0;
        virtual void SetRotation(entt::entity entityId, glm::vec3 r) = 0;
        virtual glm::vec3 GetWorldForward(entt::entity entityId) = 0;

        /* =========================================================================== */
        /*                                  Physics                                    */
        /* =========================================================================== */
        virtual glm::vec3 GetVelocity(entt::entity entityId) = 0;
        virtual void SetVelocity(entt::entity entityId, glm::vec3 v) = 0;
        virtual void AddForce(entt::entity e, glm::vec3 force) = 0;
        virtual void AddImpulse(entt::entity e, glm::vec3 impulse) = 0;
		virtual bool IsGrounded(entt::entity entityId, float maxDistance) = 0;
        virtual std::tuple<bool, glm::vec3> GetWallNormal(entt::entity entityId, glm::vec3 direction, float checkDistance) = 0;
        virtual void DisablePhysics(entt::entity e) = 0;
        virtual void EnablePhysics(entt::entity e) = 0;

        /* =========================================================================== */
        /*                                   Audio                                     */
        /* =========================================================================== */
        virtual void Audio_Play(entt::entity entityId) = 0;
        virtual void Audio_Stop(entt::entity entityId) = 0;

        virtual void Audio_SetVolumeDb(entt::entity entityId, float db) = 0;
        virtual void Audio_SetGroup(entt::entity entityId, std::string group) = 0;
        virtual void Audio_SetLooping(entt::entity entityId, bool looping) = 0;

        /* =========================================================================== */
        /*                           Scene / System state                              */
        /* =========================================================================== */
        virtual bool  ChangeScene(std::string name) = 0;
        virtual std::string GetCurrentSceneName() = 0;
        virtual std::string GetPreviousSceneName() = 0;
        virtual void  SetGamePaused(bool pause) = 0;
        virtual bool  IsGamePaused() const = 0;
        virtual float GetFps() const = 0;
        virtual void  SetDeltaMultiplier(float m) = 0;
        virtual float GetDeltaMultiplier() const = 0;

        /* =========================================================================== */
        /*                              Layer Control                                  */
        /* =========================================================================== */
        virtual bool SetLayerEnabled(int layerId, bool enabled) = 0;
        virtual bool GetLayerEnabled(int layerId) = 0;

        /* =========================================================================== */
        /*                              Graphics / FX                                  */
        /* =========================================================================== */
        virtual void ShakeCamera(float duration, float amplitude) = 0;

        virtual Camera* Camera_GetActive() = 0;
        virtual void Camera_SetTransform(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up) = 0;
        virtual std::pair<glm::vec3, glm::vec3> GetCameraOffsets(entt::entity entityId) = 0;
        virtual glm::vec3 Camera_ResolveCollision(const glm::vec3& proposedPos, const glm::vec3& currentPos) = 0;
        virtual glm::vec3 Camera_GetPositionWithCollision(const glm::vec3& playerPos, const glm::vec3& desiredPos, const entt::entity& e) = 0;

        /* =========================================================================== */
        /*                                Particles                                    */
        /* =========================================================================== */
        virtual void SpawnParticles(int entityId, int count, bool ignoreRotation = false) = 0;

        /* =========================================================================== */
        /*                              Input Helpers                                  */
        /* =========================================================================== */
        // --- per-frame input queries ---
        virtual bool      Input_IsKeyDown(int key) = 0;
        virtual bool      Input_WasKeyPressed(int key) = 0;
        virtual bool      Input_WasKeyReleased(int key) = 0;
        virtual bool      Input_IsMouseDown(int button) = 0;
        virtual bool      Input_WasMousePressed(int button) = 0;
        virtual bool      Input_WasMouseReleased(int button) = 0;
        virtual glm::vec2 Input_GetMousePos() = 0;
        virtual glm::vec2 Input_GetScrollDelta() = 0;
        virtual bool      Input_IsCursorInWindow() = 0;
        virtual void      Input_EndFrame() = 0; // call once per frame to clear edges
        virtual void      Input_OnEvent(PAIN::Event::Event& e) = 0; // feed events into the adapter 
        virtual void      HideCursor(bool hidden) = 0;


        /* =========================================================================== */
        /*                                ModelRenderer                                 */
        /* =========================================================================== */
        virtual std::optional<uint32_t> GetMeshId(entt::entity entityId) = 0;
        virtual void SetModelByName(entt::entity entityId, const std::string& virtual_path) = 0;

        virtual void SetUITexture(entt::entity entityId, const std::string& textureGuidStr) = 0;
        virtual void SetUITextureScale(entt::entity e, glm::vec2 s) = 0;
        virtual glm::vec2 GetUITextureScale(entt::entity e) = 0;

        /* =========================================================================== */
        /*                                  Lighting                                   */
        /* =========================================================================== */
        virtual bool HasLight(entt::entity entityId) = 0;
        virtual void AddLight(entt::entity entityId) = 0;
        virtual void RemoveLight(entt::entity entityId) = 0;
        virtual void SetLightPosition(entt::entity entityId, float x, float y, float z) = 0;
        virtual glm::vec3 GetLightOffset(entt::entity entityId) = 0;
        virtual void SetLightIntensity(entt::entity entityId, float r, float g, float b) = 0;
        virtual void SetLightType(entt::entity entityId, int typeEnum /*0:POINT,1:DIRECTIONAL,2:SPOTLIGHT*/) = 0;
        virtual void SetLightDirection(entt::entity entityId, float x, float y, float z) = 0;
        virtual glm::vec3 GetLightDirection(entt::entity entityId) = 0;
        virtual float GetLightOuterAngle(entt::entity entityId) = 0;
        virtual void SetLightOuterAngle(entt::entity entityId, float degrees) = 0;
        virtual float GetLightInnerAngle(entt::entity entityId) = 0;
        virtual void SetLightInnerAngle(entt::entity entityId, float degrees) = 0;
        virtual void SetShadowType(entt::entity entityId, int shadowEnum /*0:NONE,1:MAPPED,2:SCREEN_SPACE*/) = 0;

        /* =========================================================================== */
        /*                                  Animation                                  */
        /* =========================================================================== */
        virtual void Animation_Play(entt::entity entityId, std::string animName) = 0;
        virtual void Animation_CrossFade(entt::entity entityId, std::string animName, float duration) = 0;
        virtual void Animation_SetSpeed(entt::entity entityId, float speed) = 0;
        virtual void Animation_SetLoop(entt::entity entityId, bool loop) = 0;
        virtual bool Animation_IsPlaying(entt::entity entityId, std::string animName) = 0;
        virtual float GetAnimationTime(entt::entity entityId) = 0;
        virtual float GetAnimationDuration(entt::entity entityId) = 0;
        virtual bool Animation_HasFinished(entt::entity entityId) = 0;
        virtual void SetAnimationTime(entt::entity entityId, float time) = 0;
        /* =========================================================================== */
        /*                              Application Control                              */
        /* =========================================================================== */
        // Request application quit / safe shutdown
        virtual void QuitApplication() = 0;

    };

}