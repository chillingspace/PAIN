#include "luaManager.h"
#include "IEngineAPI.h"
#include "PAINEngine/CoreSystems/Audio/Audio.h"
#include "Systems/Audio/sysAudio.h"
#include "ECS/Components/cEntity.h"
#include "ECS/Components/cAudioSource.h"
#include "ECS/Controller.h"
#include "Utility/Log.h"

#include <fstream>
#include <chrono>
#include <deque>
#include <queue>
#include <string_view>

#ifndef SCRIPT_SHIPPING_SANDBOX
#define SCRIPT_SHIPPING_SANDBOX 1
#endif

#ifndef SCRIPT_ENABLE_DEBUG_TOOLS
#define SCRIPT_ENABLE_DEBUG_TOOLS 0
#endif

// -------- Android asset reading --------
#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
// Provide this from your Android app glue:
extern AAssetManager* g_AssetMgr; // set it once during app init
#endif

// ============================================================================
// Small utilities
// ============================================================================
namespace {

    inline double nowSeconds() {
        using clock = std::chrono::steady_clock;
        return std::chrono::duration<double>(clock::now().time_since_epoch()).count();
    }

    inline void logError(std::string_view ctx, const sol::error& e) {
        PN_ERROR("[LuaManager] {} error: {}", ctx, e.what());
    }

    inline void logMsg(std::string_view msg) {
        PN_INFO("[LuaManager] {}", msg);
    }

#ifdef __ANDROID__
    static std::string readAssetText(AAssetManager* mgr, const char* path) {
        if (!mgr) return {};
        AAsset* a = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
        if (!a) return {};
        const void* buf = AAsset_getBuffer(a);
        size_t len = AAsset_getLength(a);
        std::string out((const char*)buf, (const char*)buf + len);
        AAsset_close(a);
        return out;
    }
#endif

    static std::string readFileText(const std::string& path) {
#ifdef __ANDROID__
        if (g_AssetMgr) { // falls back to reading from asset manager if avail
            return readAssetText(g_AssetMgr, path.c_str());
        }
#endif
        //PN_INFO("Attempting to read file: {}", path);

        std::string normalizedPath = path;
        bool isAbsoluteWin = normalizedPath.size() > 1 && normalizedPath[1] == ':'; 
        bool isAbsoluteUnix = !normalizedPath.empty() &&
            (normalizedPath[0] == '/' || normalizedPath[0] == '\\'); 

        if (!isAbsoluteWin && !isAbsoluteUnix &&
            normalizedPath.rfind("assets/", 0) != 0 &&
            normalizedPath.rfind("assets\\", 0) != 0) {
            normalizedPath = "assets/" + path;
        }

        std::ifstream f(normalizedPath, std::ios::binary);

        if (!f.is_open()) {
            PN_ERROR("File not found at: {}", normalizedPath);
            PN_ERROR("Current working directory: {}", std::filesystem::current_path().string());  
            return "";
        }

        if (!f) return {};
        return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }

    static bool runLuaInEnv(sol::state& L, 
                            const std::string& code,
                            sol::environment& env, 
                            std::string_view debugName) { // loads lua chunk safely into given enviro
        sol::load_result lr = L.load(code);
        if (!lr.valid()) { sol::error e = lr; logError("Load(" + std::string(debugName) + ")", e); return false; }
        
        //sol::protected_function_result r = lr(env);   // pass env as argument


        
        //sol::protected_function pfunc = lr;
        //sol::set_environment(env, pfunc);  // actually sets the functions environment
        //sol::protected_function_result r = pfunc();  // executes with entityId in scope
        //
        //if (!r.valid()) {
        //    sol::error e = r;
        //    logError("Run(" + std::string(debugName) + ")", e);
        //    return false;
        //}


        sol::protected_function pfunc = lr;
        sol::set_environment(env, pfunc);  // make 'env' the environment
        sol::protected_function_result r = pfunc(); // run with that environment       
        if (!r.valid()) {
            sol::error e = r;
            logError("Run(" + std::string(debugName) + ")", e);
            return false;
        }

        return true;
    }

    // Collect virtual paths to *.lua 
    static void collectLuaVPaths(PAIN::Path::Path& fs,
        const std::string& vdir,
        bool recursive,
        std::vector<std::string>& out)
    {
        // files directly in this folder with .lua
        auto files = fs.listFiles(vdir, /*filter*/"", /*extension*/".lua");
        out.insert(out.end(), files.begin(), files.end());

        if (!recursive) return;

        // explore subfolders
        auto subs = fs.listDirectories(vdir);
        for (const auto& sub : subs) {
            collectLuaVPaths(fs, sub, true, out);
        }
    }

} // namespace

// ============================================================================
// LuaManager
// ============================================================================

namespace PAIN {

    void LuaManager::init(std::shared_ptr<IEngineAPI> api, bool shipping) {  // after init, script can then call for eg registerUpdate function coz exist in lua global table
        api_ = api;
        shipping_ = shipping;

        openLibs(shipping_); // choose which lua std libs are avail
        bindUsertypes(); // expose c++ types to lua
        bindRegistration(); // expose registration functions
        bindEngineAPI(); // expose engine actions

        // helper printlog from lua prints to stdout
        lua_["printLog"] = [](const std::string& s) {
            PN_INFO("[Lua] {}", s);
            };
    }

    bool LuaManager::loadAllScriptsForEntityFromVDir(entt::entity entity, const std::string& alias, const std::string& relativeRoot, bool recursive, bool runWhenPaused)
    {
        if (!fs_) {
            PN_ERROR("[LuaManager] Path service not set. Call setPathService() first");
            return false;
        }

        const std::string rootVPath = fs_->aliasCombineRelative(alias, relativeRoot);
        if (!fs_->pathExists(rootVPath)) {
            PN_INFO("[LuaManager] no scripts at {}", rootVPath);
            return true;
        }

        std::vector<std::string> vpaths;
        collectLuaVPaths(*fs_, rootVPath, recursive, vpaths);
        std::sort(vpaths.begin(), vpaths.end());
        vpaths.erase(std::unique(vpaths.begin(), vpaths.end()), vpaths.end());

        bool allOk = true;
        for (const auto& vpath : vpaths) {
            // resolve to a readable path for our existing loader
            const std::string real = fs_->resolvePath(vpath);
            allOk &= loadScriptForEntity(entity, real, {}, runWhenPaused);
        }
        return allOk;
    }

    void LuaManager::Input_OnEvent(PAIN::Event::Event& e) {
        if (api_) {
            api_->Input_OnEvent(e); // forward to the adapter which tracks the state
        }
    }

    void LuaManager::Input_EndFrame() {
        if (api_) {
            api_->Input_EndFrame();
        }
    }

    void LuaManager::onDetach() {
        // clear script environments, timers, and callback vectors
        resetForSceneReload();

        api_.reset();

        lua_ = sol::state();
    }

