// =======================================================
// EngineAPIAdapter.cpp
// =======================================================

#include "EngineAPIAdapter.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ECS/Components/cMeshRenderer.h"
#include "ECS/Components/cLight.h"

#ifdef PN_PLATFORM_WINDOWS
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"
#include "CoreSystems/Events/GLFW/AssetEvents.h"
#endif

#ifdef PN_PLATFORM_ANDROID
#include "CoreSystems/Events/Android/AppEvents.h"
#include "CoreSystems/Events/Android/FocusEvents.h"
#include "CoreSystems/Events/Android/OtherEvents.h"
#include "CoreSystems/Events/Android/SurfaceEvents.h"
#include "CoreSystems/Events/Android/TouchEvents.h"
#endif

//using PAIN::Audio::AudioResult;
//using PAIN::Audio::PlaylistDesc;
using namespace PAIN::Event;

/* =========================================================================== */
/*                            Entities / Prefabs                               */
/* =========================================================================== */
int EngineAPIAdapter::CreateEntity(std::string /*layer*/, std::string /*name*/) {
    auto e = ecs_.createEntity(); 
    return asInt(e);
}

void EngineAPIAdapter::DeleteEntity(int id) { ecs_.destroyEntity(asEntity(id)); }

int EngineAPIAdapter::CreatePrefabInstance(std::string /*prefab*/, std::string /*layer*/, std::string /*name*/) {
    // @TODO: Hook to prefab system 
    // now, just create a plain entity so scripts dont break
    return CreateEntity();
}

/* =========================================================================== */
/*                                  Lookup                                     */
/* =========================================================================== */
std::optional<int> EngineAPIAdapter::FindEntity(std::string_view name) {
    auto opt = meta_.getEntityByName(std::string{ name });   // name lookup in Meta service
    if (!opt) return std::nullopt;
    return asInt(*opt);
}
int EngineAPIAdapter::GetImageID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Texture); }
int EngineAPIAdapter::GetScriptID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Script); }
//int EngineAPIAdapter::GetAudioID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Audio); }
int EngineAPIAdapter::GetModelID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Model); }
int EngineAPIAdapter::GetFontID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Font); }
int EngineAPIAdapter::GetScenesID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Scenes); }
int EngineAPIAdapter::GetPrefabsID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Prefabs); }
int EngineAPIAdapter::GetDataID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Data); }
int EngineAPIAdapter::GetShaderID(std::string_view name) { return getIdIfType(name, PAIN::Assets::Type::Shader); }

int EngineAPIAdapter::guidToInt(const PAIN::Assets::GUID& id) {
    uint32_t v = 0; std::memcpy(&v, id.bytes, sizeof(uint32_t));
    return static_cast<int>(v);
}

int EngineAPIAdapter::getIdIfType(std::string_view name, PAIN::Assets::Type want) {
    if (!assets_) return -1;
    auto guid = assets_->findByName(std::string{ name }); // look up by name -> GUID 
    if (!guid.IsValid()) return -1;
    auto type = assets_->getTypeByGUID(guid); // check type
    if (type != want) return -1;
    return guidToInt(guid); //convert GUID to int
}

/* =========================================================================== */
/*                     Metadata (name / tags / groups)                         */
/* =========================================================================== */
std::optional<std::string> EngineAPIAdapter::GetEntityName(int id) {
    auto s = meta_.getEntityName(asEntity(id));
    if (s.empty()) return std::nullopt;
    return s;
}
void EngineAPIAdapter::SetEntityName(int id, std::string name) { meta_.setEntityName(asEntity(id), name); }
void EngineAPIAdapter::AddTag(int id, std::string tag) { meta_.addTag(asEntity(id), tag); }
void EngineAPIAdapter::RemoveTag(int id, std::string tag) { meta_.removeTag(asEntity(id), tag); }
bool EngineAPIAdapter::HasTag(int id, std::string tag) { return meta_.hasTag(asEntity(id), tag); }
void EngineAPIAdapter::AssignGroup(int id, std::string g) { meta_.assignToGroup(asEntity(id), g); }
void EngineAPIAdapter::UnassignGroup(int id) { meta_.unassignFromGroup(asEntity(id)); }
std::optional<std::string> EngineAPIAdapter::GetGroup(int id) { return meta_.getEntityGroup(asEntity(id)); }

