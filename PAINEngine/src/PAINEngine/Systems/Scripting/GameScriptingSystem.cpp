#include "pch.h"
#include "GameScriptingSystem.h"
#include "PAINEngine/CoreSystems/Scripting/EngineAPIAdapter.h"
#include "PAINEngine/ECS/Controller.h"
#include "PAINEngine/ECS/sMetaData.h"
#include "PAINEngine/CoreSystems/Audio/Audio.h"
#include "PAINEngine/CoreSystems/Path/Path.h"
#include "PAINEngine/CoreSystems/Assets/sAssets.h"

#ifdef PN_PLATFORM_ANDROID
#include "PAINEngine/CoreSystems/Events/Android/TouchEvents.h"
#include "PAINEngine/CoreSystems/Events/Android/AppEvents.h"
#include "PAINEngine/CoreSystems/Events/Android/FocusEvents.h"
#include "PAINEngine/CoreSystems/Events/Android/OtherEvents.h"
#include "PAINEngine/CoreSystems/Events/Android/SurfaceEvents.h"
#endif

#ifdef PN_PLATFORM_WINDOWS
#include "PAINEngine/CoreSystems/Events/GLFW/KeyEvents.h"
#include "PAINEngine/CoreSystems/Events/GLFW/MouseEvents.h"
#include "PAINEngine/CoreSystems/Events/GLFW/WindowEvents.h"
#endif

namespace PAIN {

    void GameScriptingSystem::onAttach() {
        PN_CORE_INFO("[GameScriptingSystem] Attaching...");

        // Get required services
        auto ecs_ptr = services->get<ECS::Controller>();
        auto meta_ptr = services->get<MetaData::Service>();
        auto assets_ptr = services->get<Assets::Manager>();
        //auto audio_ptr = services->get<Audio::Audio>();
        auto path_ptr = services->get<Path::Path>();

        // Validate all services
        if (!ecs_ptr || !meta_ptr) {
            PN_CORE_ERROR("[GameScriptingSystem] Required services not available!");
            return;
        }

        // Create adapter and pass directly to Lua 
        auto adapter = std::make_shared<EngineAPIAdapter>(
            *ecs_ptr,
            *meta_ptr,
            assets_ptr.get(),
           // audio_ptr.get(),
            path_ptr.get()
        );

        // Initialize Lua with the adapter
#ifdef _DEBUG
        bool shipping = false;
#else
        bool shipping = true;
#endif

        luaManager_.init(adapter, shipping);
        PN_CORE_INFO("[GameScriptingSystem] Lua initialized");
    }

    void GameScriptingSystem::onDetach() {
        PN_CORE_INFO("[GameScriptingSystem] Detaching...");
        luaManager_.onDetach();
    }

    void GameScriptingSystem::onUpdate(AppTiming timing) {
        luaManager_.tick(timing.dt);
        luaManager_.Input_EndFrame();

    }

    void GameScriptingSystem::onEvent(Event::Event& e) {
        using namespace Event;
        Dispatcher d{ e };

        luaManager_.Input_OnEvent(e); // foward all events to luamanger

#ifdef PN_PLATFORM_WINDOWS
        d.Dispatch<KeyPressed>([&](KeyPressed& ev) {
            std::string keyName = getKeyName(ev.getKeyCode());  // trigger Lua key callbacks
            luaManager_.onKeyDown(keyName);
            return false;
            });
        d.Dispatch<KeyReleased>([&](KeyReleased& ev) {
            std::string keyName = getKeyName(ev.getKeyCode());
            luaManager_.onKeyUp(keyName);
            return false;
            });
        d.Dispatch<MouseBtnPressed>([&](MouseBtnPressed&) {
            luaManager_.onClick();
            return false;
            });
        d.Dispatch<WindowFocused>([&](WindowFocused&) {
            return false;
            });
#endif

#ifdef PN_PLATFORM_ANDROID
        d.Dispatch<TouchDown>([&](TouchDown&) {
            luaManager_.onClick();
            return false;
            });
        d.Dispatch<TouchUp>([&](TouchUp&) {
            return false;
            });
        d.Dispatch<AppPause>([&](AppPause&) {
            luaManager_.onPauseChanged(true);
            return false;
            });
        d.Dispatch<AppResume>([&](AppResume&) {
            luaManager_.onPauseChanged(false);
            return false;
            });
#endif
    }

    void GameScriptingSystem::attachScript(int entityId, const std::string& scriptPath) {
        luaManager_.loadScriptForEntity(entityId, scriptPath, {}, false);
        PN_CORE_TRACE("[GameScriptingSystem] Script '{}' attached to entity {}",
            scriptPath, entityId);
    }

    void GameScriptingSystem::attachScriptWithVars(
        int entityId,
        const std::string& scriptPath,
        const std::vector<ScriptExternalVar>& vars)
    {
        luaManager_.loadScriptForEntity(entityId, scriptPath, vars, false);
    }

    void GameScriptingSystem::onCollision(int entityA, int entityB) {
        luaManager_.onCollision(entityA, entityB);
    }

    void GameScriptingSystem::setPathService(PAIN::Path::Path* fs)
    {
        luaManager_.setPathService(fs);
    }

