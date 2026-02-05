// =======================================================
// EngineAPIAdapter.cpp
// =======================================================

#include "EngineAPIAdapter.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ECS/Components/cMeshRenderer.h"
#include "ECS/Components/cLight.h"
#include "ECS/Components/cMetadata.h"
#include "ECS/Components/cEntity.h"
#include "ECS/Components/cMeshRenderer.h"
#include "Systems/Transform/sysTransform.h"
#include "Common/AssetTypes/src/AssetData.h"
#include "Systems/Physics/sysPhysics.h"
#include <glm/gtx/quaternion.hpp>   // for glm::eulerAngles, glm::quat(vec3)
#include "CoreSystems/Scene/Camera.h"
#include "CoreSystems/Scene/Scene.h"


#ifdef PN_PLATFORM_WINDOWS
#include "CoreSystems/Events/GLFW/KeyEvents.h"
#include "CoreSystems/Events/GLFW/MouseEvents.h"
#include "CoreSystems/Events/GLFW/WindowEvents.h"
#include "CoreSystems/Events/GLFW/AssetEvents.h"
#ifdef _DEBUG
#include <imgui.h>
#endif
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
//using namespace PAIN::Event;


namespace PAIN {

    /* =========================================================================== */
    /*                            Entities / Prefabs                               */
    /* =========================================================================== */

    entt::entity EngineAPIAdapter::CreateEntity(std::string /*layer*/, std::string /*name*/) {
        return ecs_.createEntity();
    }

    void EngineAPIAdapter::DeleteEntity(entt::entity entityId) {
        ecs_.destroyEntity(entityId);
    }

    entt::entity EngineAPIAdapter::CreatePrefabInstance(std::string prefab, std::string layer, std::string name) {
        // @TODO: Hook to prefab system 
        // now, just create a plain entity so scripts dont break
        // return CreateEntity();


       // // validate 
       // if (!assets_ || !fs_) {
       //     PN_CORE_ERROR("CreatePrefabInstance: missing asset/fs services");
       //     return entt::null;
       // }

       // // lookup prefab by name
       // PAIN::Assets::GUID guid = assets_->findByName(prefab);
       // if (!guid.IsValid()) {
       //     PN_CORE_ERROR("CreatePrefabInstance: prefab '{}' not found", prefab);
       //     return entt::null;
       // }

       // auto meta_data = assets_->getAssetData(guid);
       // if (!meta_data) {
       //     PN_CORE_ERROR("CreatePrefabInstance: prefab '{}' has no metadata", prefab);
       //     return entt::null;
       // }

       // // resolve to OS path
       // std::string vpath =
       //     fs_->aliasCombineRelative("assets", meta_data->shipped_relative_path.string());

       // std::string osPath = fs_->resolvePath(vpath);


       // // load JSON
       // nlohmann::json j;
       // try {
       //     std::ifstream in(osPath);
       //     if (!in) {
       //         PN_CORE_ERROR("CreatePrefabInstance: cannot open '{}'", osPath);
       //         return entt::null;
       //     }
       //     in >> j;
       // }
       // catch (const std::exception& e) {
       //     PN_CORE_ERROR("CreatePrefabInstance: JSON error '{}': {}", osPath, e.what());
       //     return entt::null;
       // }

       // // create entity 
       // entt::entity e = ecs_.createEntity();

       // // meta
       // if (!name.empty())
       //     meta_.setEntityName(e, name);

       ///* if (!layer.empty())
       //     ecs_.addEntityComponent<PAIN::MetaData::Group>(e, PAIN::MetaData::Group{ layer });*/

       // // deserialize components
       // try {
       //     const nlohmann::json* comps = &j;
       //     if (j.contains("components") && j["components"].is_object())
       //         comps = &j["components"];

       //     ecs_.loadAllComponentsFromJson(e, *comps);
       // }
       // catch (const std::exception& eex) {
       //     PN_CORE_ERROR("CreatePrefabInstance: component hydrate failed: {}", eex.what());
       //     ecs_.destroyEntity(e);
       //     return entt::null;
       // }

       // return e;
        return entt::null;
    }