/* =========================================================================== */
/*                                Transform                                    */
/* =========================================================================== */
glm::vec3 EngineAPIAdapter::GetPosition(int id) {
    auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id));
    if (!opt) return { 0,0,0 };
    const auto& t = opt->get();
    return { t.position.x, t.position.y, t.position.z };
}

void EngineAPIAdapter::SetPosition(int id, glm::vec3 p) {
    /*auto& t = ensure<PAIN::Transform>(id);     
    t.position = { p.x, p.y, p.z };*/

    auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id));
    if (!opt) {
        PN_CORE_WARN("SetPosition: Entity {} has no Transform component", id);
        return;
    }
    auto& t = opt->get();
    t.position = { p.x, p.y, p.z };
}

glm::vec3 EngineAPIAdapter::GetScale(int id) {
    auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id));
    if (!opt) return { 1,1,1 };
    const auto& t = opt->get();
    return { t.scale.x, t.scale.y, t.scale.z };
}

void EngineAPIAdapter::SetScale(int id, glm::vec3 s) {
    /*auto& t = ensure<PAIN::Transform>(id);     
    t.scale = { s.x, s.y, s.z };*/

    auto opt = ecs_.getEntityComponent<PAIN::Transform>(asEntity(id));
    if (!opt) {
        PN_CORE_WARN("SetScale: Entity {} has no Transform component", id);
        return;
    }
    auto& t = opt->get();
    t.scale = { s.x, s.y, s.z };
}

/* =========================================================================== */
/*                                  Physics                                    */
/* =========================================================================== */
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
        // @TODO: also push to Jolt body if needed 
    }
}

/* =========================================================================== */
/*                                   Audio                                     */
/* =========================================================================== */
//int EngineAPIAdapter::Audio_Play(const std::string& vpath, float x, float y, float z, float volumeDb) {
//    if (!audio_) return -1;
//    const std::string real = resolveMaybeVirtual(vpath);
//    auto ch = audio_->play(real, { x,y,z }, volumeDb);    
//    return ch ? ch->value : -1;                         
//}
//
//int EngineAPIAdapter::Audio_PlayRandomFrom(const std::string& playlist, float x, float y, float z, float volumeDb) {
//    if (!audio_) return -1;
//    auto ch = audio_->playRandom(playlist, { x,y,z }, volumeDb);          
//    return ch ? ch->value : -1;
//}
//
//bool EngineAPIAdapter::Audio_Stop(int channelId) {
//    if (!audio_) return false;
//    return audio_->stop({ channelId }) == AudioResult::Ok;                 
//}
//void EngineAPIAdapter::Audio_StopAll() { if (audio_) audio_->stopAll(); }   
//bool EngineAPIAdapter::Audio_Pause(int channelId) { return audio_ && audio_->pauseChannel({ channelId }) == AudioResult::Ok; } 
//bool EngineAPIAdapter::Audio_Resume(int channelId) { return audio_ && audio_->resumeChannel({ channelId }) == AudioResult::Ok; } 
//void EngineAPIAdapter::Audio_PauseAll() { if (audio_) audio_->pauseAll(); }  
//void EngineAPIAdapter::Audio_ResumeAll() { if (audio_) audio_->resumeAll(); } 
//bool EngineAPIAdapter::Audio_SetChannelVolumeDb(int channelId, float db) { return audio_ && audio_->setVolumeDb({ channelId }, db) == AudioResult::Ok; }
//bool EngineAPIAdapter::Audio_SetChannelPosition(int channelId, float x, float y, float z) { return audio_ && audio_->setPosition({ channelId }, { x,y,z }) == AudioResult::Ok; }
//void EngineAPIAdapter::Audio_SetListener(float px, float py, float pz, float vx, float vy, float vz, float fx, float fy, float fz, float ux, float uy, float uz) { if (audio_) audio_->setListener({ px,py,pz }, { vx,vy,vz }, { fx,fy,fz }, { ux,uy,uz }); }
//bool EngineAPIAdapter::Audio_SetGroupVolumeDb(const std::string& group, float db) { return audio_ && audio_->setGroupVolumeDb(group.c_str(), db) == AudioResult::Ok; }
//bool EngineAPIAdapter::Audio_FadeGroupToDb(const std::string& group, float targetDb, float seconds) { return audio_ && audio_->fadeGroupToDb(group.c_str(), targetDb, seconds) == AudioResult::Ok; }
//bool EngineAPIAdapter::Audio_SetMuteAll(bool mute) { return audio_ && audio_->setMuteAll(mute) == AudioResult::Ok; }