    bool GameScriptingSystem::loadAllScriptsForEntityFromVDir(int entityId, const std::string& alias,
                                                              const std::string& relativeRoot, bool recursive,
                                                              bool runWhenPaused)
    {
        return luaManager_.loadAllScriptsForEntityFromVDir(entityId, alias, relativeRoot, recursive, runWhenPaused);
    }

    std::string GameScriptingSystem::getKeyName(int keyCode) const {
        
        switch (keyCode) {
#ifdef PN_PLATFORM_WINDOWS
            case GLFW_KEY_SPACE: return "SPACE";
            case GLFW_KEY_ENTER: return "ENTER";

            case GLFW_KEY_A: return "A";
            case GLFW_KEY_D: return "D";
            case GLFW_KEY_S: return "S";
            case GLFW_KEY_W: return "W";

            case GLFW_KEY_C: return "C";
            case GLFW_KEY_E: return "E";
#endif

#ifdef PN_PLATFORM_ANDROID
            //  raw key codes no glfw for android
            case 32: return "SPACE";
            case 257: return "ENTER";

            case 65: return "A";
            case 68: return "D";
            case 83: return "S";
            case 87: return "W";

#endif

            default: return std::to_string(keyCode);
        }
    }

}  // namespace PAIN



//namespace PAIN {
//
//    void GameScriptingSystem::onAttach() {
//        PN_CORE_INFO("[GameScriptingSystem] Attaching...");
//
//        // Get required services from the shared services map
//        auto& ecs = *services->get<ECS::Controller>();
//        auto& meta = *services->get<MetaData::Service>();
//        auto* assets = services->get<Assets::Manager>().get();
//        //auto* audio = services->get<Audio::Audio>().get();
//        auto* pathSvc = services->get<Path::Path>().get();
//
//        // Create production engine wiring
//        engineWiring_ = Scripting::CreateProductionEngine(ecs, meta, assets, //audio, 
//                                                          pathSvc);
//
//        // Initialize Lua with production API
//#ifdef _DEBUG
//        bool shipping = false;  // Dev builds have debug tools
//#else
//        bool shipping = true;   // Release builds are sandboxed
//#endif
//
//        luaManager_.init(engineWiring_->api.get(), shipping);
//
//        PN_CORE_INFO("[GameScriptingSystem] Production engine initialized");
//    }
//
//    void GameScriptingSystem::onDetach() {
//        PN_CORE_INFO("[GameScriptingSystem] Detaching...");
//        // Lua manager and engine wiring will clean up automatically
//    }
//
//    void GameScriptingSystem::onUpdate(AppTiming timing) {
//        // Update all Lua scripts
//        luaManager_.tick(timing.dt);
//
//        // Clear one-frame input events
//        if (engineWiring_ && engineWiring_->api) {
//            engineWiring_->api->Input_EndFrame();
//        }
//    }
//
//    void GameScriptingSystem::onEvent(Event::Event& e) {
//        using namespace Event;
//
//        // Feed event to EngineAPIAdapter for input tracking
//        if (engineWiring_ && engineWiring_->api) {
//            engineWiring_->api->Input_OnEvent(e);
//        }
//
//        // Dispatch to Lua callbacks
//        Dispatcher d{ e };
//
//#ifdef PN_PLATFORM_WINDOWS
//        d.Dispatch<KeyPressed>([&](KeyPressed& ev) {
//            luaManager_.onKeyDown(std::to_string(ev.getKeyCode()));
//            return false;
//            });
//
//        d.Dispatch<KeyReleased>([&](KeyReleased& ev) {
//            luaManager_.onKeyUp(std::to_string(ev.getKeyCode()));
//            return false;
//            });
//
//        d.Dispatch<MouseBtnPressed>([&](MouseBtnPressed&) {
//            luaManager_.onClick();
//            return false;
//            });
//#endif
//
//#ifdef PN_PLATFORM_ANDROID
//        d.Dispatch<TouchDown>([&](TouchDown&) {
//            luaManager_.onClick();  // Treat touch as click
//            return false;
//            });
//
//        d.Dispatch<AppPause>([&](AppPause&) {
//            luaManager_.onPauseChanged(true);
//            return false;
//            });
//
//        d.Dispatch<AppResume>([&](AppResume&) {
//            luaManager_.onPauseChanged(false);
//            return false;
//            });
//#endif
//    }
//
//    void GameScriptingSystem::attachScript(int entityId, const std::string& scriptPath) {
//        luaManager_.loadScriptForEntity(entityId, scriptPath, {}, false);
//        PN_CORE_TRACE("[GameScriptingSystem] Attached script '{}' to entity {}", scriptPath, entityId);
//    }
//
//    void GameScriptingSystem::attachScriptWithVars(
//        int entityId,
//        const std::string& scriptPath,
//        const std::vector<ScriptExternalVar>& vars)
//    {
//        luaManager_.loadScriptForEntity(entityId, scriptPath, vars, false);
//        PN_CORE_TRACE("[GameScriptingSystem] Attached script '{}' to entity {} with {} vars",
//            scriptPath, entityId, vars.size());
//    }
//
//    void GameScriptingSystem::onCollision(int entityA, int entityB) {
//        luaManager_.onCollision(entityA, entityB);
//    }
//
//}  // namespace PAIN