    /* =========================================================================== */
    /*                                  Lookup                                     */
    /* =========================================================================== */
    std::optional<int> EngineAPIAdapter::FindEntity(std::string_view name) {
        auto& reg = ecs_.getRegistry();
        
        // 1) Fast path: check meta cache BUT VALIDATE
        if (auto opt = meta_.getEntityByName(std::string{ name })) {
            entt::entity e = *opt;
            
            // Validate: entity must be alive and still have this name
            if (reg.valid(e) && reg.all_of<PAIN::Entity::Name>(e)) {
                const auto& entityName = reg.get<PAIN::Entity::Name>(e).name;
                if (entityName == name) {
                    return asInt(e);  // Valid cached entity
                }
            }
            
            // Stale cache entry detected - will be refreshed below
            PN_CORE_WARN("[FindEntity] Stale cache entry for '{}' (entity {}) - refreshing", 
                         name, entt::to_integral(e));
        }
        
        // 2) Fallback: scan the registry for Entity::Name
        auto view = reg.view<PAIN::Entity::Name>();
        for (auto e : view) {
            const auto& n = view.get<PAIN::Entity::Name>(e).name;
            if (n == name) {
                // seed the meta index so future lookups are O(1)
                meta_.setEntityName(e, std::string{ name });
                return asInt(e);
            }
        }

        return std::nullopt;
    }

    static int FindAnimIndex(PAIN::ECS::Controller& ecs, entt::entity entity, const std::string& animName) {
        auto& reg = ecs.getRegistry();

        // We need ModelRenderer to access the asset's animation list
        if (!reg.all_of<PAIN::ModelRenderer>(entity)) return -1;

        const auto& renderer = reg.get<PAIN::ModelRenderer>(entity);
        if (!renderer.cachedModelAsset) return -1;

        const auto& animations = renderer.cachedModelAsset->animations;
        for (int i = 0; i < animations.size(); ++i) {
            if (animations[i].name == animName) {
                return i;
            }
        }
        return -1;
    }

