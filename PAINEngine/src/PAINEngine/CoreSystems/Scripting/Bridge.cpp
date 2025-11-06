#include "Bridge.h"
#include <string>
#include <unordered_map>
#include <optional>
#include <memory>
#include "EngineAPIAdapter.h"
#include <glm/glm.hpp>


namespace PAIN::Event { class Event; }

class TinyAPI : public IEngineAPI { // for testing
public:

    // ===== Entity Management =====
    int CreateEntity(std::string layer, std::string name) override {
        uint32_t id = ++next_;
        ents_[id].name = name;
        return id;
    }

    void DeleteEntity(int id) override { ents_.erase(id); }

    int CreatePrefabInstance(std::string, std::string, std::string) override {
        return CreateEntity("", "");
    }

    // ===== Lookup =====
    std::optional<int> FindEntity(std::string_view) override { return std::nullopt; }
    int GetImageID(std::string_view) override { return -1; }
    int GetScriptID(std::string_view) override { return -1; }
    //int GetAudioID(std::string_view) override { return -1; }
    int GetModelID(std::string_view) override { return -1; }
    int GetFontID(std::string_view) override { return -1; }
    int GetScenesID(std::string_view) override { return -1; }
    int GetPrefabsID(std::string_view) override { return -1; }
    int GetDataID(std::string_view) override { return -1; }
    int GetShaderID(std::string_view) override { return -1; }

    // ===== Metadata =====
    void SetEntityName(int id, std::string name) override { ents_[id].name = name; }
    std::optional<std::string> GetEntityName(int id) override { return ents_[id].name; }
    void AddTag(int, std::string) override {}
    void RemoveTag(int, std::string) override {}
    bool HasTag(int, std::string) override { return false; }
    void AssignGroup(int, std::string) override {}
    void UnassignGroup(int) override {}
    std::optional<std::string> GetGroup(int) override { return std::nullopt; }

    // ===== Transform =====
    glm::vec3 GetPosition(int id) override { return ents_[id].pos; }
    void SetPosition(int id, glm::vec3 p) override { ents_[id].pos = p; }
    glm::vec3 GetScale(int id) override { return ents_[id].scale; }
    void SetScale(int id, glm::vec3 s) override { ents_[id].scale = s; }

    // ===== Physics =====
    glm::vec3 GetVelocity(int) override { return { 0,0,0 }; }
    void SetVelocity(int, glm::vec3) override {}

    // ===== Audio =====
    /*int Audio_Play(const std::string&, float, float, float, float) override { return -1; }
    int Audio_PlayRandomFrom(const std::string&, float, float, float, float) override { return -1; }
    bool Audio_Stop(int) override { return false; }
    void Audio_StopAll() override {}
    bool Audio_Pause(int) override { return false; }
    bool Audio_Resume(int) override { return false; }
    void Audio_PauseAll() override {}
    void Audio_ResumeAll() override {}
    bool Audio_SetChannelVolumeDb(int, float) override { return false; }
    bool Audio_SetChannelPosition(int, float, float, float) override { return false; }
    void Audio_SetListener(float, float, float, float, float, float, float, float, float, float, float, float) override {}
    bool Audio_SetGroupVolumeDb(const std::string&, float) override { return false; }
    bool Audio_FadeGroupToDb(const std::string&, float, float) override { return false; }
    bool Audio_SetMuteAll(bool) override { return false; }*/

    // ===== Scene / System =====
    void ChangeScene(std::string) override {}
    void PauseAllSystems(bool) override {}
    bool IsGamePaused() const override { return false; }
    float GetFps() const override { return 60.0f; }
    void SetDeltaMultiplier(float) override {}
    float GetDeltaMultiplier() const override { return 1.0f; }

    // ===== Graphics / FX =====
    void ShakeCamera(float, float) override {}

    // ===== Particles =====
    void SpawnParticles(int, int, bool) override {}

    // ===== Input =====
    bool Input_IsKeyDown(int) override { return false; }
    bool Input_WasKeyPressed(int) override { return false; }
    bool Input_WasKeyReleased(int) override { return false; }
    bool Input_IsMouseDown(int) override { return false; }
    bool Input_WasMousePressed(int) override { return false; }
    bool Input_WasMouseReleased(int) override { return false; }
    glm::vec2 Input_GetMousePos() override { return { 0,0 }; }
    glm::vec2 Input_GetScrollDelta() override { return { 0,0 }; }
    bool Input_IsCursorInWindow() override { return true; }
    void Input_EndFrame() override {}
    void Input_OnEvent(PAIN::Event::Event&) override {}

    // ===== MeshRenderer =====
    std::optional<uint32_t> GetMeshId(int id) override { return ents_[id].meshId; }
    void SetMeshId(int id, uint32_t m) override { ents_[id].meshId = m; }

    // ===== Lighting =====
    bool HasLight(int id) override { return ents_[id].hasLight; }
    void AddLight(int id) override { ents_[id].hasLight = true; }
    void RemoveLight(int id) override { ents_[id].hasLight = false; }
    void SetLightPosition(int, float, float, float) override {}
    void SetLightIntensity(int, float, float, float) override {}
    void SetLightType(int, int) override {}
    void SetLightForward(int, float, float, float) override {}
    void SetShadowType(int, int) override {}

private:
    struct Entity {
        std::string name = "Entity";
        glm::vec3 pos = { 0,0,0 };
        glm::vec3 scale = { 1,1,1 };
        uint32_t meshId = 0;
        bool hasLight = false;
    };

    uint32_t next_ = 0;
    std::unordered_map<uint32_t, Entity> ents_;
};


namespace PAIN::Scripting {

    EngineWiring::EngineWiring() = default;
    EngineWiring::~EngineWiring() = default;
    EngineWiring::EngineWiring(EngineWiring&&) noexcept = default;
    EngineWiring& EngineWiring::operator=(EngineWiring&&) noexcept = default;

    EngineWiring CreateMinimalEngineForAndroid() {
        EngineWiring out{};
        out.api = std::make_shared<TinyAPI>();
        return out;  // This uses move semantics, not copy
    }

    EngineWiring CreateProductionEngine(
        PAIN::ECS::Controller& ecs,
        PAIN::MetaData::Service& meta,
        PAIN::Assets::Manager* assets,
        //PAIN::Audio::Audio* audio,
        PAIN::Path::Path* fs)
    {
        EngineWiring out{};
        out.api = std::make_unique<EngineAPIAdapter>(ecs, meta, assets, //audio
                                                      fs);
        return out;
    }

}