    void LuaManager::resetForSceneReload() {
        PN_CORE_INFO("[LuaManager::resetForSceneReload] Starting reset...");

        PN_CORE_INFO("[LuaManager::resetForSceneReload] Clearing {} update callbacks", updates_.size());
        updates_.clear();
        PN_CORE_INFO("[LuaManager::resetForSceneReload] Clearing key handlers");
        keyDown_.clear();
        keyUp_.clear();
        onClick_.clear();
        PN_CORE_INFO("[LuaManager::resetForSceneReload] Clearing {} collision handlers", onCollision_.size());
        onCollision_.clear();
        pauseHandlers_.clear();
        timeouts_.clear();

        inputQueue_.clear();
        collisionQueue_.clear();
        mouseInOut_.clear();
        collisionInterests_.clear();
        delayedOps_.clear();
        while (!timeoutHeap_.empty()) timeoutHeap_.pop();

        pendingSceneChange_.reset();

        // clear cached entity references directly from lua globals
        PN_CORE_INFO("[LuaManager::resetForSceneReload] Clearing Lua global cached entities...");

        // clear known globals that cache entity state
        lua_["PlayerEntity"] = sol::nil;
        lua_["PlayerState"] = sol::nil;
        lua_["DetectionUI"] = sol::nil;
        lua_["CameraState"] = sol::nil;
        lua_["gamePaused"] = sol::nil;
        lua_["PlayerInput"] = sol::nil;

        // clear the _G.UI table if it exists
        sol::object ui_obj = lua_["UI"];
        if (ui_obj.valid()) {
            lua_["UI"] = sol::nil;
        }

        PN_CORE_INFO("[LuaManager::resetForSceneReload] Cleared Lua global cached values");
        PN_CORE_INFO("[LuaManager::resetForSceneReload] Reset complete");
    }


    void LuaManager::setPrefabInstantiator(std::function<entt::entity(const std::string& prefab, const std::string& layer, const std::string& name)> fn) {
        instantiatePrefab_ = std::move(fn);
    }


    void LuaManager::openLibs(bool shipping) { // decides which lua standard lib to open
#if SCRIPT_SHIPPING_SANDBOX
        (void)shipping; // always sandbox in shipping
        lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::utf8);
#ifndef NDEBUG
        lua_.open_libraries(sol::lib::debug);
#endif
        // DO NOT open: io, os, package, ffi (keeps runtime safe on Android/shipping)
#else
        // Dev mode can open more, still prefer to keep it tight
        lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string,
            sol::lib::table, sol::lib::utf8
#ifndef NDEBUG
            , sol::lib::debug
#endif
        );
        if (!shipping) {
            // Optional in dev only:
            lua_.open_libraries(sol::lib::package);
            // Avoid io/os unless really need them:
            // lua_.open_libraries(sol::lib::io, sol::lib::os);
        }
