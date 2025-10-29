#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/**
 * @brief Interface exposing high-level engine operations to scripting systems (e.g., LuaManager).
 *
 * This allows LuaManager (or any other scripting environment) to interact with the engine
 * without directly depending on ECS, AssetManager, or System internals.
 */
struct IEngineAPI {
    virtual ~IEngineAPI() = default;

    // ---------------------- Entities / Prefabs ----------------------
    virtual int  CreateEntity(std::string layer = "", std::string name = "") = 0;
    virtual void DeleteEntity(int id) = 0;
    virtual int  CreatePrefabInstance(std::string prefab,
        std::string layer = "",
        std::string name = "") = 0;

    // ---------------------- Lookup ----------------------
    virtual std::optional<int> FindEntity(std::string_view name) = 0;
    virtual int  GetImageID(std::string_view name) = 0;
    virtual int  GetAnimationID(std::string_view name) = 0;
    virtual int  GetScriptID(std::string_view name) = 0;
    virtual int  GetAudioID(std::string_view name) = 0;

    // Transform
    virtual glm::vec3 GetPosition(int id) = 0;
    virtual void SetPosition(int id, glm::vec3 p) = 0;
    virtual glm::vec3 GetScale(int id) = 0;
    virtual void SetScale(int id, glm::vec3 s) = 0;
    //virtual void RotateEulerDeg(int id, glm::vec3 eulerDeg) = 0;

    // Physics
    virtual glm::vec3 GetVelocity(int id) = 0;
    virtual void SetVelocity(int id, glm::vec3 v) = 0;

    // ---------------------- Audio ----------------------
    virtual std::optional<int> PlayAudio(int entityId, std::string_view sound) = 0;
    virtual void PauseAudio(int instanceId) = 0;
    virtual void ResumeAudio(int instanceId) = 0;
    virtual void SetVolume(int instanceId, float v) = 0;

    // ---------------------- Scene / System ----------------------
    virtual void ChangeScene(std::string name) = 0;
    virtual void PauseAllSystems(bool toPause) = 0;
    virtual bool IsGamePaused() const = 0;
    virtual float GetFps() const = 0;
    virtual void SetDeltaMultiplier(float m) = 0;
    virtual float GetDeltaMultiplier() const = 0;

    // ---------------------- Graphics / FX ----------------------
    virtual void ShakeCamera(float duration, float amplitude) = 0;
    virtual void SetGlobalIlluminance(float v) = 0;
    virtual void SetAmbientColor(float r, float g, float b) = 0;
    virtual void ShowVignette(bool on,
        float r,
        float g,
        float b,
        float radius) = 0;

    // ---------------------- Particles ----------------------
    virtual void SpawnParticles(int entityId,
        int count,
        bool ignoreRotation = false) = 0;

    // ---------------------- Input Helpers ----------------------
    virtual glm::vec2 GetMouseWorld() const = 0;
    virtual glm::vec2 GetMouseView() const = 0;

    // Metadata (Name / Tag / Group)
    virtual void SetEntityName(int id, std::string name) = 0;
    virtual std::optional<std::string> GetEntityName(int id) = 0;
    virtual void AddTag(int id, std::string tag) = 0;
    virtual void RemoveTag(int id, std::string tag) = 0;
    virtual bool HasTag(int id, std::string tag) = 0;
    virtual void AssignGroup(int id, std::string group) = 0;
    virtual void UnassignGroup(int id) = 0;
    virtual std::optional<std::string> GetGroup(int id) = 0;

};