/* =========================================================================== */
/*                           Scene / System state                              */
/* =========================================================================== */
void EngineAPIAdapter::ChangeScene(std::string) {}
void EngineAPIAdapter::PauseAllSystems(bool) {}
bool EngineAPIAdapter::IsGamePaused() const { return false; }
float EngineAPIAdapter::GetFps() const { return 0.0f; }
void EngineAPIAdapter::SetDeltaMultiplier(float) {}
float EngineAPIAdapter::GetDeltaMultiplier() const { return 1.0f; }

/* =========================================================================== */
/*                              Graphics / FX                                  */
/* =========================================================================== */
void EngineAPIAdapter::ShakeCamera(float /*duration*/, float /*amplitude*/) {}

/* =========================================================================== */
/*                                Particles                                    */
/* =========================================================================== */
void EngineAPIAdapter::SpawnParticles(int /*entityId*/, int /*count*/, bool /*ignoreRotation*/) {}

/* =========================================================================== */
/*                              Input Helpers                                  */
/* =========================================================================== */
void EngineAPIAdapter::Input_OnEvent(Event& e)
{
    Dispatcher d{ e };  

#ifdef PN_PLATFORM_WINDOWS
    d.Dispatch<KeyPressed>([&](KeyPressed& ev) {          
        keysDown_.insert(ev.getKeyCode());
        keysPressed_.insert(ev.getKeyCode());
        return false;
        });
    d.Dispatch<KeyReleased>([&](KeyReleased& ev) {        
        keysDown_.erase(ev.getKeyCode());
        keysReleased_.insert(ev.getKeyCode());
        return false;
        });
    d.Dispatch<KeyRepeated>([&](KeyRepeated&) {           
        return false;
        });

    d.Dispatch<MouseBtnPressed>([&](MouseBtnPressed& ev) {
        mouseDown_.insert(ev.getBtnCode());
        mousePressed_.insert(ev.getBtnCode());
        return false;
        });
    d.Dispatch<MouseBtnReleased>([&](MouseBtnReleased& ev) {
        mouseDown_.erase(ev.getBtnCode());
        mouseReleased_.insert(ev.getBtnCode());
        return false;
        });
    d.Dispatch<MouseMoved>([&](MouseMoved& ev) {
        mousePos_ = ev.getWindowPos();
        return false;
        });
    d.Dispatch<MouseScrolled>([&](MouseScrolled& ev) {
        scrollDelta_ += ev.getOffset();
        return false;
        });
    d.Dispatch<CursorEntered>([&](CursorEntered& ev) {
        cursorIn_ = ev.checkCursorEntered();
        return false;
        });

    // react to focus/resolution changes here
    d.Dispatch<WindowFocused>([&](WindowFocused&) { return false; });
    d.Dispatch<WindowResized>([&](WindowResized&) { return false; });
    d.Dispatch<FileDropped>([&](FileDropped&) { return false; });
#endif

#ifdef PN_PLATFORM_ANDROID
    // --- Touch -> mouse abstraction (use 0 pointer as mouse)
    d.Dispatch<TouchDown>([&](TouchDown& ev) {
        mousePos_ = { ev.getX(), ev.getY() };
        mouseDown_.insert(0);            // button 0
        mousePressed_.insert(0);
        return false;
        });
    d.Dispatch<TouchUp>([&](TouchUp& ev) {
        mousePos_ = { ev.getX(), ev.getY() };
        mouseDown_.erase(0);
        mouseReleased_.insert(0);
        return false;
        });
    d.Dispatch<TouchMove>([&](TouchMove& ev) {
        mousePos_ = { ev.getX(), ev.getY() };
        return false;
        });
    d.Dispatch<TouchCancel>([&](TouchCancel&) {
        mouseDown_.erase(0);
        mouseReleased_.insert(0);
        return false;
        });

    // --- Focus / app lifecycle ---
    d.Dispatch<FocusGained>([&](FocusGained&) { cursorIn_ = true;  return false; });
    d.Dispatch<FocusLost>([&](FocusLost&) {   cursorIn_ = false; return false; });
    d.Dispatch<AppStart>([&](AppStart&) { return false; });
    d.Dispatch<AppResume>([&](AppResume&) { return false; });
    d.Dispatch<AppPause>([&](AppPause&) { return false; });
    d.Dispatch<AppStop>([&](AppStop&) { return false; });
    d.Dispatch<AppDestroy>([&](AppDestroy&) { return false; });

    // --- Surface (window) changes ---
    d.Dispatch<SurfaceCreated>([&](SurfaceCreated&) { return false; });
    d.Dispatch<SurfaceChanged>([&](SurfaceChanged& sc) { /* width = sc.getWidth(); height = sc.getHeight(); */ return false; });
    d.Dispatch<SurfaceDestroyed>([&](SurfaceDestroyed&) { return false; });

    // --- Android keys / back button ---
    d.Dispatch<AndroidKeyDown>([&](AndroidKeyDown& k) {
        keysDown_.insert(k.getKeyCode());
        keysPressed_.insert(k.getKeyCode());
        return false;
        });
    d.Dispatch<AndroidKeyUp>([&](AndroidKeyUp& k) {
        keysDown_.erase(k.getKeyCode());
        keysReleased_.insert(k.getKeyCode());
        return false;
        });
    d.Dispatch<BackButton>([&](BackButton&) {
        // Example: treat as Escape (-1) or expose a separate flag
        keysPressed_.insert(-1);
        return false;
        });

    // --- System signals ---
    d.Dispatch<LowMemory>([&](LowMemory&) { return false; });
    d.Dispatch<ConfigurationChanged>([&](ConfigurationChanged&) { return false; });
    d.Dispatch<SensorEvent>([&](SensorEvent&) { return false; });
#endif
}