    std::string EngineAPIAdapter::GetAssetGUID(std::string_view name, PAIN::Assets::Type want) {
        if (!assets_) return {};
        auto guid = assets_->findByName(std::string{ name });
        if (!guid.IsValid()) return {};
        if (assets_->getTypeByGUID(guid) != want) return {};
        return guid.ToString();
    }
    std::string EngineAPIAdapter::GetImageGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Texture); }
    std::string EngineAPIAdapter::GetScriptGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Script); }
    std::string EngineAPIAdapter::GetAudioGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Audio); }
    std::string EngineAPIAdapter::GetModelGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Model); }
    std::string EngineAPIAdapter::GetFontGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Font); }
    std::string EngineAPIAdapter::GetScenesGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Scenes); }
    std::string EngineAPIAdapter::GetPrefabsGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Prefabs); }
    std::string EngineAPIAdapter::GetDataGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Data); }
    std::string EngineAPIAdapter::GetShaderGUID(std::string_view name) { return GetAssetGUID(name, PAIN::Assets::Type::Shader); }

    std::string EngineAPIAdapter::GetEntityTexture(entt::entity entityId) {
        auto& reg = ecs_.getRegistry();

        // Check if entity exists and has a texture
        if (reg.valid(entityId) && reg.all_of<PAIN::Texture2D>(entityId)) {
            auto& texComp = reg.get<PAIN::Texture2D>(entityId);
            return texComp.texture_guid.ToString();
        }

        return ""; // Return empty if no texture component found
    }

    void EngineAPIAdapter::SetEntityTexture(entt::entity entityId, const std::string& guidStr) {
        auto& reg = ecs_.getRegistry();

        // 1. Validation: Ensure entity is valid and has the component
        if (!reg.valid(entityId) || !reg.all_of<PAIN::Texture2D>(entityId)) {
            return;
        }

        // 2. Update the component data
        auto& texComp = reg.get<PAIN::Texture2D>(entityId);

        // Convert string to GUID (using the constructor/method available in your engine)
        texComp.texture_guid = PAIN::Assets::GUID(guidStr);
    }

    //int EngineAPIAdapter::guidToInt(const PAIN::Assets::GUID& id) {
    //    uint32_t v = 0; std::memcpy(&v, id.bytes, sizeof(uint32_t));
    //    return static_cast<int>(v);
    //}

    //int EngineAPIAdapter::getIdIfType(std::string_view name, PAIN::Assets::Type want) {
    //    if (!assets_) return -1;
    //    auto guid = assets_->findByName(std::string{ name }); // look up by name -> GUID 
    //    if (!guid.IsValid()) return -1;
    //    auto type = assets_->getTypeByGUID(guid); // check type
    //    if (type != want) return -1;
    //    return guidToInt(guid); //convert GUID to int
    //}

    //void EngineAPIAdapter::SetRegistry(entt::registry* reg)
    //{
    //    currentRegistry_ = reg;
    //}

    /* =========================================================================== */
    /*                     Metadata (name / tags / groups)                         */
    /* =========================================================================== */
    std::optional<std::string> EngineAPIAdapter::GetEntityName(entt::entity entityId) {
        auto s = meta_.getEntityName(entityId);
        if (s.empty()) return std::nullopt;
        return s;
    }
    void EngineAPIAdapter::SetEntityName(entt::entity entityId, std::string name) { meta_.setEntityName(entityId, name); }
    void EngineAPIAdapter::AddTag(entt::entity entityId, std::string tag) { meta_.addTag(entityId, tag); }
    void EngineAPIAdapter::RemoveTag(entt::entity entityId, std::string tag) { meta_.removeTag(entityId, tag); }
    bool EngineAPIAdapter::HasTag(entt::entity entityId, std::string tag) { return meta_.hasTag(entityId, tag); }
    /*void EngineAPIAdapter::AssignGroup(entt::entity entityId, std::string g) { meta_.assignToGroup(entityId, g); }
    void EngineAPIAdapter::UnassignGroup(entt::entity entityId) { meta_.unassignFromGroup(entityId); }
    std::optional<std::string> EngineAPIAdapter::GetGroup(entt::entity entityId) { return meta_.getEntityGroup(entityId); }*/
    std::vector<entt::entity> EngineAPIAdapter::GetEntitiesByTag(const std::string& tag) { return meta_.getEntitiesByTag(tag); }

    /* =========================================================================== */
    /*                                Transform                                    */
    /* =========================================================================== */
    glm::vec3 EngineAPIAdapter::GetPosition(entt::entity entityId) {
        if (auto opt = ecs_.getEntityComponent<PAIN::LocalTransform>(entityId)) {
            return opt->get().position;
        }
        return { 0.f, 0.f, 0.f };
    }

    void EngineAPIAdapter::SetPosition(entt::entity entityId, glm::vec3 p) {
        /*auto& t = ensure<PAIN::Transform>(entityId);
        t.position = { p.x, p.y, p.z };*/

        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::LocalTransform>(entityId)) return;

        auto& t = reg.get<PAIN::LocalTransform>(entityId);
        t.position = p;

        if (auto ts = ecs_.getSystem<PAIN::Transform::System>()) { //recompute worldtransform
            ts->markDirty(entityId, reg);
        }

        if (reg.all_of<PAIN::Physics::RigidBody3D>(entityId)) {
            auto& rb = reg.get<PAIN::Physics::RigidBody3D>(entityId);
            if (auto phys = ecs_.getSystem<PAIN::Physics::System>()) {
                phys->teleportBodyToTransform(entityId, t, rb);
            }
        }
    }

    glm::vec2 EngineAPIAdapter::Get2DPosition(entt::entity entityId) {
        if (auto opt = ecs_.getEntityComponent<PAIN::Texture2D>(entityId)) {
            return opt->get().pos;
        }
        return { 0.f, 0.f};
    }

    void EngineAPIAdapter::Set2DPosition(entt::entity entityId, glm::vec2 p) {
        /*auto& t = ensure<PAIN::Transform>(entityId);
        t.position = { p.x, p.y, p.z };*/

        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Texture2D>(entityId)) return;

        auto& t = reg.get<PAIN::Texture2D>(entityId);
        t.pos = p;

        if (auto ts = ecs_.getSystem<PAIN::Transform::System>()) { //recompute worldtransform
            ts->markDirty(entityId, reg);
        }
    }

    glm::vec3 EngineAPIAdapter::GetScale(entt::entity entityId) {
        if (auto opt = ecs_.getEntityComponent<PAIN::LocalTransform>(entityId)) {
            return opt->get().scale;
        }
        return { 1.f, 1.f, 1.f };
    }

    void EngineAPIAdapter::SetScale(entt::entity entityId, glm::vec3 s) {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::LocalTransform>(entityId)) return;

        auto& t = reg.get<PAIN::LocalTransform>(entityId);
        t.scale = s;

        if (auto ts = ecs_.getSystem<PAIN::Transform::System>()) {
            ts->markDirty(entityId, reg);
        }
    }

    glm::vec3 EngineAPIAdapter::GetRotation(entt::entity entityId)
    {
        if (auto opt = ecs_.getEntityComponent<PAIN::LocalTransform>(entityId)) {
            const auto& t = opt->get();
            // convert quaternion -> euler angles, in rad
            glm::vec3 euler = glm::eulerAngles(t.rotation);
            return euler;
        }
        return { 0.f, 0.f, 0.f };
    }

    void EngineAPIAdapter::SetRotation(entt::entity entityId, glm::vec3 r)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::LocalTransform>(entityId)) return;

        auto& t = reg.get<PAIN::LocalTransform>(entityId);
        t.rotation = glm::quat(r); // euler to quat

        if (auto ts = ecs_.getSystem<PAIN::Transform::System>()) {
            ts->markDirty(entityId, reg);
        }
    }

    /* =========================================================================== */
    /*                                  Physics                                    */
    /* =========================================================================== */
    glm::vec3 EngineAPIAdapter::GetVelocity(entt::entity entityId) {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Physics::RigidBody3D>(entityId)) {
            return { 0.f, 0.f, 0.f };
        }

        auto& rb = reg.get<PAIN::Physics::RigidBody3D>(entityId);

        // Get physics system and read from Jolt
        if (auto phys = ecs_.getSystem<PAIN::Physics::System>()) {
            return phys->getVelocity(entityId);
        }

        return { 0.f, 0.f, 0.f };
    }

    void EngineAPIAdapter::SetVelocity(entt::entity entityId, glm::vec3 v) {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Physics::RigidBody3D>(entityId)) return;

        auto& ph = reg.get<PAIN::Physics::RigidBody3D>(entityId);
        ph.velocity = v;

        // Update the actual Jolt body velocity
        if (auto phys = ecs_.getSystem<PAIN::Physics::System>()) {
            phys->setVelocity(entityId, v);
        }

    }

    bool EngineAPIAdapter::IsGrounded(entt::entity entityId, float maxDistance) {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Physics::RigidBody3D>(entityId)) return false;

		auto& rb = reg.get<PAIN::Physics::RigidBody3D>(entityId);       

        if (auto phys = ecs_.getSystem<PAIN::Physics::System>()) {
			return phys->isGrounded(rb.bodyID, maxDistance);
        }

        return false;
    }

    std::tuple<bool, glm::vec3> EngineAPIAdapter::GetWallNormal(entt::entity entityId, glm::vec3 direction, float checkDistance) {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Physics::RigidBody3D>(entityId)) {
            return { false, glm::vec3(0.0f) };
        }

        auto& rb = reg.get<PAIN::Physics::RigidBody3D>(entityId);

        if (auto phys = ecs_.getSystem<PAIN::Physics::System>()) {
            glm::vec3 normal;
            bool hit = phys->getWallNormal(rb.bodyID, direction, checkDistance, normal);
            return { hit, normal };
        }

        return { false, glm::vec3(0.0f) };
    }

    void EngineAPIAdapter::DisablePhysics(entt::entity entityId) {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Physics::RigidBody3D>(entityId)) return;

        auto& rb = reg.get<PAIN::Physics::RigidBody3D>(entityId);

        if (auto phys = ecs_.getSystem<PAIN::Physics::System>()) {
            phys->disablePhysics(entityId);
        }
    }

    void EngineAPIAdapter::EnablePhysics(entt::entity entityId) {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Physics::RigidBody3D>(entityId)) return;

        if (auto phys = ecs_.getSystem<PAIN::Physics::System>()) {
            phys->enablePhysics(entityId); 
        }
    }

    /* =========================================================================== */
    /*                                   Audio                                     */
    /* =========================================================================== */
    void EngineAPIAdapter::Audio_Play(entt::entity entityId)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Audio::AudioSource>(entityId)) return;

        auto& src = reg.get<PAIN::Audio::AudioSource>(entityId);
        src.playTrigger = true;  // tells audiosys to start
        src.stopTrigger = false;
    }

    void EngineAPIAdapter::Audio_Stop(entt::entity entityId)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Audio::AudioSource>(entityId)) return;

        auto& src = reg.get<PAIN::Audio::AudioSource>(entityId);
        src.stopTrigger = true;
        src.playTrigger = false;
    }

    void EngineAPIAdapter::Audio_SetVolumeDb(entt::entity entityId, float db)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Audio::AudioSource>(entityId)) return;

        reg.get<PAIN::Audio::AudioSource>(entityId).volumeDb = db;
    }

    void EngineAPIAdapter::Audio_SetGroup(entt::entity entityId, std::string group)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Audio::AudioSource>(entityId)) return;

        reg.get<PAIN::Audio::AudioSource>(entityId).group_name = std::move(group);
    }

    void EngineAPIAdapter::Audio_SetLooping(entt::entity entityId, bool looping)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Audio::AudioSource>(entityId)) return;

        reg.get<PAIN::Audio::AudioSource>(entityId).looping = looping;
    }

    /* =========================================================================== */
    /*                           Scene / System state                              */
    /* =========================================================================== */

    std::string EngineAPIAdapter::GetCurrentSceneName() {
        if (!scene_ || !assets_) return "";
        auto guid = scene_->getCurrScnID();
        if (!guid.IsValid()) return "";
        auto asset = assets_->getAssetData(guid);
        return asset ? asset->main_relative_path.generic_string() : "";
    }

    std::string EngineAPIAdapter::GetPreviousSceneName() {
        if (!scene_ || !assets_) return "";
        auto guid = scene_->getPrevScnID();
        if (!guid.IsValid()) return "";
        auto asset = assets_->getAssetData(guid);
        return asset ? asset->main_relative_path.generic_string() : "";
    }

    bool EngineAPIAdapter::ChangeScene(std::string name) {

        if (!scene_ || !assets_) {
            PN_CORE_WARN("[EngineAPI] Missing SceneManager or Assets::Manager; cannot change scene '{}'", name);
            return false;
        }

        auto scn_opt = assets_->getAssetData(name);

        if (scn_opt) {

            scene_->changeScene(scn_opt.get()->guid);
            //SetGameCamera();
        }

        return true;

        //// Invalid default
        //PAIN::Assets::GUID targetGuid; 

        //// Get asset metadata
        //auto sceneInfos = assets_->getAllAssetDataOfType(PAIN::Assets::Type::Scenes);

        //for (const auto& info : sceneInfos) {
        //    if (info->name == name) {     
        //        // e.g "prototype.scn"
        //        targetGuid = info->guid;
        //        break;
        //    }
        //}

        //if (!targetGuid.IsValid()) {
        //    PN_CORE_WARN("[EngineAPI] No scene GUID found for scene name '{}'", name);

        //    for (const auto& info : sceneInfos) {
        //        PN_CORE_INFO("[EngineAPI] Scene asset: name='{}', guid={}", info->name, info->guid.ToString());
        //    }

        //    return false;
        //}

        //PN_CORE_INFO("[EngineAPI] Changing scene '{}' (GUID {})",
        //    name, targetGuid.ToString());

        //// Preserve play state
        //bool previousSceneState = scene_->isPlaying();

        //// Load next scene
        //scene_->loadScene(targetGuid);
        //
        //// Set scene to play
        //if (previousSceneState) {
        //    scene_->onPlay();
        //}
        //else {
        //    scene_->onStop();
        //}

        //// Set Camera to game camera
        //scene_->SetGameCamera();

    }

    // Expose pausing to lua
    void EngineAPIAdapter::SetGamePaused(bool pause) {
        if (!scene_) {
            PN_CORE_WARN("[EngineAPI] Missing SceneManager.");
            return;
        }

        scene_->setGamePaused(pause);
    }
    // Check if game is paused
    bool EngineAPIAdapter::IsGamePaused() const { 
        if (!scene_) {
            PN_CORE_WARN("[EngineAPI] Missing SceneManager.");
            return false;
        }

        return scene_->isGamePaused();
    }
    float EngineAPIAdapter::GetFps() const { return 0.0f; }
    void EngineAPIAdapter::SetDeltaMultiplier(float) {}
    float EngineAPIAdapter::GetDeltaMultiplier() const { return 1.0f; }

    /* =========================================================================== */
    /*                              Layer Control                                  */
    /* =========================================================================== */

    bool EngineAPIAdapter::SetLayerEnabled(int layerId, bool enabled) {
        if (!scene_) {
            PN_CORE_ERROR("[EngineAPIAdapter] SetLayerEnabled: SceneManager not available");
            return false;
        }

        auto& layers = scene_->getLayers();
        for (auto& layer : layers) {
            if (layer.id == layerId) {
                layer.enabled = enabled;
                PN_CORE_INFO("[EngineAPIAdapter] Set Layer {} enabled: {}", layerId, enabled);
                return true;
            }
        }

        PN_CORE_WARN("[EngineAPIAdapter] Layer {} not found", layerId);
        return false;
    }

    bool EngineAPIAdapter::GetLayerEnabled(int layerId) {
        if (!scene_) {
            PN_CORE_ERROR("[EngineAPIAdapter] GetLayerEnabled: SceneManager not available");
            return false;
        }

        auto& layers = scene_->getLayers();
        for (const auto& layer : layers) {
            if (layer.id == layerId) {
                return layer.enabled;
            }
        }

        PN_CORE_WARN("[EngineAPIAdapter] Layer {} not found", layerId);
        return false;
    }

    /* =========================================================================== */
    /*                         Application / Quit Control                           */
    /* =========================================================================== */
    void EngineAPIAdapter::QuitApplication()
    {
        // Prefer a safe shutdown via the window system when available
        if (window_) {
            PN_CORE_INFO("[EngineAPIAdapter] QuitApplication -> calling Window::safeShutdown()");
            window_->safeShutdown();
            return;
        }

        // Fallback: request quit via global flag (Application::Run checks this)
        PN_CORE_INFO("[EngineAPIAdapter] QuitApplication -> window not available, setting g_shouldQuitApplication");
        PAIN::g_shouldQuitApplication = true;
    }


    /* =========================================================================== */
    /*                              Graphics / FX                                  */
    /* =========================================================================== */
    void EngineAPIAdapter::ShakeCamera(float /*duration*/, float /*amplitude*/) {}

    Camera* EngineAPIAdapter::Camera_GetActive()
    {
        return scene_ ? scene_->GetActiveCamera() : nullptr;
    }

    void EngineAPIAdapter::Camera_SetTransform(const glm::vec3& pos, const glm::vec3& target, const glm::vec3& up)
    {
        if (!scene_) return;
        PAIN::Camera* cam = scene_->GetGameCamera(); // so editor still has camera control
        if (!cam) return;

        cam->pos = pos;
        cam->forward = glm::normalize(target - pos);
        cam->up = glm::normalize(up);
        cam->right = glm::normalize(glm::cross(cam->forward, cam->up));
    }

    std::pair<glm::vec3, glm::vec3> EngineAPIAdapter::GetCameraOffsets(entt::entity entityId)
    {
        if (auto opt = ecs_.getEntityComponent<PAIN::Cam>(entityId)) {
            const auto& cam = opt->get();
            return { cam.trans_offset, cam.rot_offset };
        }

        // default if entity has no camera
        return { glm::vec3(0.f), glm::vec3(0.f) };
    }

    /* =========================================================================== */
    /*                                Particles                                    */
    /* =========================================================================== */
    void EngineAPIAdapter::SpawnParticles(int /*entityId*/, int /*count*/, bool /*ignoreRotation*/) {}

    /* =========================================================================== */
    /*                              Input Helpers                                  */
    /* =========================================================================== */
    void EngineAPIAdapter::Input_OnEvent(PAIN::Event::Event& e)
    {
        using namespace PAIN::Event;

        Dispatcher d{ e };

#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG
        // Skip keyboard input if user is typing in ImGui text field
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantTextInput) {
#endif
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
#ifdef _DEBUG
        }
