#include "luaManager.h"
#include "IEngineAPI.h"
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
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }

    static bool runLuaInEnv(sol::state& L, 
                            const std::string& code,
                            sol::environment& env, 
                            std::string_view debugName) { // loads lua chunk safely into given enviro
        sol::load_result lr = L.load(code);
        if (!lr.valid()) { sol::error e = lr; logError("Load(" + std::string(debugName) + ")", e); return false; }
        sol::protected_function_result r = lr(env);   // pass env as argument
        if (!r.valid()) {
            sol::error e = r;
            logError("Run(" + std::string(debugName) + ")", e);
            return false;
        }

        return true;
    }

} // namespace

// ============================================================================
// LuaManager
// ============================================================================

void LuaManager::init(IEngineAPI* api, bool shipping) {  // after init, script can then call for eg registerUpdate function coz exist in lua global table
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
    // NOTE: Bind your usertypes here if needed 
    // Keeping this empty is fine, the manager doesnt depend on ECS details.
}

void LuaManager::bindRegistration() {
    // All registration APIs record callbacks against the "currently parsing" entity

    lua_["registerUpdate"] = [this](sol::protected_function fn) {
        updates_.push_back({ currentEntity_, fn, currentRunWhenPaused_ });
        };

    lua_["registerKeyDown"] = [this](std::string name, sol::protected_function fn) {
        keyDown_[name].push_back({ currentEntity_, fn, currentRunWhenPaused_ });
        };

    lua_["registerKeyUp"] = [this](std::string name, sol::protected_function fn) {
        keyUp_[name].push_back({ currentEntity_, fn, currentRunWhenPaused_ });
        };

    lua_["registerOnClick"] = [this](sol::protected_function fn) {
        onClick_.push_back({ currentEntity_, fn, currentRunWhenPaused_ });
        };

    // entityToCheck == -1 means "ANY_ENTITY"
    lua_["registerOnCollision"] = [this](sol::protected_function fn, int entityToCheck) {
        onCollision_[currentEntity_].push_back({ currentEntity_, entityToCheck, fn, false });
        collisionInterests_.push_back({ currentEntity_, entityToCheck });
        };

    lua_["registerPauseHandler"] = [this](sol::protected_function fn) {
        pauseHandlers_.push_back({ currentEntity_, fn, /*runWhenPaused*/ true });
        };

    // SetTimeout (list-based member exists in header, will keep the API but
    // use a local min-heap in tick() for performance
    lua_["setTimeout"] = [this](sol::protected_function fn, float delay) {
        // Store temporarily; converted to a heap node on next tick()
        timeouts_.push_back({ fn, delay });
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
    // quick guard
    lua_.set_function("engineAvailable", [this] { return api_ != nullptr; });

    // -------- Entities / Prefabs --------
    lua_.set_function("createEntity", [this](std::string layer, std::string name) {
        return api_ ? api_->CreateEntity(std::move(layer), std::move(name)) : -1;
        });
    lua_.set_function("deleteEntity", [this](int id) {
        if (api_) api_->DeleteEntity(id);
        });
    lua_.set_function("createPrefabInstance", [this](std::string prefab, std::string layer, std::string name) {
        return api_ ? api_->CreatePrefabInstance(std::move(prefab), std::move(layer), std::move(name)) : -1;
        });

    // -------- Lookup / Names --------
    lua_.set_function("findEntity", [this](std::string name)->sol::object {
        if (!api_) return sol::make_object(lua_, sol::nil);
        auto id = api_->FindEntity(name);
        return id ? sol::make_object(lua_, *id) : sol::make_object(lua_, sol::nil);
        });
    lua_.set_function("setEntityName", [this](int id, std::string name) { if (api_) api_->SetEntityName(id, name); });
    lua_.set_function("getEntityName", [this](int id)->sol::object {
        if (!api_) return sol::make_object(lua_, sol::nil);
        auto s = api_->GetEntityName(id);
        return s ? sol::make_object(lua_, *s) : sol::make_object(lua_, sol::nil);
        });

    // -------- Meta: Tags / Groups --------
    lua_.set_function("addTag", [this](int id, std::string tag) { if (api_) api_->AddTag(id, tag); });
    lua_.set_function("removeTag", [this](int id, std::string tag) { if (api_) api_->RemoveTag(id, tag); });
    lua_.set_function("hasTag", [this](int id, std::string tag) { return api_ ? api_->HasTag(id, tag) : false; });
    lua_.set_function("assignGroup", [this](int id, std::string g) { if (api_) api_->AssignGroup(id, g); });
    lua_.set_function("unassignGroup", [this](int id) { if (api_) api_->UnassignGroup(id); });
    lua_.set_function("getGroup", [this](int id)->sol::object {
        if (!api_) return sol::make_object(lua_, sol::nil);
        auto g = api_->GetGroup(id);
        return g ? sol::make_object(lua_, *g) : sol::make_object(lua_, sol::nil);
        });

    // -------- Transform (safe, high-level) --------
    lua_.set_function("getPosition", [this](int id) {
        if (!api_) return std::make_tuple(0.f, 0.f, 0.f);
        auto p = api_->GetPosition(id);
        return std::make_tuple(p.x, p.y, p.z);
        });
    lua_.set_function("setPosition", [this](int id, float x, float y, float z) { if (api_) api_->SetPosition(id, { x,y,z }); });
    lua_.set_function("getScale", [this](int id) {
        if (!api_) return std::make_tuple(1.f, 1.f, 1.f);
        auto s = api_->GetScale(id);
        return std::make_tuple(s.x, s.y, s.z);
        });
    lua_.set_function("setScale", [this](int id, float x, float y, float z) { if (api_) api_->SetScale(id, { x,y,z }); });
    // (Optional) Euler helpers:
    //lua_.set_function("rotateEulerDeg", [this](int id, float rx, float ry, float rz) { if (api_) api_->RotateEulerDeg(id, { rx,ry,rz }); });

    // -------- Physics (velocity only, safe) --------
    lua_.set_function("getVelocity", [this](int id) {
        if (!api_) return std::make_tuple(0.f, 0.f, 0.f);
        auto v = api_->GetVelocity(id);
        return std::make_tuple(v.x, v.y, v.z);
        });
    lua_.set_function("setVelocity", [this](int id, float x, float y, float z) { if (api_) api_->SetVelocity(id, { x,y,z }); });

    // -------- Particles / Audio / Scene --------
    lua_.set_function("spawnParticles", [this](int id, int count) { if (api_) api_->SpawnParticles(id, count, false); });
    lua_.set_function("spawnParticlesIgnoreRotation", [this](int id, int count) { if (api_) api_->SpawnParticles(id, count, true); });

    lua_.set_function("playAudio", [this](int id, std::string sound)->sol::object {
        if (!api_) return sol::make_object(lua_, sol::nil);
        auto inst = api_->PlayAudio(id, sound);
        return inst ? sol::make_object(lua_, *inst) : sol::make_object(lua_, sol::nil);
        });
    lua_.set_function("pauseAudio", [this](int inst) { if (api_) api_->PauseAudio(inst); });
    lua_.set_function("resumeAudio", [this](int inst) { if (api_) api_->ResumeAudio(inst); });
    lua_.set_function("setVolume", [this](int inst, float v) { if (api_) api_->SetVolume(inst, v); });

    lua_.set_function("changeScene", [this](std::string name) {
        if (!api_ || pendingSceneChange_) return;
        setPendingSceneChange([this, n = std::move(name)] { api_->ChangeScene(n); });
        });
    lua_.set_function("pauseAllSystems", [this](bool p) { if (api_) api_->PauseAllSystems(p); });
    lua_.set_function("isGamePaused", [this] { return api_ ? api_->IsGamePaused() : false; });
    lua_.set_function("getFPS", [this] { return api_ ? api_->GetFps() : 0.0f; });
    lua_.set_function("setDeltaTimeMultiplier", [this](float m) { if (api_) api_->SetDeltaMultiplier(m); });
    lua_.set_function("getDeltaTimeMultiplier", [this] { return api_ ? api_->GetDeltaMultiplier() : 1.0f; });

    // -------- Camera FX --------
    lua_.set_function("shakeCamera", [this](float dur, float amp) { if (api_) api_->ShakeCamera(dur, amp); });
    lua_.set_function("globalIlluminance", [this](float v) { if (api_) api_->SetGlobalIlluminance(v); });
    lua_.set_function("setAmbientColor", [this](float r, float g, float b) { if (api_) api_->SetAmbientColor(r, g, b); });
    lua_.set_function("showVignetteEffect", [this](bool on, float r, float g, float b, float radius) { if (api_) api_->ShowVignette(on, r, g, b, radius); });

    // -------- Input helpers --------
    lua_.set_function("getMouseWorld", [this] {
        if (!api_) return std::make_tuple(0.f, 0.f);
        auto v = api_->GetMouseWorld(); 
        return std::make_tuple(v.x, v.y);
        });
    lua_.set_function("getMouseView", [this] {
        if (!api_) return std::make_tuple(0.f, 0.f);
        auto v = api_->GetMouseView(); 
        return std::make_tuple(v.x, v.y);
        });
}


bool LuaManager::loadScriptForEntity(int entityId, 
                                     const std::string& filePath,
                                     const std::vector<ScriptExternalVar>& vars,
                                     bool runWhenPaused) {
    currentEntity_ = entityId;
    currentRunWhenPaused_ = runWhenPaused;

    bool ok = runFileIntoEnv(filePath, entityId, vars, runWhenPaused);

    currentEntity_ = -1;
    currentRunWhenPaused_ = false;
    return ok;
}

bool LuaManager::runFileIntoEnv(const std::string& path, int entityId,
    const std::vector<ScriptExternalVar>& vars,
    bool /*runWhenPaused*/) {
    std::string code = readFileText(path);
    if (code.empty()) {
        PN_ERROR("[LuaManager] Failed to read script: {}", path);
        return false;
    }

    // Per-script sandbox inheriting from globals
    sol::environment env(lua_, sol::create, lua_.globals());
    env["entityId"] = entityId;

    // Inject external vars
    for (const auto& v : vars) {
        std::visit([&](auto&& value) { env[v.id] = value; }, v.val);
    }

    return runLuaInEnv(lua_, code, env, path);
}

// ----------------------------------------------------------------------------
// Tick & Event processing
// ----------------------------------------------------------------------------

namespace {
    struct TimeoutNode {
        double wake;
        sol::protected_function fn;
        bool operator<(const TimeoutNode& other) const noexcept { return wake > other.wake; } // priorityqueue is max-heap by default, invert comparator for min-heap
    };
}

void LuaManager::tick(double dt) {
    // 1) Move any newly scheduled timeouts into the heap with absolute times
    static std::priority_queue<TimeoutNode> timeoutHeap;
    if (!timeouts_.empty()) {
        const double base = nowSeconds();
        for (auto& t : timeouts_) {
            timeoutHeap.push(TimeoutNode{ base + static_cast<double>(t.remaining), t.fn });
        }
        timeouts_.clear();
    }

    // 2) Pump due timeouts
    const double now = nowSeconds();
    while (!timeoutHeap.empty() && timeoutHeap.top().wake <= now) {
        auto node = timeoutHeap.top();
        timeoutHeap.pop();
        sol::protected_function_result r = node.fn();
        if (!r.valid()) { sol::error e = r; logError("Timeout", e); }
    }

    // 3) Execute queued input callbacks
    for (auto& cb : inputQueue_) {
        if (!gamePaused_ || cb.runWhenPaused) {
            sol::protected_function_result r = cb.fn();
            if (!r.valid()) { sol::error e = r; logError("Input callback", e); }
        }
    }
    inputQueue_.clear();

    // 4) Execute collision callbacks
    for (auto& cb : collisionQueue_) {
        if (!gamePaused_ || cb.fn.valid()) {
            sol::protected_function_result r = cb.fn(cb.currentEntityId, cb.collidedEntityId);
            if (!r.valid()) {
                // try calling without args
                r = cb.fn();
            }
            if (!r.valid()) { sol::error e = r; logError("Collision callback", e); }
        }
    }
    collisionQueue_.clear();

    // 5) Per-frame updates
    for (auto& cb : updates_) {
        if (!gamePaused_ || cb.runWhenPaused) {
            sol::protected_function_result r = cb.fn(dt);
            if (!r.valid()) { sol::error e = r; logError("Update", e); }
        }
    }

    // 6) Run any queued C++ operations 
    for (auto& op : delayedOps_) op();
    delayedOps_.clear();

    // 7) Apply pending scene change once per frame
    static bool sceneChangeQueued = false;
    if (pendingSceneChange_ && !sceneChangeQueued) {
        sceneChangeQueued = true;
        (*pendingSceneChange_)();
        pendingSceneChange_.reset();
        sceneChangeQueued = false;
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

void LuaManager::onCollision(int a, int b) {
    // Look up callbacks registered on entity 'a'
    if (auto it = onCollision_.find(a); it != onCollision_.end()) {
        for (const auto& cb : it->second) {
            if (cb.collidedEntityId == b || cb.collidedEntityId == -1 /*ANY_ENTITY*/) {
                collisionQueue_.push_back(cb);
            }
        }
    }
    // also check callbacks registered on entity 'b' 
    if (auto it = onCollision_.find(b); it != onCollision_.end()) {
        for (const auto& cb : it->second) {
            if (cb.collidedEntityId == a || cb.collidedEntityId == -1 /*ANY_ENTITY*/) {
                collisionQueue_.push_back(cb);
            }
        }
    }
}

void LuaManager::onPauseChanged(bool paused) {
    gamePaused_ = paused;
    for (auto& cb : pauseHandlers_) {
        sol::protected_function_result r = cb.fn(paused);
        if (!r.valid()) { sol::error e = r; logError("PauseHandler", e); }
    }
}

void LuaManager::callGlobal(const std::string& name) {
    sol::object obj = lua_[name];
    if (!obj.valid() || obj.get_type() != sol::type::function) return;
    sol::protected_function fn = obj.as<sol::protected_function>();
    sol::protected_function_result r = fn();
    if (!r.valid()) { sol::error e = r; logError("Global(" + name + ")", e); }
}