#endif
    }

    void LuaManager::bindUsertypes() {
        // expose glm::vec2 to lua with getter/setters coz android dont like glm types
        lua_.new_usertype<glm::vec2>("vec2",
            sol::constructors<glm::vec2(), glm::vec2(float, float)>(),
            "x", sol::property(
                [](const glm::vec2& v) { return v.x; },
                [](glm::vec2& v, float x) { v.x = x; }
            ),
            "y", sol::property(
                [](const glm::vec2& v) { return v.y; },
                [](glm::vec2& v, float y) { v.y = y; }
            )
        );

        // expose gom::vec3
        lua_.new_usertype<glm::vec3>("vec3",
            sol::constructors<glm::vec3(), glm::vec3(float, float, float)>(),
            "x", sol::property(
                [](const glm::vec3& v) { return v.x; },
                [](glm::vec3& v, float x) { v.x = x; }
            ),
            "y", sol::property(
                [](const glm::vec3& v) { return v.y; },
                [](glm::vec3& v, float y) { v.y = y; }
            ),
            "z", sol::property(
                [](const glm::vec3& v) { return v.z; },
                [](glm::vec3& v, float z) { v.z = z; }
            )
        );
    }

    void LuaManager::bindRegistration() {

        lua_["registerUpdate"] = [this](sol::protected_function fn) {
            updates_.push_back({ currentEntity_, fn, currentRunWhenPaused_ });
            };

        lua_["registerKeyDown"] = [this](std::string name, sol::protected_function fn) {
            //keyDown_[name].push_back({ currentEntity_, fn, currentRunWhenPaused_ });


            auto& vec = keyDown_[name];
            vec.push_back({ currentEntity_, fn, currentRunWhenPaused_ });
            //PN_CORE_INFO("[Lua] registerKeyDown '{}' ({} handlers)", name, (int)vec.size());
            };

        lua_["registerKeyUp"] = [this](std::string name, sol::protected_function fn) {
            keyUp_[name].push_back({ currentEntity_, fn, currentRunWhenPaused_ });
            };

        lua_["registerOnClick"] = [this](sol::protected_function fn) {
            onClick_.push_back({ currentEntity_, fn, currentRunWhenPaused_ });
            };

        lua_["registerOnCollision"] = [this](sol::protected_function fn, sol::object t) {
            /*onCollision_[currentEntity_].push_back({ currentEntity_, entityToCheck, fn, false, currentRunWhenPaused_ });
            collisionInterests_.push_back({ currentEntity_, entityToCheck });*/

            // target: nil => listen to any; number => entt entity id
            entt::entity target = entt::null;

            if (t.valid()) {
                if (t.get_type() == sol::type::number) {
                    uint32_t id = t.as<uint32_t>();
                    target = static_cast<entt::entity>(id);
                }
                else if (t.get_type() != sol::type::nil) {
                    PN_CORE_WARN("[Lua] registerOnCollision: expected entity id or nil, got {}", (int)t.get_type());
                }
            }

            PN_CORE_INFO("[Lua] registerOnCollision: self={} target={}", (uint32_t)currentEntity_, (uint32_t)target);

            // store under the registering entity (self), target is used only as a filter
            onCollision_[currentEntity_].push_back(
                { currentEntity_, target, std::move(fn), /*once*/ false, currentRunWhenPaused_ }
            );
            };

        lua_["registerPauseHandler"] = [this](sol::protected_function fn) {
            pauseHandlers_.push_back({ currentEntity_, fn, /*runWhenPaused*/ true });
            };

        lua_["setTimeout"] = [this](sol::protected_function fn, float delay) {
            timeouts_.push_back({ fn, delay });
            };

        lua_["getMousePos"] = [this]() {
            if (!api_) return std::make_tuple(0.0f, 0.0f);
            auto pos = api_->Input_GetMousePos();
            return std::make_tuple(pos.x, pos.y);
            };


#if SCRIPT_ENABLE_DEBUG_TOOLS
        // Example: only in debug builds, allow dangerous actions
        lua_["shutdownApplication"] = [] {
            logMsg("shutdownApplication called (debug only)");
            // Your engine shutdown here if you really want to expose it.
            };
#endif
    }

    void LuaManager::bindEngineAPI() {

        auto is_api_ready = [this]() -> bool {
            if (!api_) {
                PN_ERROR("[LuaManager] IEngineAPI not set.");
                return false;
            }
            return true;
            };

        lua_.set_function("engineAvailable", [this] {
            return api_ != nullptr;
            });

        lua_.set_function("getCurrentSceneName", [this] {
            return api_ ? api_->GetCurrentSceneName() : std::string();
            });

        // ---------- logging / print ----------
        lua_.set_function("log", [this](sol::variadic_args va, sol::this_state ts) {
            sol::state_view L(ts);
            sol::protected_function tostring = L["tostring"];

            std::string out;
            out.reserve(256);

            bool first = true;
            for (sol::object v : va) {
                if (!first) out += ' ';
                first = false;

                sol::protected_function_result r = tostring(v);
                if (r.valid()) out += r.get<std::string>();
                else           out += "<tostring-error>";
            }

#ifdef PN_PLATFORM_ANDROID
            __android_log_print(ANDROID_LOG_INFO, "LUA", "%s", out.c_str());
#else
            std::fputs(("[LUA] " + out + "\n").c_str(), stdout);
            std::fflush(stdout);
#endif
            });

        lua_.set_function("print", [this](sol::variadic_args va, sol::this_state ts) {
            sol::state_view L(ts);
            sol::function log = L["log"];
            log(sol::as_args(va));
            });


        /* =========================================================================== */
        /*                            Entities / Prefabs                               */
        /* =========================================================================== */
        lua_.set_function("createEntity",
            [this, is_api_ready](std::string layer, std::string name) -> int {
                if (!is_api_ready()) return -1;
                entt::entity e = api_->CreateEntity(std::move(layer), std::move(name));
                return static_cast<int>(entt::to_integral(e));
            }
        );

        lua_.set_function("deleteEntity",
            [this, is_api_ready](int id) {
                if (!is_api_ready()) return;
                api_->DeleteEntity(toEntity(id));
            }
        );

        //lua_.set_function("createPrefabInstance",
        //    [this, is_api_ready](std::string prefab, std::string layer, std::string name) -> int {
        //        if (!is_api_ready()) return -1;

        //        //ask the engine to instantiate the prefab
        //        entt::entity e = api_->CreatePrefabInstance(std::move(prefab), std::move(layer), std::move(name));
        //        if (e != entt::null) {
        //            return static_cast<int>(entt::to_integral(e));
        //        }

        //        // fallback hook 
        //        if (instantiatePrefab_) {
        //            entt::entity f = instantiatePrefab_(prefab, layer, name);
        //            if (f != entt::null) {
        //                return static_cast<int>(entt::to_integral(f));
        //            }
        //            PN_WARN("[Lua] instantiatePrefab_ failed. Falling back to empty entity.");
        //        }

        //        // create an empty entity so scripts dont break
        //        entt::entity empty = api_->CreateEntity({}, {});
        //        PN_WARN("[Lua] Prefab creation failed. Created empty entity instead.");
        //        return static_cast<int>(entt::to_integral(empty));
        //    }
        //);

        /* =========================================================================== */
        /*                                  Lookup                                     */
        /* =========================================================================== */
        lua_.set_function("findEntity", [this](std::string name)->sol::object {
            if (!api_) return sol::make_object(lua_, sol::nil);
            auto id = api_->FindEntity(name);
            return id ? sol::make_object(lua_, *id) : sol::make_object(lua_, sol::nil);
            });
        lua_.set_function("getImageID", [this](const std::string name) { return api_ ? api_->GetImageGUID(name) : std::string{};   });
        lua_.set_function("getScriptID", [this](const std::string name) { return api_ ? api_->GetScriptGUID(name) : std::string{};  });
        lua_.set_function("getAudioID", [this](const std::string name) { return api_ ? api_->GetAudioGUID(name) : std::string{};   });
        lua_.set_function("getModelID", [this](const std::string name) { return api_ ? api_->GetModelGUID(name) : std::string{};   });
        lua_.set_function("getFontID", [this](const std::string name) { return api_ ? api_->GetFontGUID(name) : std::string{};    });
        lua_.set_function("getScenesID", [this](const std::string name) { return api_ ? api_->GetScenesGUID(name) : std::string{};  });
        lua_.set_function("getPrefabsID", [this](const std::string name) { return api_ ? api_->GetPrefabsGUID(name) : std::string{}; });
        lua_.set_function("getDataID", [this](const std::string name) { return api_ ? api_->GetDataGUID(name) : std::string{};    });
        lua_.set_function("getShaderID", [this](const std::string name) { return api_ ? api_->GetShaderGUID(name) : std::string{};  });

        lua_.set_function("getTexture", [this](entt::entity entityId) {
            return api_ ? api_->GetEntityTexture(entityId) : "";
            });
        lua_.set_function("setTexture", [this](entt::entity entityId, std::string guidStr) {
            if (api_) {
                api_->SetEntityTexture(entityId, guidStr);
            }
            });


        /* =========================================================================== */
        /*                     Metadata (name / tags / groups)                         */
        /* =========================================================================== */
        lua_.set_function("setEntityName", [this](entt::entity entityId, std::string name) { if (api_) api_->SetEntityName(entityId, name); });
        lua_.set_function("getEntityName", [this](entt::entity entityId)->sol::object {
            if (!api_) return sol::make_object(lua_, sol::nil);
            auto s = api_->GetEntityName(entityId);
            return s ? sol::make_object(lua_, *s) : sol::make_object(lua_, sol::nil);
            });
        lua_.set_function("addTag", [this](entt::entity entityId, std::string tag) { if (api_) api_->AddTag(entityId, tag); });
        lua_.set_function("removeTag", [this](entt::entity entityId, std::string tag) { if (api_) api_->RemoveTag(entityId, tag); });
        lua_.set_function("hasTag", [this](entt::entity entityId, std::string tag) { return api_ ? api_->HasTag(entityId, tag) : false; });
        /*lua_.set_function("assignGroup", [this](entt::entity entityId, std::string g) { if (api_) api_->AssignGroup(entityId, g); });
        lua_.set_function("unassignGroup", [this](entt::entity entityId) { if (api_) api_->UnassignGroup(entityId); });
        lua_.set_function("getGroup", [this](entt::entity entityId)->sol::object {
            if (!api_) return sol::make_object(lua_, sol::nil);
            auto g = api_->GetGroup(entityId);
            return g ? sol::make_object(lua_, *g) : sol::make_object(lua_, sol::nil);
            });*/
        lua_.set_function("getEntitiesByTag", [this](const std::string& tag) {
            if (!api_) return std::vector<entt::entity>{};
            return api_->GetEntitiesByTag(tag);
            });


        /* =========================================================================== */
        /*                                Transform                                    */
        /* =========================================================================== */
        lua_.set_function("getPosition", [this](entt::entity entityId) {
            if (!api_) {
                PN_ERROR("[LuaManager] API not initialized!");
                return std::make_tuple(0.f, 0.f, 0.f);
            }
            auto p = api_->GetPosition(entityId);
            return std::make_tuple(p.x, p.y, p.z);
            });
        lua_.set_function("setPosition", [this](entt::entity entityId, float x, float y, float z) { if (api_) api_->SetPosition(entityId, { x,y,z }); });

        lua_.set_function("get2DPosition", [this](entt::entity entityId) {
            if (!api_) {
                PN_ERROR("[LuaManager] API not initialized!");
                return std::make_tuple(0.f, 0.f);
            }
            auto p = api_->Get2DPosition(entityId);
            return std::make_tuple(p.x, p.y);
            });
        lua_.set_function("set2DPosition", [this](entt::entity entityId, float x, float y) { if (api_) api_->Set2DPosition(entityId, { x,y }); });

        lua_.set_function("getScale", [this](entt::entity entityId) {
            if (!api_) return std::make_tuple(1.f, 1.f, 1.f);
            auto s = api_->GetScale(entityId);
            return std::make_tuple(s.x, s.y, s.z);
            });
        lua_.set_function("setScale", [this](entt::entity entityId, float x, float y, float z) { if (api_) api_->SetScale(entityId, { x,y,z }); });
        lua_.set_function("getRotation", [this](entt::entity entityId) {
            if (!api_) return std::make_tuple(0.f, 0.f, 0.f);
            auto r = api_->GetRotation(entityId);
            return std::make_tuple(r.x, r.y, r.z);
            });

        lua_.set_function("setRotation", [this](entt::entity entityId, float x, float y, float z) {
            if (!api_) return;
            api_->SetRotation(entityId, { x, y, z });
            });

        lua_.set_function("getMobileMoveAxes", []() {
            return std::make_tuple(
                PAIN::g_MobileMoveAxes.x,
                PAIN::g_MobileMoveAxes.y
            );
            });

        lua_.set_function("getMobileLookDelta", []() { // PC: (0,0), Android: rightside drag since last frame
            float dx = PAIN::g_MobileLookDelta.dx;
            float dy = PAIN::g_MobileLookDelta.dy;
            // consume for this frame
            PAIN::g_MobileLookDelta.dx = 0.f;
            PAIN::g_MobileLookDelta.dy = 0.f;
            return std::make_tuple(dx, dy);
            });

        #ifdef __ANDROID__
                lua_.set_function("isAndroid", []() { return true; });
        #else
                lua_.set_function("isAndroid", []() { return false; });
        #endif


        /* =========================================================================== */
        /*                                  Physics                                    */
        /* =========================================================================== */
        lua_.set_function("getVelocity", [this](entt::entity entityId) {
            if (!api_) return std::make_tuple(0.f, 0.f, 0.f);
            auto v = api_->GetVelocity(entityId);
            return std::make_tuple(v.x, v.y, v.z);
            });
        lua_.set_function("setVelocity", [this](entt::entity entityId, float x, float y, float z) { if (api_) api_->SetVelocity(entityId, { x,y,z }); });

        lua_.set_function("isGrounded_", [this](entt::entity entityId, float maxDistance) {
            return api_ ? api_->IsGrounded(entityId, maxDistance) : false;
			});
        
        lua_.set_function("getWallNormal_", [this](entt::entity entityId, float dx, float dy, float dz, float checkDistance) {
            if (!api_) return std::make_tuple(false, 0.f, 0.f, 0.f);

            auto [hit, normal] = api_->GetWallNormal(entityId, glm::vec3(dx, dy, dz), checkDistance);
            return std::make_tuple(hit, normal.x, normal.y, normal.z);
            });

        lua_.set_function("disablePhysics", [this](entt::entity entityId) {
            if (!api_) return;
            api_->DisablePhysics(entityId); 
            });

        lua_.set_function("enablePhysics", [this](entt::entity entityId) {
            if (!api_) return;
            api_->EnablePhysics(entityId);
            });

        /* =========================================================================== */
        /*                                   Audio                                     */
        /* =========================================================================== */
        //lua_.set_function("audioPlayAt", [this](std::string vpath, float x, float y, float z, float volumeDb) { return api_ ? api_->Audio_Play(vpath, x, y, z, volumeDb) : -1; }); // play a sound at a world position
        //lua_.set_function("audioPlay", [this](std::string vpath, float volumeDb) { return api_ ? api_->Audio_Play(vpath, 0.f, 0.f, 0.f, volumeDb) : -1; }); // play at origin 
        //lua_.set_function("audioPlayRandomFrom", [this](std::string playlist, float x, float y, float z, float volumeDb) { return api_ ? api_->Audio_PlayRandomFrom(playlist, x, y, z, volumeDb) : -1; });
        //lua_.set_function("audioStop", [this](int ch) { return api_ && api_->Audio_Stop(ch); });
        //lua_.set_function("audioPause", [this](int ch) { return api_ && api_->Audio_Pause(ch); });
        //lua_.set_function("audioResume", [this](int ch) { return api_ && api_->Audio_Resume(ch); });
        //lua_.set_function("audioSetVolumeDb", [this](int ch, float db) { return api_ && api_->Audio_SetChannelVolumeDb(ch, db); });
        //lua_.set_function("audioSetPos", [this](int ch, float x, float y, float z) { return api_ && api_->Audio_SetChannelPosition(ch, x, y, z); });
        //lua_.set_function("audioStopAll", [this] { if (api_) api_->Audio_StopAll(); });
        //lua_.set_function("audioPauseAll", [this] { if (api_) api_->Audio_PauseAll(); });
        //lua_.set_function("audioResumeAll", [this] { if (api_) api_->Audio_ResumeAll(); });
        //lua_.set_function("audioSetMuteAll", [this](bool m) { return api_ && api_->Audio_SetMuteAll(m); });
        //lua_.set_function("audioSetListener", [this](float px, float py, float pz, float vx, float vy, float vz, float fx, float fy, float fz, float ux, float uy, float uz) { if (api_) api_->Audio_SetListener(px, py, pz, vx, vy, vz, fx, fy, fz, ux, uy, uz); });
        //lua_.set_function("audioSetGroupVolumeDb", [this](std::string group, float db) { return api_ && api_->Audio_SetGroupVolumeDb(group, db); });
        //lua_.set_function("audioFadeGroupToDb", [this](std::string group, float targetDb, float seconds) { return api_ && api_->Audio_FadeGroupToDb(group, targetDb, seconds); });

        lua_.set_function("audioPlay", [this](entt::entity entityId) {
            if (!api_) return;
            api_->Audio_Play(entityId);
            });

        lua_.set_function("audioStop", [this](entt::entity entityId) {
            if (!api_) return;
            api_->Audio_Stop(entityId);
            });

        lua_.set_function("audioSetVolumeDb", [this](entt::entity entityId, float db) {
            if (!api_) return;
            api_->Audio_SetVolumeDb(entityId, db);
            });

        lua_.set_function("audioSetGroup", [this](entt::entity entityId, std::string group) {
            if (!api_) return;
            api_->Audio_SetGroup(entityId, std::move(group));
            });

        lua_.set_function("audioSetLooping", [this](entt::entity entityId, bool looping) {
            if (!api_) return;
            api_->Audio_SetLooping(entityId, looping);
            });

        // ==================== New Direct File Playback Functions ====================
        // These functions access the Audio service directly via services_

        // audioPlayFile(filename, volumeDb?, looping?, is3D?)
        // Plays audio file with optional spatial positioning
        lua_.set_function("audioPlayFile", [this](const std::string& filename, 
            sol::optional<float> volumeDb, 
            sol::optional<bool> looping, 
            sol::optional<bool> is3D) -> int {
            if (!services_) return -1;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return -1;
            
            float vol = volumeDb.value_or(0.0f);
            bool loop = looping.value_or(false);
            bool spatial = is3D.value_or(false);
            glm::vec3 pos(0.0f); // Default position for non-3D
            
            auto result = audio->playFile(filename, "sfx", vol, loop, spatial, pos,
                Audio::MIN_DISTANCE_3D, Audio::MAX_DISTANCE_3D);
            
            return result ? result->value : -1;
            });

        // audioPlaySFX(filename, looping?) - Simple SFX playback
        lua_.set_function("audioPlaySFX", [this](const std::string& filename, sol::optional<bool> looping) -> int {
            if (!services_) return -1;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return -1;
            
            auto result = audio->playSFX(filename, looping.value_or(false), 0.0f);
            return result ? result->value : -1;
            });

        // audioPlayBGM(filename, overlay?) - Play BGM, overlay adds to existing BGM
        lua_.set_function("audioPlayBGM", [this](const std::string& filename, sol::optional<bool> overlay) -> int {
            if (!services_) return -1;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return -1;
            
            auto result = audio->playBGM(filename, overlay.value_or(false), 0.0f);
            return result ? result->value : -1;
            });

        // audioTransitionBGM(newFilename, transitionTime?) - Crossfade to new BGM
        lua_.set_function("audioTransitionBGM", [this](const std::string& newFilename, sol::optional<float> transitionTime) {
            if (!services_) return;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return;
            
            audio->transitionBGM(newFilename, transitionTime.value_or(2.0f), 0.0f);
            });

        // audioTransitionBGMWithSFX(newBGMFilename, sfxFilename, transitionTime?) - Crossfade with SFX trigger
        lua_.set_function("audioTransitionBGMWithSFX", [this](const std::string& newBGMFilename, 
            const std::string& sfxFilename, sol::optional<float> transitionTime) {
            if (!services_) return;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return;
            
            audio->transitionBGMWithSFX(newBGMFilename, sfxFilename, transitionTime.value_or(2.0f), 0.0f);
            });

        // ==================== Spatial / 3D Audio Functions ====================

        // audioPlaySFXAt(filename, x, y, z, volumeDb?, looping?) - Play SFX at 3D position
        lua_.set_function("audioPlaySFXAt", [this](const std::string& filename, 
            float x, float y, float z, sol::optional<float> volumeDb, sol::optional<bool> looping) -> int {
            if (!services_) return -1;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return -1;
            
            auto result = audio->playSFXAt(filename, glm::vec3(x, y, z), 
                volumeDb.value_or(0.0f), looping.value_or(false),
                Audio::MIN_DISTANCE_3D, Audio::MAX_DISTANCE_3D);
            return result ? result->value : -1;
            });

        // audioPlaySFXFromEntity(filename, entityId, volumeDb?, looping?) - Play SFX from entity's position
        lua_.set_function("audioPlaySFXFromEntity", [this](const std::string& filename, 
            entt::entity entityId, sol::optional<float> volumeDb, sol::optional<bool> looping) -> int {
            if (!services_ || !api_) return -1;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return -1;
            
            // Get entity position
            glm::vec3 pos = api_->GetPosition(entityId);
            
            auto result = audio->playSFXAt(filename, pos, 
                volumeDb.value_or(0.0f), looping.value_or(false),
                Audio::MIN_DISTANCE_3D, Audio::MAX_DISTANCE_3D);
            return result ? result->value : -1;
            });

        // ==================== Random SFX Functions ====================

        // audioPlayRandomSFX({file1, file2, ...}, volumeDb?) - Play random SFX from list
        lua_.set_function("audioPlayRandomSFX", [this](sol::table fileList, sol::optional<float> volumeDb) -> int {
            if (!services_) return -1;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return -1;
            
            std::vector<std::string> files;
            for (size_t i = 1; i <= fileList.size(); ++i) {
                sol::optional<std::string> f = fileList[i];
                if (f) files.push_back(*f);
            }
            
            if (files.empty()) return -1;
            
            auto result = audio->playRandomFromList(files, volumeDb.value_or(0.0f), false, 
                glm::vec3(0), Audio::MIN_DISTANCE_3D, Audio::MAX_DISTANCE_3D);
            return result ? result->value : -1;
            });

        // audioPlayRandomSFXAt({file1, file2, ...}, x, y, z, volumeDb?) - Random SFX at position
        lua_.set_function("audioPlayRandomSFXAt", [this](sol::table fileList, 
            float x, float y, float z, sol::optional<float> volumeDb) -> int {
            if (!services_) return -1;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return -1;
            
            std::vector<std::string> files;
            for (size_t i = 1; i <= fileList.size(); ++i) {
                sol::optional<std::string> f = fileList[i];
                if (f) files.push_back(*f);
            }
            
            if (files.empty()) return -1;
            
            auto result = audio->playRandomFromList(files, volumeDb.value_or(0.0f), true, 
                glm::vec3(x, y, z), Audio::MIN_DISTANCE_3D, Audio::MAX_DISTANCE_3D);
            return result ? result->value : -1;
            });

        // audioPlayRandomSFXFromEntity({file1, file2, ...}, entityId, volumeDb?) - Random SFX from entity
        lua_.set_function("audioPlayRandomSFXFromEntity", [this](sol::table fileList, 
            entt::entity entityId, sol::optional<float> volumeDb) -> int {
            if (!services_ || !api_) return -1;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return -1;
            
            std::vector<std::string> files;
            for (size_t i = 1; i <= fileList.size(); ++i) {
                sol::optional<std::string> f = fileList[i];
                if (f) files.push_back(*f);
            }
            
            if (files.empty()) return -1;
            
            glm::vec3 pos = api_->GetPosition(entityId);
            
            auto result = audio->playRandomFromList(files, volumeDb.value_or(0.0f), true, 
                pos, Audio::MIN_DISTANCE_3D, Audio::MAX_DISTANCE_3D);
            return result ? result->value : -1;
            });

        // ==================== Channel Control Functions ====================

        // audioSetChannelVolume(channelId, volumeDb) - Set volume of a specific channel
        lua_.set_function("audioSetChannelVolume", [this](int channelId, float volumeDb) {
            if (!services_) return;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return;
            audio->setVolumeDb(Audio::AudioChannelId{channelId}, volumeDb);
            });

        // audioSetChannelPosition(channelId, x, y, z) - Set 3D position of a channel
        lua_.set_function("audioSetChannelPosition", [this](int channelId, float x, float y, float z) {
            if (!services_) return;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return;
            audio->setPosition(Audio::AudioChannelId{channelId}, glm::vec3(x, y, z));
            });

        // audioStopChannel(channelId) - Stop a specific channel
        lua_.set_function("audioStopChannel", [this](int channelId) {
            if (!services_) return;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return;
            audio->stop(Audio::AudioChannelId{channelId});
            });

        // ==================== Global Audio Multi-Track Control ====================
        // These functions control persistent global audio that survives scene changes
        // Uses static storage in sysAudio.cpp

        // globalBGMSetVolume(trackIndex, volumeDb) - Set volume immediately
        lua_.set_function("globalBGMSetVolume", [this](int trackIndex, float volumeDb) {
            if (!services_) return;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return;
            Audio::GlobalAudio_SetVolume(trackIndex, volumeDb, audio.get());
        });

        // globalBGMFade(trackIndex, targetDb, durationSeconds) - Smooth C++ fade
        lua_.set_function("globalBGMFade", [this](int trackIndex, float targetDb, float duration) {
            Audio::GlobalAudio_Fade(trackIndex, targetDb, duration);
            PN_INFO("[Lua] globalBGMFade: Track {} fading to {}dB over {}s", trackIndex, targetDb, duration);
        });

        // globalBGMGetTrackCount() - Get number of active Global Audio tracks
        lua_.set_function("globalBGMGetTrackCount", []() -> int {
            return Audio::GlobalAudio_GetTrackCount();
        });

        // globalBGMGetVolume(trackIndex) - Get current volume of a track
        lua_.set_function("globalBGMGetVolume", [](int trackIndex) -> float {
            return Audio::GlobalAudio_GetVolume(trackIndex);
        });

        // globalBGMStopAll() - Stop all Global Audio tracks
        lua_.set_function("globalBGMStopAll", [this]() {
            if (!services_) return;
            auto audio = services_->get<Audio::Audio>();
            if (!audio) return;
            Audio::GlobalAudio_StopAll(audio.get());
        });

        // globalBGMClear() - Clear initialization flag for new track set (used during scene transitions)
        lua_.set_function("globalBGMClear", []() {
            Audio::GlobalAudio_Clear();
        });

        // globalBGMIsInitialized() - Check if global audio is initialized
        lua_.set_function("globalBGMIsInitialized", []() -> bool {
            return Audio::GlobalAudio_IsInitialized();
        });

        // globalBGMFadeAllAndQuit(durationSeconds) - Fade all tracks then quit
        lua_.set_function("globalBGMFadeAllAndQuit", [this](float duration) {
            if (!services_) return;
            
            // Fade all tracks to -80dB
            int count = Audio::GlobalAudio_GetTrackCount();
            for (int i = 0; i < count; ++i) {
                Audio::GlobalAudio_Fade(i, -80.0f, duration);
            }
            
            // Schedule quit after fade (using pending scene change mechanism)
            // For now, just log - actual quit delay needs additional implementation
            PN_INFO("[Lua] globalBGMFadeAllAndQuit: Fading {} tracks over {}s then quit", count, duration);
            
            // Note: To actually delay quit, we'd need a timer system
            // For now the script should manually call quitApplication() after waiting
        });


        /* =========================================================================== */
        /*                           Scene / System state                              */
        /* =========================================================================== */
        lua_.set_function("changeScene", [this](std::string name) {
            if (!api_ || pendingSceneChange_) return;
            setPendingSceneChange([this, n = std::move(name)] { api_->ChangeScene(n); });
            });
        lua_.set_function("getCurrentSceneName", [this]() -> std::string {
            return api_ ? api_->GetCurrentSceneName() : "";
            });
        lua_.set_function("getPreviousSceneName", [this]() -> std::string {
            return api_ ? api_->GetPreviousSceneName() : "";
            });
        lua_.set_function("quitApplication", [this]() {
            if (!api_) return;
            api_->QuitApplication();
            });
        lua_.set_function("SetGamePaused", [this](bool p) { if (api_) api_->SetGamePaused(p); });
        lua_.set_function("IsGamePaused", [this] { return api_ ? api_->IsGamePaused() : false; });
        lua_.set_function("getFPS", [this] { return api_ ? api_->GetFps() : 0.0f; });
        lua_.set_function("setDeltaTimeMultiplier", [this](float m) { if (api_) api_->SetDeltaMultiplier(m); });
        lua_.set_function("getDeltaTimeMultiplier", [this] { return api_ ? api_->GetDeltaMultiplier() : 1.0f; });

        /* =========================================================================== */
        /*                              Layer Control                                  */
        /* =========================================================================== */
        lua_.set_function("setLayerEnabled", [this](int layerId, bool enabled) {
            if (!api_) {
                PN_ERROR("[LuaManager] setLayerEnabled: API not initialized");
                return;
            }

            bool success = api_->SetLayerEnabled(layerId, enabled);
            if (success) {
                PN_INFO("[Lua] Layer {} enabled: {}", layerId, enabled);
            }
            else {
                PN_WARN("[Lua] Failed to set Layer {} - layer not found", layerId);
            }
            });

        lua_.set_function("getLayerEnabled", [this](int layerId) -> bool {
            if (!api_) {
                PN_ERROR("[LuaManager] getLayerEnabled: API not initialized");
                return false;
            }
            return api_->GetLayerEnabled(layerId);
            });


        /* =========================================================================== */
        /*                              Graphics / FX                                  */
        /* =========================================================================== */
        lua_.set_function("shakeCamera", [this](float dur, float amp) { if (api_) api_->ShakeCamera(dur, amp); });

        lua_.set_function("cameraSetTransform",
            [this](float px, float py, float pz,
                   float tx, float ty, float tz,
                   float ux, float uy, float uz)
                  {
                      if (!api_) return;
                      api_->Camera_SetTransform(
                          glm::vec3{ px, py, pz },
                          glm::vec3{ tx, ty, tz },
                          glm::vec3{ ux, uy, uz }
                      );
                  });

        lua_.set_function("getCameraOffsets", [this](entt::entity entityId) {
            if (!api_) {
                PN_ERROR("[LuaManager] API not initialized!");
                // 6 floats: trans(x,y,z), rot(x,y,z)
                return std::make_tuple(0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
            }

            auto [t, r] = api_->GetCameraOffsets(entityId);
            return std::make_tuple(t.x, t.y, t.z, r.x, r.y, r.z);
            });

        lua_.set_function("cameraResolveCollision",
            [this](float px, float py, float pz,
                   float cx, float cy, float cz) {
                if (!api_) return std::make_tuple(px, py, pz);
                
                glm::vec3 resolved = api_->Camera_ResolveCollision(
                    glm::vec3(px, py, pz),
                    glm::vec3(cx, cy, cz)
                );
                return std::make_tuple(resolved.x, resolved.y, resolved.z);
            });

        lua_.set_function("cameraGetPositionWithCollision",
            [this](float playerX, float playerY, float playerZ,
                   float desiredX, float desiredY, float desiredZ) {
                if (!api_) return std::make_tuple(desiredX, desiredY, desiredZ);
                
                glm::vec3 result = api_->Camera_GetPositionWithCollision(
                    glm::vec3(playerX, playerY, playerZ),
                    glm::vec3(desiredX, desiredY, desiredZ)
                );
                return std::make_tuple(result.x, result.y, result.z);
            });

        /* =========================================================================== */
        /*                                Particles                                    */
        /* =========================================================================== */
        lua_.set_function("spawnParticles", [this](int id, int count) { if (api_) api_->SpawnParticles(id, count, false); });
        lua_.set_function("spawnParticlesIgnoreRotation", [this](int id, int count) { if (api_) api_->SpawnParticles(id, count, true); });

        /* =========================================================================== */
        /*                              Input Helpers                                  */
        /* =========================================================================== */
        lua_.set_function("isKeyDown", [this](int k) { return api_ && api_->Input_IsKeyDown(k); });
        lua_.set_function("wasKeyPressed", [this](int k) { return api_ && api_->Input_WasKeyPressed(k); });
        lua_.set_function("wasKeyReleased", [this](int k) { return api_ && api_->Input_WasKeyReleased(k); });
        lua_.set_function("isMouseDown", [this](int b) { return api_ && api_->Input_IsMouseDown(b); });
        lua_.set_function("wasMousePressed", [this](int b) { return api_ && api_->Input_WasMousePressed(b); });
        lua_.set_function("wasMouseReleased", [this](int b) { return api_ && api_->Input_WasMouseReleased(b); });
        lua_.set_function("mousePos", [this] { return api_ ? api_->Input_GetMousePos() : glm::vec2{ 0 }; });
        lua_.set_function("mouseScroll", [this] { return api_ ? api_->Input_GetScrollDelta() : glm::vec2{ 0 }; });
        lua_.set_function("cursorInWindow", [this] { return api_ && api_->Input_IsCursorInWindow(); });
        lua_.set_function("hideCursor", [this](bool hidden){
           if (!api_) return;
           api_->HideCursor(hidden);
        });

        /* =========================================================================== */
        /*                                ModelRenderer                                 */
        /* =========================================================================== */
        lua_.set_function("getModelId", [this](entt::entity entityId) -> sol::object {
            if (!api_) return sol::make_object(lua_, sol::nil);
            auto m = api_->GetMeshId(entityId);
            return m ? sol::make_object(lua_, static_cast<int>(*m))
                : sol::make_object(lua_, sol::nil);
            });
        lua_.set_function("setMeshId", [this](entt::entity entityId, int meshId) { if (api_) api_->SetMeshId(entityId, static_cast<uint32_t>(meshId)); });

        lua_.set_function("setUITexture", [this](entt::entity entityId, std::string textureName) {
            if (!api_) return;
            api_->SetUITexture(entityId, textureName);
            }
        );
        lua_.set_function("getUITextureScale", [this](entt::entity e) {
            auto s = api_->GetUITextureScale(e);
            return std::make_tuple(s.x, s.y);
            });

        lua_.set_function("setUITextureScale", [this](entt::entity e, float x, float y) {
            api_->SetUITextureScale(e, { x, y });
            });

        /* =========================================================================== */
        /*                                  Lighting                                   */
        /* =========================================================================== */
        lua_.set_function("hasLight", [this](entt::entity entityId) {return api_ && api_->HasLight(entityId); });
        lua_.set_function("addLight", [this](entt::entity entityId) {if (api_) api_->AddLight(entityId); });
        lua_.set_function("removeLight", [this](entt::entity entityId) {if (api_) api_->RemoveLight(entityId); });
        lua_.set_function("setLightPosition", [this](entt::entity entityId, float x, float y, float z) {if (api_) api_->SetLightPosition(entityId, x, y, z); });
        lua_.set_function("setLightIntensity", [this](entt::entity entityId, float r, float g, float b) {if (api_) api_->SetLightIntensity(entityId, r, g, b); });
        lua_.set_function("setLightType", [this](entt::entity entityId, int typeInt) {if (api_) api_->SetLightType(entityId, typeInt);});
        lua_.set_function("setLightDirection", [this](entt::entity entityId, float x, float y, float z) {if (api_) api_->SetLightDirection(entityId, x, y, z); });
        lua_.set_function("setShadowType", [this](entt::entity entityId, int shadowTypeInt) {if (api_) api_->SetShadowType(entityId, shadowTypeInt); });

        /* =========================================================================== */
        /*                                 Animations                                  */
        /* =========================================================================== */
        auto animTable = lua_["Animation"].get_or_create<sol::table>();

        animTable.set_function("Play", [this](entt::entity entityId, const std::string& name) {
            if (api_) api_->Animation_Play(entityId, name);
        });

        animTable.set_function("CrossFade", [this](entt::entity entityId, const std::string& name, float duration) {
            if (api_) api_->Animation_CrossFade(entityId, name, duration);
        });

        animTable.set_function("SetSpeed", [this](entt::entity entityId, float speed) {
            if (api_) api_->Animation_SetSpeed(entityId, speed);
        });

        animTable.set_function("SetLoop", [this](entt::entity entityId, bool loop) {
            if (api_) api_->Animation_SetLoop(entityId, loop);
        });

        animTable.set_function("IsPlaying", [this](entt::entity entityId, const std::string& name) {
            if (api_) return api_->Animation_IsPlaying(entityId, name);
                return false;
        });
        animTable.set_function("GetTime", [this](entt::entity entityId) {
            if (api_) return api_->GetAnimationTime(entityId);
            return 0.0f;
            });
        animTable.set_function("GetDuration", [this](entt::entity entityId) {
            if (api_) return api_->GetAnimationDuration(entityId);
            return 0.0f;
            });
        animTable.set_function("SetTime", [this](entt::entity entityId, float time) {
            if (api_) api_->SetAnimationTime(entityId, time);
            });
    }


    bool LuaManager::loadScriptForEntity(entt::entity entity,
        const std::string& filePath,
        const std::vector<ScriptExternalVar>& vars,
        bool runWhenPaused) {
        currentEntity_ = entity;
        currentRunWhenPaused_ = runWhenPaused;

        bool ok = runFileIntoEnv(filePath, entity, vars, runWhenPaused);

        currentEntity_ = entt::null;
        currentRunWhenPaused_ = false;
        return ok;
    }

    bool LuaManager::runFileIntoEnv(const std::string& path, entt::entity entity,
        const std::vector<ScriptExternalVar>& vars,
        bool /*runWhenPaused*/) {
        std::string code = readFileText(path);
        if (code.empty()) {
            PN_ERROR("[LuaManager] Failed to read script: {}", path);
            return false;
        }

        // Per-script sandbox inheriting from globals
        sol::environment env(lua_, sol::create, lua_.globals());
        env["entityId"] = entity;

        // expose the real globals table to scripts
        env["_G_root"] = lua_.globals();

        // Inject external vars
        for (const auto& v : vars) {
            std::visit([&](auto&& value) { env[v.id] = value; }, v.val);
        }

        return runLuaInEnv(lua_, code, env, path);
    }

    entt::entity LuaManager::toEntity(int id)
    {
        return static_cast<entt::entity>(id);
    }

    // ----------------------------------------------------------------------------
    // Tick & Event processing
    // ----------------------------------------------------------------------------

    namespace {

    }

    void LuaManager::tick(double dt, double unscaled_dt) {
        //// Update Global Time Table
        //sol::table time = lua_["Time"];
        //if (!time.valid()) time = lua_.create_named_table("Time");

        //time["deltaTime"] = dt;                 // 0.0 when paused
        //time["unscaledDeltaTime"] = unscaled_dt; // 0.016 always
        //time["timeScale"] = (unscaled_dt > 0) ? (dt / unscaled_dt) : 0.0;

        // 1) Move any newly scheduled timeouts into the heap with absolute times
        if (!timeouts_.empty()) {
            const double base = nowSeconds();
            for (auto& t : timeouts_) {
                timeoutHeap_.push(TimeoutNode{ base + static_cast<double>(t.remaining), t.fn });
            }
            timeouts_.clear();
        }

        // 2) Pump due timeouts
        const double now = nowSeconds();
        while (!timeoutHeap_.empty() && timeoutHeap_.top().wake <= now) {
            auto node = timeoutHeap_.top();
            timeoutHeap_.pop();
            sol::protected_function_result r = node.fn();
            if (!r.valid()) {
                sol::error e = r;
                logError("Timeout", e);
            }
        }

        // 3) Execute queued input callbacks
        for (auto& cb : inputQueue_) {
            if (!gamePaused_ || cb.runWhenPaused) {
                sol::protected_function_result r = cb.fn();
                if (!r.valid()) {
                    sol::error e = r;
                    logError("Input callback", e);
                }
            }
        }
        inputQueue_.clear();

        // 4) Execute collision callbacks
        for (auto& cb : collisionQueue_) {
            if (!gamePaused_ || cb.runWhenPaused) {
                sol::protected_function_result r = cb.fn(cb.currentEntityId, cb.collidedEntityId);
                if (!r.valid()) {
                    // try calling without args
                    r = cb.fn();
                }
                if (!r.valid()) {
                    sol::error e = r;
                    logError("Collision callback", e);
                }
            }
        }
        collisionQueue_.clear();

        // 5) Per-frame updates
        for (auto& cb : updates_) {
            if (!gamePaused_ || cb.runWhenPaused) {
                sol::protected_function_result r = cb.fn(dt);
                if (!r.valid()) {
                    sol::error e = r;
                    logError("Update", e);
                }
            }
        }

        // 6) Run any queued C++ operations 
        for (auto& op : delayedOps_) op();
        delayedOps_.clear();

        // 7) Apply pending scene change once per frame
        if (pendingSceneChange_ && !sceneChangeQueued_) {
            sceneChangeQueued_ = true;
            (*pendingSceneChange_)();
            //pendingSceneChange_.reset();
            resetForSceneReload();
            sceneChangeQueued_ = false;
        }
    }

    void LuaManager::onKeyDown(const std::string& name) {
        if (auto it = keyDown_.find(name); it != keyDown_.end()) {
            for (auto& fn : it->second) inputQueue_.push_back(fn);
        }
    }

    void LuaManager::onKeyUp(const std::string& name) {
        if (auto it = keyUp_.find(name); it != keyUp_.end()) {
            for (auto& fn : it->second) inputQueue_.push_back(fn);
        }
    }

    void LuaManager::onClick() {
        for (auto& fn : onClick_) inputQueue_.push_back(fn);
    }

    void LuaManager::onMouseInOut() {
        // no engine-side hit-testing here, just alternating states for now

        for (auto& m : mouseInOut_) {
            if (m.state == MouseInOutLuaFunction::State::MouseOut) {
                inputQueue_.push_back(m.mouseIn);
                m.state = MouseInOutLuaFunction::State::MouseIn;
            }
            else {
                inputQueue_.push_back(m.mouseOut);
                m.state = MouseInOutLuaFunction::State::MouseOut;
            }
        }
    }

    void LuaManager::onCollision(entt::entity a, entt::entity b) {
        //// Look up callbacks registered on entity 'a'
        //if (auto it = onCollision_.find(a); it != onCollision_.end()) {
        //    for (const auto& cb : it->second) {
        //        if (cb.collidedEntityId == b || cb.collidedEntityId == entt::null) {
        //            collisionQueue_.push_back(cb);
        //        }
        //    }
        //}
        //// also check callbacks registered on entity 'b' 
        //if (auto it = onCollision_.find(b); it != onCollision_.end()) {
        //    for (const auto& cb : it->second) {
        //        if (cb.collidedEntityId == a || cb.collidedEntityId == entt::null) {
        //            collisionQueue_.push_back(cb);
        //        }
        //    }
        //}

        auto deliver = [&](entt::entity self, entt::entity other) {
            auto it = onCollision_.find(self);
            if (it == onCollision_.end()) return;
            //PN_CORE_INFO("[Lua] onCollision deliver: self={} other={} handlers={}", (uint32_t)self, (uint32_t)other, (int)it->second.size());

            for (auto& h : it->second) {
                if (h.collidedEntityId == entt::null || h.collidedEntityId == other) {
                    sol::protected_function_result r = h.fn(self, other);
                    if (!r.valid()) { sol::error e = r; logError("onCollision", e); }
                }
            }
            };

        deliver(a, b); // handlers registered by 'a'
        deliver(b, a); // handlers registered by 'b'
    }

    void LuaManager::onPauseChanged(bool paused) {
        gamePaused_ = paused;
        for (auto& cb : pauseHandlers_) {
            sol::protected_function_result r = cb.fn(paused);
            if (!r.valid()) { sol::error e = r; logError("PauseHandler", e); }
        }
    }

    void LuaManager::callGlobal(const std::string& name) {
        /*sol::object obj = lua_[name];
        if (!obj.valid() || obj.get_type() != sol::type::function) return;
        sol::protected_function fn = obj.as<sol::protected_function>();
        sol::protected_function_result r = fn();
        if (!r.valid()) { sol::error e = r; logError("Global(" + name + ")", e); }*/


        sol::object obj = lua_[name];
        if (!obj.valid()) {
            PN_CORE_WARN("[LuaManager] callGlobal: '{}' not found in globals", name);
            return;
        }
        if (obj.get_type() != sol::type::function) {
            PN_CORE_WARN("[LuaManager] callGlobal: '{}' is not a function", name);
            return;
        }
        PN_CORE_INFO("[LuaManager] callGlobal: calling '{}'", name);
        sol::protected_function fn = obj.as<sol::protected_function>();
        sol::protected_function_result r = fn();
        if (!r.valid()) { sol::error e = r; logError("Global(" + name + ")", e); }
    }

    void LuaManager::callGlobal(const std::string& name, const std::string& arg1, entt::entity arg2, const std::string& arg3)
    {
        sol::object obj = lua_[name];
        if (!obj.valid()) {
            PN_CORE_WARN("[LuaManager] callGlobal: '{}' not found in globals", name);
            return;
        }
        if (obj.get_type() != sol::type::function) {
            PN_CORE_WARN("[LuaManager] callGlobal: '{}' is not a function", name);
            return;
        }

        PN_CORE_INFO("[LuaManager] callGlobal: calling '{}' with 3 args", name);
        sol::protected_function fn = obj.as<sol::protected_function>();
        sol::protected_function_result r = fn(arg1, arg2, arg3);
        if (!r.valid()) {
            sol::error e = r;
            logError("Global(" + name + ")", e);
        }
    }

    void LuaManager::callGlobalWithVec2(const std::string& name, float x, float y) {
        sol::object obj = lua_[name];
        if (!obj.valid()) {
            PN_CORE_WARN("[LuaManager] callGlobalWithVec2: '{}' not found in globals", name);
            return;
        }
        if (obj.get_type() != sol::type::function) {
            PN_CORE_WARN("[LuaManager] callGlobalWithVec2: '{}' is not a function", name);
            return;
        }
        
        sol::protected_function fn = obj.as<sol::protected_function>();
        sol::protected_function_result r = fn(x, y);
        if (!r.valid()) { 
            sol::error e = r; 
            logError("Global(" + name + ")", e); 
        }
    }

}