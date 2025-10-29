 #include "LuaState.h"
 #include "Utility/Log.h"
 #include <fstream>

// Shipping sandbox switch (default ON). In dev you can override at compile time.
#ifndef SCRIPT_SHIPPING_SANDBOX
#define SCRIPT_SHIPPING_SANDBOX 1
#endif

// -------- Android asset reading (optional) --------
#ifdef __ANDROID__
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
// Provide this from your app glue once at startup if you want asset-reads here:
extern AAssetManager* g_AssetMgr;
#endif

namespace {
#ifdef __ANDROID__
    static std::string readAssetText(AAssetManager* mgr, const char* path) {
        if (!mgr) return {};
        AAsset* a = AAssetManager_open(mgr, path, AASSET_MODE_BUFFER);
        if (!a) return {};
        const void* buf = AAsset_getBuffer(a);
        const size_t len = AAsset_getLength(a);
        std::string out((const char*)buf, (const char*)buf + len);
        AAsset_close(a);
        return out;
    }
#endif

    static std::string readFileAll(const std::string& path) {
#ifdef __ANDROID__
        // If an asset manager is available, prefer it (let you package Lua in APK)
        extern AAssetManager* g_AssetMgr;
        if (g_AssetMgr) {
            auto s = readAssetText(g_AssetMgr, path.c_str());
            if (!s.empty()) return s;
            // fall through to filesystem if not found in assets
        }
#endif
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }

} // anonymous namespace

 namespace PAIN::Scripting {

     LuaState::LuaState() = default;
     LuaState::~LuaState() = default;

     void LuaState::init(bool enableIoOs) {
#if SCRIPT_SHIPPING_SANDBOX
         (void)enableIoOs; // shipping builds are always sandboxed
         L_.open_libraries(
             sol::lib::base,
             sol::lib::math,
             sol::lib::table,
             sol::lib::string,
             sol::lib::utf8
         );
#ifndef NDEBUG
         // helpful, but keep it out of release to reduce surface
         L_.open_libraries(sol::lib::debug);
#endif
#else
         // Dev mode: optionally allow broader std libs
         if (enableIoOs) {
             L_.open_libraries(
                 sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string,
                 sol::lib::package, sol::lib::io, sol::lib::os
             );
         }
         else {
             L_.open_libraries(
                 sol::lib::base, sol::lib::math, sol::lib::table, sol::lib::string,
                 sol::lib::package
             );
#ifndef NDEBUG
             L_.open_libraries(sol::lib::debug);
#endif
         }
#endif

         bindEngineAPI();
     }


 bool LuaState::doFile(const std::string& filePath) {
     try {
         sol::load_result chunk = L_.load_file(filePath);
         if (!chunk.valid()) {
             sol::error e = chunk;
             PN_ERROR("[Lua load:file] %s\n", e.what());
             return false;
         }
         sol::protected_function_result r = chunk();
         if (!r.valid()) {
             sol::error e = r;
             PN_ERROR("[Lua run:file] %s\n", e.what());
             return false;
         }
         return true;
     }
     catch (const sol::error& e) {
         PN_ERROR("[Lua exception:file] %s\n", e.what());
         return false;
     }
 }

 bool LuaState::doBuffer(const char* data, size_t size, const char* debugName) {
     try {
         sol::load_result chunk = L_.load_buffer(data, size, debugName ? debugName : "buffer");
         if (!chunk.valid()) {
             sol::error e = chunk;
             PN_ERROR("[Lua load:buffer] %s\n", e.what());
             return false;
         }
         sol::protected_function_result r = chunk();
         if (!r.valid()) {
             sol::error e = r;
             PN_ERROR("[Lua run:buffer] %s\n", e.what());
             return false;
         }
         return true;
     }
     catch (const sol::error& e) {
         PN_ERROR("[Lua exception:buffer] %s\n", e.what());
         return false;
     }
 }

 bool LuaState::runScriptInEnv(const ScriptSource& src,
                               sol::environment& outEnv,
                               std::function<void(sol::environment&)> inject) {
     outEnv = sol::environment(L_, sol::create, L_.globals()); // creates env that inherits from globals
     if (inject) inject(outEnv);

     sol::protected_function_result r;
     try {
         if (src.kind == ScriptSource::Kind::FilePath) {
             // Load from file (or asset, on Android)
             std::string code = readFileAll(src.path);
             if (code.empty()) {
                 PN_ERROR("[Lua env] could not read script '%s'\n", src.path.c_str());
                 return false;
             }
             sol::load_result chunk = L_.load(code);
             if (!chunk.valid()) {
                 sol::error e = chunk;
                 PN_ERROR("[Lua env load:file] %s\n", e.what());
                 return false;
             }
             // Apply environment by calling chunk with env
             r = chunk(outEnv);
         }
         else {
             // Load from provided buffer
             if (src.buffer.empty()) {
                 PN_ERROR("[Lua env] empty buffer for '%s'\n", src.name.c_str());
                 return false;
             }
             sol::load_result chunk = L_.load_buffer(src.buffer.data(), src.buffer.size(),
                 src.name.empty() ? "buffer" : src.name.c_str());
             if (!chunk.valid()) {
                 sol::error e = chunk;
                 PN_ERROR("[Lua env load:buffer] %s\n", e.what());
                 return false;
             }
             r = chunk(outEnv);
         }

         if (!r.valid()) {
             sol::error e = r;
             PN_ERROR("[Lua env run] %s\n", e.what());
             return false;
         }
         return true;

     }
     catch (const sol::error& e) {
         PN_ERROR("[Lua env exception] %s\n", e.what());
         return false;
     }
 }

 sol::protected_function_result LuaState::ErrorHandler(lua_State* L, sol::protected_function_result pfr) {
     sol::error err = pfr;
     std::string msg = err.what();
     // Add Lua stack trace
     luaL_traceback(L, L, msg.c_str(), 1);
     const char* tb = lua_tostring(L, -1);
     PN_ERROR("[Lua Error] %s\n", tb ? tb : msg.c_str());
     lua_pop(L, 1);
     return pfr;
 }

 void LuaState::bindEngineAPI() {
     // Simple printf-style logger exposed to Lua
     L_.set_function("log_info", [](const std::string& s) {
         PN_ERROR("[Lua] %s\n", s.c_str());
         });

     //// Example type
     //struct Vec3 { float x{}, y{}, z{}; };
     //L_.new_usertype<Vec3>("Vec3",
     //    sol::constructors<Vec3(), Vec3(float,float,float)>(),
     //    "x", &Vec3::x,
     //    "y", &Vec3::y,
     //    "z", &Vec3::z
     //);

     // Registration hooks
     // L_.set_function("registerUpdate",  [&](sol::protected_function f){ /* push to queue */ });
     // L_.set_function("registerOnCollision", [&](sol::protected_function f){ /* store */ });
 }

 } // namespace PAIN::Scripting
