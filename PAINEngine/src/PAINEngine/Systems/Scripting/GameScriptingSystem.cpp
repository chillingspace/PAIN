#include "pch.h"
#include "GameScriptingSystem.h"
#include "PAINEngine/CoreSystems/Scripting/EngineAPIAdapter.h"
#include "PAINEngine/ECS/Controller.h"
#include "PAINEngine/ECS/sMetaData.h"
#include "PAINEngine/ECS/Components/cScript.h"
#include "PAINEngine/CoreSystems/Audio/Audio.h"
#include "PAINEngine/CoreSystems/Path/Path.h"
#include "PAINEngine/CoreSystems/Assets/sAssets.h"
#include "PAINEngine/CoreSystems/Scene/Scene.h"
#include "PAINEngine/CoreSystems/Scene/Camera.h"
#include "PAINEngine/CoreSystems/Windows/Window.h"

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
#ifdef _DEBUG
#include <imgui.h>
#endif
#endif

namespace PAIN {

    namespace Scripting {

        void GameScriptingSystem::onAttach() {
            ensureInit();
        }

        void GameScriptingSystem::onDetach() {
            PN_CORE_INFO("[GameScriptingSystem] Detaching...");
            luaManager_.onDetach();
        }

        GameScriptingSystem::GameScriptingSystem(std::shared_ptr<Services> svc) : ISystem(svc)
        {

        }

        GameScriptingSystem::~GameScriptingSystem()
        {

        }

        void GameScriptingSystem::onUpdate(AppTiming timing, entt::registry& reg) {
            ensureInit();
            if (!init_) return;

            auto services_ptr = getServices();
            auto scene = services_ptr->get<Scene::SceneManager>();

            #ifdef _DEBUG
            if (scene) {
                if (!scene->isPlaying()) { // If scene isn't playing don't run any scripts
                    return;
                }
            }
            #endif

            // --- publish LuaManager* into this registry's context once ---
            {
                auto& ctx = reg.ctx();
                if (!ctx.contains<PAIN::LuaManager*>()) {
                    ctx.emplace<PAIN::LuaManager*>(&luaManager_);
                }
            }


            auto assets = services_ptr->get<Assets::Manager>();
            auto fs = services_ptr->get<Path::Path>();

            auto view = reg.view<PAIN::Scripts>();

            for (auto e : view) {
                auto& scriptsComp = view.get<PAIN::Scripts>(e);

                if (!assets || !fs)
                    continue;

                for (auto& sc : scriptsComp.scripts) {
                    // per-script gatekeeping
                    if (sc.loaded || !sc.enabled)
                        continue;
                    if (!sc.script_asset.IsValid())
                        continue;

                    // look up asset metadata
                    auto meta = assets->getAssetData(sc.script_asset);
                    if (!meta)
                        continue;

                    std::string vpath =
                        fs->aliasCombineRelative("assets", meta->shipped_relative_path.string());
                    std::string realPath = fs->resolvePath(vpath);

                   /* PN_CORE_INFO("[GameScriptingSystem] Loading script '{}' for entity {} (entt::entity={})",
                        meta->name,
                        static_cast<uint32_t>(e),
                        entt::to_integral(e));*/

                    // attach this script to this entity
                    attachScript(e, realPath);

                    // mark this particular script as loaded
                    sc.loaded = true;
                }
            }

            luaManager_.tick(timing.dt, timing.unscaled_dt);
            luaManager_.Input_EndFrame();

        }