bool EngineAPIAdapter::Input_IsKeyDown(int key) { return keysDown_.count(key) != 0; }
bool EngineAPIAdapter::Input_WasKeyPressed(int key) { 
    return keysPressed_.count(key) != 0; 
}
bool EngineAPIAdapter::Input_WasKeyReleased(int key) { return keysReleased_.count(key) != 0; }
bool EngineAPIAdapter::Input_IsMouseDown(int btn) { return mouseDown_.count(btn) != 0; }
bool EngineAPIAdapter::Input_WasMousePressed(int btn) { return mousePressed_.count(btn) != 0; }
bool EngineAPIAdapter::Input_WasMouseReleased(int btn) { return mouseReleased_.count(btn) != 0; }
glm::vec2 EngineAPIAdapter::Input_GetMousePos() { return mousePos_; }
glm::vec2 EngineAPIAdapter::Input_GetScrollDelta() { return scrollDelta_; }
bool EngineAPIAdapter::Input_IsCursorInWindow() { return cursorIn_; }

void EngineAPIAdapter::Input_EndFrame() {
    keysPressed_.clear();
    keysReleased_.clear();
    mousePressed_.clear();
    mouseReleased_.clear();
    scrollDelta_ = { 0.0f, 0.0f };
}

/* =========================================================================== */
/*                                MeshRenderer                                 */
/* =========================================================================== */
std::optional<uint32_t> EngineAPIAdapter::GetMeshId(int id) { 
    PAIN::MeshRenderer* mr = nullptr; 
    if (!try_get<PAIN::MeshRenderer>(id, mr)) return std::nullopt; 
    return mr->mesh_id; 
}
void EngineAPIAdapter::SetMeshId(int id, uint32_t meshId) { 
    /*auto& mr = ensure<PAIN::MeshRenderer>(id); 
    mr.mesh_id = meshId; */

    PAIN::MeshRenderer* mr = nullptr;
    if (!try_get<PAIN::MeshRenderer>(id, mr)) {
        PN_CORE_ERROR("SetMeshId: Entity {} has no MeshRenderer component", id);
        return;
    }
    mr->mesh_id = meshId;
}

