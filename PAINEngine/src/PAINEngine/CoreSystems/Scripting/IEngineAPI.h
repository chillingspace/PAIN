#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace PAIN { namespace Event { class Event; } }

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
    virtual int  CreateEntity(std::string layer = "", std::string name = "") = 0;
    virtual void DeleteEntity(int id) = 0;
    virtual int  CreatePrefabInstance(std::string prefab,
        std::string layer = "",
        std::string name = "") = 0;

    /* =========================================================================== */
    /*                                  Lookup                                     */
    /* =========================================================================== */
    virtual std::optional<int> FindEntity(std::string_view name) = 0;
    virtual int GetImageID(std::string_view name) = 0;
    virtual int GetScriptID(std::string_view name) = 0;
    //virtual int GetAudioID(std::string_view name) = 0;
    virtual int GetModelID(std::string_view name) = 0;
    virtual int GetFontID(std::string_view name) = 0;
    virtual int GetScenesID(std::string_view name) = 0;
    virtual int GetPrefabsID(std::string_view name) = 0;
    virtual int GetDataID(std::string_view name) = 0;
    virtual int GetShaderID(std::string_view name) = 0;

    /* =========================================================================== */
    /*                     Metadata (name / tags / groups)                         */
    /* =========================================================================== */
    virtual void SetEntityName(int id, std::string name) = 0;
    virtual std::optional<std::string> GetEntityName(int id) = 0;
    virtual void AddTag(int id, std::string tag) = 0;
    virtual void RemoveTag(int id, std::string tag) = 0;
    virtual bool HasTag(int id, std::string tag) = 0;
    virtual void AssignGroup(int id, std::string group) = 0;
    virtual void UnassignGroup(int id) = 0;
    virtual std::optional<std::string> GetGroup(int id) = 0;

    /* =========================================================================== */
    /*                                Transform                                    */
    /* =========================================================================== */
    virtual glm::vec3 GetPosition(int id) = 0;
    virtual void SetPosition(int id, glm::vec3 p) = 0;
    virtual glm::vec3 GetScale(int id) = 0;
    virtual void SetScale(int id, glm::vec3 s) = 0;

    /* =========================================================================== */
    /*                                  Physics                                    */
    /* =========================================================================== */
    virtual glm::vec3 GetVelocity(int id) = 0;
    virtual void SetVelocity(int id, glm::vec3 v) = 0;

    /* =========================================================================== */
    /*                                   Audio                                     */
    /* =========================================================================== */
    //// --- Audio: playback ---
    //virtual int   Audio_Play(const std::string& vpath, float x, float y, float z, float volumeDb) = 0;
    //virtual int   Audio_PlayRandomFrom(const std::string& playlist, float x, float y, float z, float volumeDb) = 0;

    //// --- Audio: channel control ---
    //virtual bool  Audio_Stop(int channelId) = 0;
    //virtual void  Audio_StopAll() = 0;
    //virtual bool  Audio_Pause(int channelId) = 0;
    //virtual bool  Audio_Resume(int channelId) = 0;
    //virtual void  Audio_PauseAll() = 0;
    //virtual void  Audio_ResumeAll() = 0;
    //virtual bool  Audio_SetChannelVolumeDb(int channelId, float db) = 0;
    //virtual bool  Audio_SetChannelPosition(int channelId, float x, float y, float z) = 0;

    //// --- Audio: listener & groups ---
    //virtual void  Audio_SetListener(float px, float py, float pz, float vx, float vy, float vz, float fx, float fy, float fz, float ux, float uy, float uz) = 0;
    //virtual bool  Audio_SetGroupVolumeDb(const std::string& group, float db) = 0;
    //virtual bool  Audio_FadeGroupToDb(const std::string& group, float targetDb, float seconds) = 0;
    //virtual bool  Audio_SetMuteAll(bool mute) = 0;

    /* =========================================================================== */
    /*                           Scene / System state                              */
    /* =========================================================================== */
    virtual void  ChangeScene(std::string name) = 0;
    virtual void  PauseAllSystems(bool toPause) = 0;
    virtual bool  IsGamePaused() const = 0;
    virtual float GetFps() const = 0;
    virtual void  SetDeltaMultiplier(float m) = 0;
    virtual float GetDeltaMultiplier() const = 0;

    /* =========================================================================== */
    /*                              Graphics / FX                                  */
    /* =========================================================================== */
    virtual void ShakeCamera(float duration, float amplitude) = 0;

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

    /* =========================================================================== */
    /*                                MeshRenderer                                 */
    /* =========================================================================== */
    virtual std::optional<uint32_t> GetMeshId(int id) = 0;
    virtual void SetMeshId(int id, uint32_t meshId) = 0;

    /* =========================================================================== */
    /*                                  Lighting                                   */
    /* =========================================================================== */
    virtual bool HasLight(int id) = 0;
    virtual void AddLight(int id) = 0;
    virtual void RemoveLight(int id) = 0;
    virtual void SetLightPosition(int id, float x, float y, float z) = 0;
    virtual void SetLightIntensity(int id, float r, float g, float b) = 0;
    virtual void SetLightType(int id, int typeEnum /*0:POINT,1:DIRECTIONAL,2:SPOTLIGHT*/) = 0;
    virtual void SetLightForward(int id, float x, float y, float z) = 0;
    virtual void SetShadowType(int id, int shadowEnum /*0:NONE,1:MAPPED,2:SCREEN_SPACE*/) = 0;

};