        void GameScriptingSystem::onEvent(Event::Event& e) {
            ensureInit();
            if (!init_) return;

            //PN_CORE_INFO("[GSS] onEvent hit. type={} category={}", (int)e.getType(), (int)e.getCategoryFlags());
            using namespace Event;
            Dispatcher d{ e };

#ifdef PN_PLATFORM_WINDOWS
#ifdef _DEBUG
            // Skip keyboard input if user is typing in ImGui text field (WantTextInput)
            ImGuiIO& io = ImGui::GetIO();
            bool skipKeyboard = io.WantTextInput;
#else
            bool skipKeyboard = false;
#endif

            if (!skipKeyboard) {
                luaManager_.Input_OnEvent(e); // forward keyboard events to luamanager only when not captured by editor
                d.Dispatch<KeyPressed>([&](KeyPressed& ev) {
                    //PN_CORE_INFO("[GSS] KeyPressed code={}", ev.getKeyCode());
                    if (auto name = getKeyName(ev.getKeyCode())) {
                        //PN_CORE_INFO("[GSS] KeyDown -> {}", *name);
                        luaManager_.onKeyDown(*name);
                    }
                    return false;
                    });
                d.Dispatch<KeyReleased>([&](KeyReleased& ev) {
                    //PN_CORE_INFO("[GSS] KeyReleased code={}", ev.getKeyCode());
                    if (auto name = getKeyName(ev.getKeyCode())) {
                        luaManager_.onKeyUp(*name);
                    }
                    return false;
                    });
            } // end if (!skipKeyboard)
            d.Dispatch<MouseBtnPressed>([&](MouseBtnPressed&) {
                luaManager_.onClick();
                return false;
                });
            d.Dispatch<WindowFocused>([&](WindowFocused&) {
                return false;
                });
#endif

#ifdef PN_PLATFORM_ANDROID
            luaManager_.Input_OnEvent(e); // forward all events to luamanager on Android
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

        void GameScriptingSystem::attachScript(entt::entity entity, const std::string& scriptPath) {
            int entityId = (int)entt::to_integral(entity);
            luaManager_.loadScriptForEntity(entity, scriptPath);
        }

        void GameScriptingSystem::attachScriptWithVars(
            entt::entity entity,
            const std::string& scriptPath,
            const std::vector<ScriptExternalVar>& vars)
        {
            luaManager_.loadScriptForEntity(entity, scriptPath, vars, false);
        }

        void GameScriptingSystem::onCollision(entt::entity entityA, entt::entity entityB) {
            //PN_CORE_INFO("[Script] onCollision forwarded to LuaManager (a={} b={})", (uint32_t)entityA, (uint32_t)entityB);
            luaManager_.onCollision(entityA, entityB);
        }

        void GameScriptingSystem::setPathService(PAIN::Path::Path* fs)
        {
            luaManager_.setPathService(fs);
        }

        bool GameScriptingSystem::loadAllScriptsForEntityFromVDir(entt::entity entity, const std::string& alias,
            const std::string& relativeRoot, bool recursive,
            bool runWhenPaused)
        {
            return luaManager_.loadAllScriptsForEntityFromVDir(entity, alias, relativeRoot, recursive, runWhenPaused);
        }

        std::optional<std::string> GameScriptingSystem::getKeyName(int keyCode) const {

            switch (keyCode) {
#ifdef PN_PLATFORM_WINDOWS
            // Special keys
            case GLFW_KEY_SPACE: return "SPACE";
            case GLFW_KEY_ENTER: return "ENTER";
            case GLFW_KEY_TAB:   return "TAB";
            case GLFW_KEY_ESCAPE: return "ESCAPE";

            // All alpha keys A-Z
            case GLFW_KEY_A: return "A";
            case GLFW_KEY_B: return "B";
            case GLFW_KEY_C: return "C";
            case GLFW_KEY_D: return "D";
            case GLFW_KEY_E: return "E";
            case GLFW_KEY_F: return "F";
            case GLFW_KEY_G: return "G";
            case GLFW_KEY_H: return "H";
            case GLFW_KEY_I: return "I";
            case GLFW_KEY_J: return "J";
            case GLFW_KEY_K: return "K";
            case GLFW_KEY_L: return "L";
            case GLFW_KEY_M: return "M";
            case GLFW_KEY_N: return "N";
            case GLFW_KEY_O: return "O";
            case GLFW_KEY_P: return "P";
            case GLFW_KEY_Q: return "Q";
            case GLFW_KEY_R: return "R";
            case GLFW_KEY_S: return "S";
            case GLFW_KEY_T: return "T";
            case GLFW_KEY_U: return "U";
            case GLFW_KEY_V: return "V";
            case GLFW_KEY_W: return "W";
            case GLFW_KEY_X: return "X";
            case GLFW_KEY_Y: return "Y";
            case GLFW_KEY_Z: return "Z";

            // Number keys
            case GLFW_KEY_0: return "0";
            case GLFW_KEY_1: return "1";
            case GLFW_KEY_2: return "2";
            case GLFW_KEY_3: return "3";
            case GLFW_KEY_4: return "4";
            case GLFW_KEY_5: return "5";
            case GLFW_KEY_6: return "6";
            case GLFW_KEY_7: return "7";
            case GLFW_KEY_8: return "8";
            case GLFW_KEY_9: return "9";

            // Arrow keys
            case GLFW_KEY_RIGHT: return "KEY_R";
            case GLFW_KEY_LEFT: return "KEY_L";
            case GLFW_KEY_DOWN: return "KEY_D";
            case GLFW_KEY_UP: return "KEY_U";

            // Shift keys
            case GLFW_KEY_LEFT_SHIFT: return "LSHIFT";
            case GLFW_KEY_RIGHT_SHIFT: return "RSHIFT";
            case GLFW_KEY_LEFT_CONTROL: return "LCTRL";
            case GLFW_KEY_RIGHT_CONTROL: return "RCTRL";
#endif

#ifdef PN_PLATFORM_ANDROID
                //  raw key codes no glfw for android
            case 32: return "SPACE";
            case 257: return "ENTER";

            case 65: return "A";
            case 66: return "B";
            case 67: return "C";
            case 68: return "D";
            case 69: return "E";
            case 70: return "F";
            case 71: return "G";
            case 72: return "H";
            case 73: return "I";
            case 74: return "J";
            case 75: return "K";
            case 76: return "L";
            case 77: return "M";
            case 78: return "N";
            case 79: return "O";
            case 80: return "P";
            case 81: return "Q";
            case 82: return "R";
            case 83: return "S";
            case 84: return "T";
            case 85: return "U";
            case 86: return "V";
            case 87: return "W";
            case 88: return "X";
            case 89: return "Y";
            case 90: return "Z";

            case 48: return "0";
            case 49: return "1";
            case 50: return "2";
            case 51: return "3";
            case 52: return "4";
            case 53: return "5";
            case 54: return "6";
            case 55: return "7";
            case 56: return "8";
            case 57: return "9";

            case 262: return "KEY_R";
            case 263: return "KEY_L";
            case 264: return "KEY_D";
            case 265: return "KEY_U";

#endif

            default: return std::to_string(keyCode);
            }
        }

        void GameScriptingSystem::ensureInit()
        {
            if (init_) return;
            init_ = true;

            PN_CORE_INFO("[GameScriptingSystem] Attaching...");

            auto services_ptr = getServices();
            auto ecs_ptr = services_ptr->get<ECS::Controller>();
            auto meta_ptr = services_ptr->get<MetaData::Service>();
            auto assets_ptr = services_ptr->get<Assets::Manager>();
            auto path_ptr = services_ptr->get<Path::Path>();
            auto scene_ptr = services_ptr->get<Scene::SceneManager>();
            
            if (!ecs_ptr || !meta_ptr) {
                PN_CORE_ERROR("[GameScriptingSystem] Required services not available!");
                return;
            }

            // Provide optional window pointer so adapter can request safeShutdown
            auto window_ptr = services_ptr->get<Window::Window>();
            auto adapter = std::make_shared<EngineAPIAdapter>(
                *ecs_ptr,
                *meta_ptr,
                assets_ptr.get(),
                path_ptr.get(),
                scene_ptr.get(),
                window_ptr ? window_ptr.get() : nullptr
            );

#ifdef _DEBUG
            bool shipping = false;
#else
            bool shipping = true;
#endif

            luaManager_.init(adapter, shipping);
            luaManager_.setServices(services_ptr.get());  // Enable direct audio playback functions
            PN_CORE_INFO("[GameScriptingSystem] Lua initialized");
        }

    }
}  // namespace PAIN

