#pragma once
#include "IEngineAPI.h"

#include <optional>
#include <string>
#include <string_view>

// ECS & Metadata
#include "ECS/Controller.h"
#include "ECS/ECSTypes.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cPhysics.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/sMetaData.h"

class EngineAPIAdapter final : public IEngineAPI {
public:
    EngineAPIAdapter(PAIN::ECS::Controller& ecs,
        PAIN::MetaData::Service& meta) 
        : ecs_(ecs), meta_(meta) {
    }

    // ---------------- Entities / Prefabs ----------------
    int  CreateEntity(std::string layer = "", std::string name = "") override;
    void DeleteEntity(int id) override;
    int  CreatePrefabInstance(std::string prefab,
        std::string layer = "",
        std::string name = "") override;

    // ---------------- Lookup ----------------
    std::optional<int> FindEntity(std::string_view name) override;
    int  GetImageID(std::string_view) override;
    int  GetAnimationID(std::string_view) override;
    int  GetScriptID(std::string_view) override;
    int  GetAudioID(std::string_view) override;

    // ---------------- Metadata (Name/Tag/Group) ----------------
    void SetEntityName(int id, std::string name) override;
    std::optional<std::string> GetEntityName(int id) override;

    void AddTag(int id, std::string tag) override;
    void RemoveTag(int id, std::string tag) override;
    bool HasTag(int id, std::string tag) override;

    void AssignGroup(int id, std::string group) override;
    void UnassignGroup(int id) override;
    std::optional<std::string> GetGroup(int id) override;

    // ---------------- Transform ----------------
    glm::vec3 GetPosition(int id) override;
    void SetPosition(int id, glm::vec3 p) override;
    glm::vec3 GetScale(int id) override;
    void SetScale(int id, glm::vec3 s) override;
    //void RotateEulerDeg(int id, glm::vec3 eulerDeg) override;

    // ---------------- Physics ----------------
    glm::vec3 GetVelocity(int id) override;
    void SetVelocity(int id, glm::vec3 v) override;

    // ---------------- Audio ----------------
    std::optional<int> PlayAudio(int entityId, std::string_view sound) override;
    void PauseAudio(int instanceId) override;
    void ResumeAudio(int instanceId) override;
    void SetVolume(int instanceId, float v) override;

    // ---------------- Scene / System ----------------
    void  ChangeScene(std::string name) override;
    void  PauseAllSystems(bool toPause) override;
    bool  IsGamePaused() const override;
    float GetFps() const override;
    void  SetDeltaMultiplier(float m) override;
    float GetDeltaMultiplier() const override;

    // ---------------- Graphics / FX ----------------
    void ShakeCamera(float duration, float amplitude) override;
    void SetGlobalIlluminance(float v) override;
    void SetAmbientColor(float r, float g, float b) override;
    void ShowVignette(bool on, float r, float g, float b, float radius) override;

    // ---------------- Particles ----------------
    void SpawnParticles(int entityId, int count, bool ignoreRotation) override;

    // ---------------- Input Helpers ----------------
    glm::vec2 GetMouseWorld() const override;
    glm::vec2 GetMouseView() const override;

private:
    inline static PAIN::ECS::Entity::Type asEntity(int id) {
        return static_cast<PAIN::ECS::Entity::Type>(id);
    }

private:
    PAIN::ECS::Controller& ecs_;
    PAIN::MetaData::Service& meta_;

    // Optionally hold refs/pointers to Audio/Graphics/Particles/Window etc
    // Add ctor params and store them here when ready to wire those
};