/* =========================================================================== */
/*                                  Lighting                                   */
/* =========================================================================== */
bool EngineAPIAdapter::HasLight(int id) { 
    PAIN::Lighting* l = nullptr; 
    return try_get<PAIN::Lighting>(id, l); 
}
void EngineAPIAdapter::AddLight(int id) { 
    (void)ensure<PAIN::Lighting>(id); 
}
void EngineAPIAdapter::RemoveLight(int id) {
    ecs_.removeEntityComponent<PAIN::Lighting>(asEntity(id)); 
}
void EngineAPIAdapter::SetLightPosition(int id, float x, float y, float z) { 
    /*auto& l = ensure<PAIN::Lighting>(id); 
    l.position = { x,y,z }; */

    PAIN::Lighting* l = nullptr;
    if (!try_get<PAIN::Lighting>(id, l)) {
        PN_CORE_ERROR("SetLightPosition: Entity {} has no Lighting component", id);
        return;
    }
    l->position = { x, y, z };
}
void EngineAPIAdapter::SetLightIntensity(int id, float r, float g, float b) { 
    /*auto& l = ensure<PAIN::Lighting>(id); 
    l.light_intensity = { r,g,b }; */

    PAIN::Lighting* l = nullptr;
    if (!try_get<PAIN::Lighting>(id, l)) {
        PN_CORE_ERROR("SetLightIntensity: Entity {} has no Lighting component", id);
        return;
    }
    l->light_intensity = { r, g, b };
}
void EngineAPIAdapter::SetLightType(int id, int ty) { 
    /*auto& l = ensure<PAIN::Lighting>(id); 
    l.light_type = static_cast<PAIN::TYPES>(ty); */

    PAIN::Lighting* l = nullptr;
    if (!try_get<PAIN::Lighting>(id, l)) {
        PN_CORE_ERROR("SetLightType: Entity {} has no Lighting component", id);
        return;
    }
    l->light_type = static_cast<PAIN::TYPES>(ty);
}
void EngineAPIAdapter::SetLightForward(int id, float x, float y, float z) { 
    /*auto& l = ensure<PAIN::Lighting>(id); 
    l.forward = glm::normalize(glm::vec3{ x,y,z }); */

    PAIN::Lighting* l = nullptr;
    if (!try_get<PAIN::Lighting>(id, l)) {
        PN_CORE_ERROR("SetLightForward: Entity {} has no Lighting component", id);
        return;
    }
    l->forward = glm::normalize(glm::vec3{ x, y, z });
}
void EngineAPIAdapter::SetShadowType(int id, int st) { 
    /*auto& l = ensure<PAIN::Lighting>(id); 
    l.shadow_type = static_cast<PAIN::SHADOW_TYPES>(st); */

    PAIN::Lighting* l = nullptr;
    if (!try_get<PAIN::Lighting>(id, l)) {
        PN_CORE_ERROR("SetShadowType: Entity {} has no Lighting component", id);
        return;
    }
    l->shadow_type = static_cast<PAIN::SHADOW_TYPES>(st);
}