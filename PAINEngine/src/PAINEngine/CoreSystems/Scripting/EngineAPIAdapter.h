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

class EngineAPIAdapter final : public IEngineAPI {
public:
    EngineAPIAdapter(PAIN::ECS::Controller& ecs,
        PAIN::MetaData::Service& meta, 
        PAIN::Assets::Manager* assets,
        //PAIN::Audio::Audio* audio,
        PAIN::Path::Path* fs)
        : ecs_(ecs), meta_(meta), assets_(assets), //audio_(audio), 
        fs_(fs) {
    }

    /* =========================================================================== */
    /*                            Entities / Prefabs                               */
    /* =========================================================================== */
    int  CreateEntity(std::string layer = "", std::string name = "") override;
    void DeleteEntity(int id) override;
    int  CreatePrefabInstance(std::string prefab,
        std::string layer = "",
        std::string name = "") override;

    /* =========================================================================== */
    /*                                  Lookup                                     */
    /* =========================================================================== */
    std::optional<int> FindEntity(std::string_view name) override;
    int GetImageID(std::string_view) override;
    int GetScriptID(std::string_view) override;
    //int GetAudioID(std::string_view) override;
    int GetModelID(std::string_view name) override;
    int GetFontID(std::string_view name) override;
    int GetScenesID(std::string_view name) override;
    int GetPrefabsID(std::string_view name) override;
    int GetDataID(std::string_view name) override;
    int GetShaderID(std::string_view name) override;

    /* =========================================================================== */
    /*                     Metadata (name / tags / groups)                         */
    /* =========================================================================== */
    void SetEntityName(int id, std::string name) override;
    std::optional<std::string> GetEntityName(int id) override;
    void AddTag(int id, std::string tag) override;
    void RemoveTag(int id, std::string tag) override;
    bool HasTag(int id, std::string tag) override;
    void AssignGroup(int id, std::string group) override;
    void UnassignGroup(int id) override;
    std::optional<std::string> GetGroup(int id) override;

    /* =========================================================================== */
    /*                                Transform                                    */
    /* =========================================================================== */
    glm::vec3 GetPosition(int id) override;
    void SetPosition(int id, glm::vec3 p) override;
    glm::vec3 GetScale(int id) override;
    void SetScale(int id, glm::vec3 s) override;

    /* =========================================================================== */
    /*                                  Physics                                    */
    /* =========================================================================== */
    glm::vec3 GetVelocity(int id) override;
    void SetVelocity(int id, glm::vec3 v) override;

    /* =========================================================================== */
    /*                                   Audio                                     */
    /* =========================================================================== */
    //// --- Audio: playback ---
    //int   Audio_Play(const std::string& vpath, float x, float y, float z, float volumeDb) override;
    //int   Audio_PlayRandomFrom(const std::string& playlist, float x, float y, float z, float volumeDb) override;

    //// --- Audio: channel control ---
    //bool  Audio_Stop(int channelId) override;
    //void  Audio_StopAll() override;
    //bool  Audio_Pause(int channelId) override;
    //bool  Audio_Resume(int channelId) override;
    //void  Audio_PauseAll() override;
    //void  Audio_ResumeAll() override;
    //bool  Audio_SetChannelVolumeDb(int channelId, float db) override;
    //bool  Audio_SetChannelPosition(int channelId, float x, float y, float z) override;

    //// --- Audio: listener & groups ---
    //void  Audio_SetListener(float px, float py, float pz, float vx, float vy, float vz,
    //                        float fx, float fy, float fz, float ux, float uy, float uz) override;
    //bool  Audio_SetGroupVolumeDb(const std::string& group, float db) override;
    //bool  Audio_FadeGroupToDb(const std::string& group, float targetDb, float seconds) override;
    //bool  Audio_SetMuteAll(bool mute) override;

    /* =========================================================================== */
    /*                           Scene / System state                              */
    /* =========================================================================== */
    void  ChangeScene(std::string name) override;
    void  PauseAllSystems(bool toPause) override;
    bool  IsGamePaused() const override;
    float GetFps() const override;
    void  SetDeltaMultiplier(float m) override;
    float GetDeltaMultiplier() const override;

    /* =========================================================================== */
    /*                              Graphics / FX                                  */
    /* =========================================================================== */
    void ShakeCamera(float duration, float amplitude) override;

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

    /* =========================================================================== */
    /*                                MeshRenderer                                 */
    /* =========================================================================== */
    std::optional<uint32_t> GetMeshId(int id) override;
    void SetMeshId(int id, uint32_t meshId) override;

    /* =========================================================================== */
    /*                                  Lighting                                   */
    /* =========================================================================== */
    bool HasLight(int id) override;
    void AddLight(int id) override;
    void RemoveLight(int id) override;
    void SetLightPosition(int id, float x, float y, float z) override;
    void SetLightIntensity(int id, float r, float g, float b) override;
    void SetLightType(int id, int typeEnum /*0:POINT,1:DIRECTIONAL,2:SPOTLIGHT*/) override;
    void SetLightForward(int id, float x, float y, float z) override;
    void SetShadowType(int id, int shadowEnum /*0:NONE,1:MAPPED,2:SCREEN_SPACE*/) override;

private:
    using EntityType = decltype(std::declval<PAIN::ECS::Controller&>().createEntity());
    inline static EntityType asEntity(int id) { return static_cast<EntityType>(id); }
    inline static int asInt(EntityType e) { return static_cast<int>(entt::to_integral(e)); }

private:
    template <typename T>
    bool try_get(int id, T*& out) {
        if (auto opt = ecs_.getEntityComponent<T>(asEntity(id))) { out = &opt->get(); return true; }
        return false;
    }

    template <typename T, typename... Args>
    T& ensure(int id, Args&&... args) {
        // create the component if missing, or return existing
        if (auto opt = ecs_.getEntityComponent<T>(asEntity(id))) return opt->get();
        ecs_.addEntityComponent<T>(asEntity(id), T{ std::forward<Args>(args)... });
        return ecs_.getEntityComponent<T>(asEntity(id))->get();
    }

    int guidToInt(const PAIN::Assets::GUID& id);
    int getIdIfType(std::string_view name, PAIN::Assets::Type want);

private:
    PAIN::ECS::Controller& ecs_;
    PAIN::MetaData::Service& meta_;
    //PAIN::Audio::Audio* audio_ = nullptr;
    PAIN::Path::Path* fs_ = nullptr;
    PAIN::Assets::Manager* assets_ = nullptr;
    std::unordered_set<int> keysDown_, mouseDown_;
    std::unordered_set<int> keysPressed_, keysReleased_;
    std::unordered_set<int> mousePressed_, mouseReleased_;
    glm::vec2 mousePos_{ 0.0f }, scrollDelta_{ 0.0f };
    bool cursorIn_{ true };

    // resolve "alias://..." -> real path once; accept raw real paths too
    std::string resolveMaybeVirtual(const std::string& p) const {
        if (!fs_) return p;
        if (p.find("://") != std::string::npos) return fs_->resolvePath(p);
        return p;
    }

};
