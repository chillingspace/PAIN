#include "EngineAPIAdapter.h"
#include <glm/gtc/quaternion.hpp>
//#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ---------------- Entities / Prefabs ----------------
int EngineAPIAdapter::CreateEntity(std::string /*layer*/, std::string /*name*/) {
    auto e = ecs_.createEntity();                    // ECS create/destroy via Controller
    return static_cast<int>(e);
}

void EngineAPIAdapter::DeleteEntity(int id) {
    ecs_.destroyEntity(asEntity(id));
}

int EngineAPIAdapter::CreatePrefabInstance(std::string /*prefab*/,
                                           std::string /*layer*/,
                                           std::string /*name*/) {
    // TODO: Hook to your prefab system when available.
    // For now, just create a plain entity so scripts dont break
    return CreateEntity();
}

// ---------------- Lookup ----------------
std::optional<int> EngineAPIAdapter::FindEntity(std::string_view name) {
    auto opt = meta_.getEntityByName(std::string{ name });   // name lookup in Meta service
    if (!opt) return std::nullopt;
    return static_cast<int>(*opt);
}

int EngineAPIAdapter::GetImageID(std::string_view) { 
    return -1; 
} // TODO: Asset system

int EngineAPIAdapter::GetAnimationID(std::string_view) { 
    return -1; 
}

int EngineAPIAdapter::GetScriptID(std::string_view) { 
    return -1; 
}

int EngineAPIAdapter::GetAudioID(std::string_view) { 
    return -1; 
}

// ---------------- Metadata (Name/Tag/Group) ----------------
void EngineAPIAdapter::SetEntityName(int id, std::string name) {
    meta_.setEntityName(asEntity(id), name);
}

std::optional<std::string> EngineAPIAdapter::GetEntityName(int id) {
    auto s = meta_.getEntityName(asEntity(id));
    if (s.empty()) return std::nullopt;
    return s;
}

void EngineAPIAdapter::AddTag(int id, std::string tag) { 
    meta_.addTag(asEntity(id), tag); 
}

void EngineAPIAdapter::RemoveTag(int id, std::string tag) { 
    meta_.removeTag(asEntity(id), tag); 
}

bool EngineAPIAdapter::HasTag(int id, std::string tag) { 
    return meta_.hasTag(asEntity(id), tag); 
}

void EngineAPIAdapter::AssignGroup(int id, std::string g) { 
    meta_.assignToGroup(asEntity(id), g); 
}

void EngineAPIAdapter::UnassignGroup(int id) { 
    meta_.unassignFromGroup(asEntity(id)); 
}

std::optional<std::string> EngineAPIAdapter::GetGroup(int id) {
    return meta_.getEntityGroup(asEntity(id));
}

// ---------------- Transform ----------------
glm::vec3 EngineAPIAdapter::GetPosition(int id) {
    auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id));
    if (!opt) return { 0,0,0 };
    const auto& t = opt->get();
    return { t.position.x, t.position.y, t.position.z };
}

void EngineAPIAdapter::SetPosition(int id, glm::vec3 p) {
    if (auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id))) {
        auto& t = opt->get();
        t.position = { p.x, p.y, p.z };
    }
}

glm::vec3 EngineAPIAdapter::GetScale(int id) {
    auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id));
    if (!opt) return { 1,1,1 };
    const auto& t = opt->get();
    return { t.scale.x, t.scale.y, t.scale.z };
}

void EngineAPIAdapter::SetScale(int id, glm::vec3 s) {
    if (auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id))) {
        opt->get().scale = { s.x, s.y, s.z };
    }
}

//void EngineAPIAdapter::RotateEulerDeg(int id, glm::vec3 eulerDeg) {
//    if (auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id))) {
//        auto& t = opt->get();
//        // Compose new rotation (additive in Euler, then convert)
//        glm::vec3 current = glm::degrees(glm::eulerAngles(t.rotation));
//        glm::vec3 next = current + glm::vec3(eulerDeg.x, eulerDeg.y, eulerDeg.z);
//        t.rotation = glm::quat(glm::radians(next));
//    }
//}

// ---------------- Physics ----------------
glm::vec3 EngineAPIAdapter::GetVelocity(int id) {
    auto opt = ecs_.getEntityComponent<PAIN::Physics::RigidBody3D>(asEntity(id));
    if (!opt) return { 0,0,0 };
    const auto& rb = opt->get();
    return { rb.velocity.x, rb.velocity.y, rb.velocity.z };
}

void EngineAPIAdapter::SetVelocity(int id, glm::vec3 v) {
    if (auto opt = ecs_.getEntityComponent<PAIN::Physics::RigidBody3D>(asEntity(id))) {
        auto& rb = opt->get();
        rb.velocity = { v.x, v.y, v.z };
        // TODO: also push to Jolt body if needed (rb.bodyID)
    }
}

// ---------------- Audio ----------------
std::optional<int> EngineAPIAdapter::PlayAudio(int /*entityId*/, std::string_view /*sound*/) {
    // TODO: bridge to AudioSystem; return instance id
    return std::nullopt;
}
void EngineAPIAdapter::PauseAudio(int /*instanceId*/) {

}

void EngineAPIAdapter::ResumeAudio(int /*instanceId*/) {

}

void EngineAPIAdapter::SetVolume(int /*instanceId*/, float /*v*/) {

}

// ---------------- Scene / System ----------------
void  EngineAPIAdapter::ChangeScene(std::string /*name*/) {

}

void  EngineAPIAdapter::PauseAllSystems(bool /*toPause*/) {

}

bool  EngineAPIAdapter::IsGamePaused() const { 
    return false; 
}

float EngineAPIAdapter::GetFps() const { 
    return 0.0f; 
}

void  EngineAPIAdapter::SetDeltaMultiplier(float /*m*/) {

}

float EngineAPIAdapter::GetDeltaMultiplier() const { 
    return 1.0f; 
}

// ---------------- Graphics / FX ----------------
void EngineAPIAdapter::ShakeCamera(float /*duration*/, float /*amplitude*/) {

}

void EngineAPIAdapter::SetGlobalIlluminance(float /*v*/) {

}

void EngineAPIAdapter::SetAmbientColor(float /*r*/, float /*g*/, float /*b*/) {

}

void EngineAPIAdapter::ShowVignette(bool /*on*/, float /*r*/, float /*g*/, float /*b*/, float /*radius*/) {

}

// ---------------- Particles ----------------
void EngineAPIAdapter::SpawnParticles(int /*entityId*/, int /*count*/, bool /*ignoreRotation*/) {

}

// ---------------- Input Helpers ----------------
glm::vec2 EngineAPIAdapter::GetMouseWorld() const { 
    return { 0.f, 0.f }; 
}  // @TODO: input manager bridge

glm::vec2 EngineAPIAdapter::GetMouseView()  const { 
    return { 0.f, 0.f }; 
}