#endif

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


    void EngineAPIAdapter::HideCursor(bool hidden)
    {
        PN_CORE_ERROR("Setting CURSOR mode to {}", hidden);
        window_->hideCursor(hidden);
    }


    bool EngineAPIAdapter::Input_IsKeyDown(int key) { return keysDown_.count(key) != 0; }
    bool EngineAPIAdapter::Input_WasKeyPressed(int key) { return keysPressed_.count(key) != 0; }
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
    /*                                ModelRenderer                                 */
    /* =========================================================================== */
    std::optional<uint32_t> EngineAPIAdapter::GetMeshId(entt::entity entityId) {
        //PAIN::ModelRenderer* mr = nullptr; 
        //if (!try_get<PAIN::ModelRenderer>(entityId, mr)) return std::nullopt;
        //return mr->mesh_id; 
        return std::nullopt;
    }
    void EngineAPIAdapter::SetMeshId(entt::entity entityId, uint32_t meshId) {
        //auto& mr = ensure<PAIN::ModelRenderer>(entityId);
        //mr.mesh_id = meshId; 

        /*PAIN::ModelRenderer* mr = nullptr;
        if (!try_get<PAIN::ModelRenderer>(entityId, mr)) {
            PN_CORE_ERROR("SetMeshId: Entity {} has no ModelRenderer component", static_cast<entt::id_type>(entt::to_integral(entityId)));
            return;
        }
        mr->mesh_id = meshId;*/
    }

    void EngineAPIAdapter::SetUITexture(entt::entity entityId, const std::string& textureGuidStr)
    {
        if (!assets_) return;

        auto guid = assets_->findByName(textureGuidStr);
        if (!guid.IsValid()) {
            PN_CORE_WARN("[EngineAPIAdapter] SetUITexture: texture '{}' not found", textureGuidStr);
            return;
        }

        auto& reg = ecs_.getRegistry();
        if (!reg.any_of<Texture2D>(entityId)) {
            PN_CORE_WARN("[EngineAPIAdapter] SetUITexture: entity has no Texture2D");
            return;
        }

        auto& tex = reg.get<Texture2D>(entityId);
        tex.texture_guid = guid;
    }

    void EngineAPIAdapter::SetUITextureScale(entt::entity e, glm::vec2 s)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<Texture2D>(e)) return;
        reg.get<Texture2D>(e).texture_scale = s;
    }

    glm::vec2 EngineAPIAdapter::GetUITextureScale(entt::entity e)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<Texture2D>(e)) return { 1.f, 1.f };
        return reg.get<Texture2D>(e).texture_scale;
    }

    /* =========================================================================== */
    /*                                  Lighting                                   */
    /* =========================================================================== */
    bool EngineAPIAdapter::HasLight(entt::entity entityId) {
        PAIN::Lighting* l = nullptr;
        return try_get<PAIN::Lighting>(entityId, l);
    }
    void EngineAPIAdapter::AddLight(entt::entity entityId) {
        (void)ensure<PAIN::Lighting>(entityId);
    }
    void EngineAPIAdapter::RemoveLight(entt::entity entityId) {
        ecs_.removeEntityComponent<PAIN::Lighting>(entityId);
    }
    void EngineAPIAdapter::SetLightPosition(entt::entity entityId, float x, float y, float z) {
        auto& l = ensure<PAIN::Lighting>(entityId);
        l.offset = { x,y,z };
    }
    void EngineAPIAdapter::SetLightIntensity(entt::entity entityId, float r, float g, float b) {
        auto& l = ensure<PAIN::Lighting>(entityId);
        l.light_intensity = { r,g,b };
    }
    void EngineAPIAdapter::SetLightType(entt::entity entityId, int ty) {
        auto& l = ensure<PAIN::Lighting>(entityId);
        l.light_type = static_cast<PAIN::TYPES>(ty);
    }
    void EngineAPIAdapter::SetLightDirection(entt::entity entityId, float x, float y, float z) {
        auto& l = ensure<PAIN::Lighting>(entityId);
        l.direction = glm::normalize(glm::vec3{ x,y,z });
    }
    void EngineAPIAdapter::SetShadowType(entt::entity entityId, int st) {
        auto& l = ensure<PAIN::Lighting>(entityId);
        l.shadow_type = static_cast<PAIN::SHADOW_TYPES>(st);

    }

    /* =========================================================================== */
    /*                                  Animation                                  */
    /* =========================================================================== */

    void EngineAPIAdapter::Animation_Play(entt::entity entityId, std::string animName)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Animation>(entityId)) return;

        int idx = FindAnimIndex(ecs_, entityId, animName);
        if (idx != -1) {
            auto& anim = reg.get<PAIN::Animation>(entityId);

            // If already playing this animation, do nothing (or reset if you prefer)
            if (anim.currentAnimationIndex == idx && anim.isPlaying) return;

            anim.currentAnimationIndex = idx;
            anim.animationTime = 0.0f;
            anim.isPlaying = true;

            // Clear any transition
            anim.nextAnimationIndex = -1;
            anim.transitionWeight = 0.0f;
        }
        else {
            PN_CORE_WARN("[EngineAPI] Animation_Play: Animation '{}' not found on entity {}", animName, (uint32_t)entityId);
        }
    }

    void EngineAPIAdapter::Animation_CrossFade(entt::entity entityId, std::string animName, float duration)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Animation>(entityId)) return;

        int idx = FindAnimIndex(ecs_, entityId, animName);

        if (idx == -1) {
            PN_CORE_WARN("[EngineAPI] Animation_CrossFade: Animation '{}' not found on entity {}", animName, (uint32_t)entityId);
            return;
        }

        auto& anim = reg.get<PAIN::Animation>(entityId);

        // Only crossfade if different
        if (anim.currentAnimationIndex == idx && anim.nextAnimationIndex == -1) {
            return;
        }

        // If ALREADY transitioning to this animation, let it finish.
        if (anim.nextAnimationIndex == idx) {
            return; 
        }

        anim.nextAnimationIndex = idx;
        anim.transitionDuration = duration > 0.0f ? duration : 0.1f; 
        anim.transitionWeight = 0.0f;
        anim.isPlaying = true;

    }

    void EngineAPIAdapter::Animation_SetSpeed(entt::entity entityId, float speed)
    {
        auto& reg = ecs_.getRegistry();
        if (reg.all_of<PAIN::Animation>(entityId)) {
            reg.get<PAIN::Animation>(entityId).playbackSpeed = speed;
        }
    }

    void EngineAPIAdapter::Animation_SetLoop(entt::entity entityId, bool loop)
    {
        auto& reg = ecs_.getRegistry();
        if (reg.all_of<PAIN::Animation>(entityId)) {
            reg.get<PAIN::Animation>(entityId).loopAnimation = loop;
        }
    }

    bool EngineAPIAdapter::Animation_IsPlaying(entt::entity entityId, std::string animName)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Animation>(entityId)) return false;

        const auto& anim = reg.get<PAIN::Animation>(entityId);
        int idx = FindAnimIndex(ecs_, entityId, animName);

        // Check if current is the anim AND it's playing
        // OR check if we are transitioning TO it
        return (anim.currentAnimationIndex == idx && anim.isPlaying) || (anim.nextAnimationIndex == idx);
    }
    float EngineAPIAdapter::GetAnimationTime(entt::entity entityId)
    {
        auto& reg = ecs_.getRegistry();
        if (reg.all_of<PAIN::Animation>(entityId)) {
            return reg.get<PAIN::Animation>(entityId).animationTime;
        }

        return 0.0f;
    }
    float EngineAPIAdapter::GetAnimationDuration(entt::entity entityId)
    {
        auto& reg = ecs_.getRegistry();
        if (reg.all_of<PAIN::Animation>(entityId)) {
            return reg.get<PAIN::Animation>(entityId).animationDuration;
        }

        return 0.0f;
    }
    bool EngineAPIAdapter::Animation_HasFinished(entt::entity entityId)
    {
        auto& reg = ecs_.getRegistry();
        if (!reg.all_of<PAIN::Animation>(entityId)) return false;

        const auto& anim = reg.get<PAIN::Animation>(entityId);

        // Looping animations never "finish"
        if (anim.loopAnimation) return false;

        // 2. Safety check against uninitialized animations
        if (anim.animationDuration<= 0.001f) return false;

        // 3. Check if current time has reached or exceeded the total duration
        // We use a small epsilon (0.05f) to be safe, or just direct comparison
        return anim.animationTime >= (anim.animationDuration - 0.01f);

        return false;
    }
    void EngineAPIAdapter::SetAnimationTime(entt::entity entityId, float time)
    {
        auto& reg = ecs_.getRegistry();
        if (reg.all_of<PAIN::Animation>(entityId)) {
            reg.get<PAIN::Animation>(entityId).animationTime = time;
        }
    }
}