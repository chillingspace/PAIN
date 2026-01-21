 #include "luaState.h"
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
extern AAssetManager* g_AssetMgr;
#endif

namespace {

    // Reads a whole asset file into a std::string (Android only).
    // Returns empty string if not found / not readable.
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

    // reads file contents either from APK assets (Android) or filesystem (all platforms).
    // on Android, we first try the asset manager (scripts packaged into APK).
    // if not found in assets, we fall back to filesystem (useful on dev builds).
    static std::string readFileAll(const std::string& path) {
#ifdef __ANDROID__
        // prefer APK assets when available
        if (::g_AssetMgr) {
            auto s = readAssetText(::g_AssetMgr, path.c_str());
            if (!s.empty()) return s;
            // fall through to filesystem if not found in assets
        }
#endif
        std::ifstream f(path, std::ios::binary);
        if (!f) return {};
        return std::string(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }

} 

 namespace PAIN::Scripting {

     LuaState::LuaState() = default;
     LuaState::~LuaState() = default;

     // initializes the Lua VM and opens the allowed standard libraries
     //  - Shipping (SCRIPT_SHIPPING_SANDBOX=1): tightly sandboxed libs
     //  - Dev (SCRIPT_SHIPPING_SANDBOX=0): can opt io/os/package via enableIoOs
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
         // debug lib helpful while developing, but keep out of release
         L_.open_libraries(sol::lib::debug);
#endif
#else
         // dev mode: optionally allow broader std libs
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

 // loads and executes a lua file in global environment
 bool LuaState::doFile(const std::string& filePath) {
     try {
         // Read from APK assets if available (Android), else from filesystem.
         std::string code = readFileAll(filePath);
         if (code.empty()) {
             PN_ERROR("[Lua load:file] could not read '{}'", filePath);
             return false;
         }

         // Load the string (not the file path) so it works both on Android and desktop.
         sol::load_result chunk = L_.load(code);
         if (!chunk.valid()) {
             sol::error e = chunk;
             PN_ERROR("[Lua load:file] {}", e.what());
             return false;
         }

         sol::protected_function_result r = chunk();
         if (!r.valid()) {
             sol::error e = r;
             PN_ERROR("[Lua run:file] {}", e.what());
             return false;
         }
         return true;
     }
     catch (const sol::error& e) {
         PN_ERROR("[Lua exception:file] {}", e.what());
         return false;
     }
 }

 // loads and executes a lua buffer, useful for pipelines that feed compiled/packed lua from memory
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

 // executes script inside a fresh env table
 bool LuaState::runScriptInEnv(const ScriptSource& src,
                               sol::environment& outEnv,
                               std::function<void(sol::environment&)> inject) {
     outEnv = sol::environment(L_, sol::create, L_.globals()); // creates env that inherits from globals
     if (inject) inject(outEnv);

     sol::protected_function_result r;
     try {
         if (src.kind == ScriptSource::Kind::FilePath) {
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
             // apply environment by calling chunk with env
             r = chunk(outEnv);
         }
         else {
             // load from provided buffer
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
     luaL_traceback(L, L, msg.c_str(), 1);
     const char* tb = lua_tostring(L, -1);
     PN_ERROR("[Lua Error] %s\n", tb ? tb : msg.c_str());
     lua_pop(L, 1);
     return pfr;
 }

 void LuaState::bindEngineAPI() {
     // printf-style logger exposed to Lua
     L_.set_function("log_info", [](const std::string& s) {
         PN_INFO("[Lua] %s\n", s.c_str());
         });

     // Quit application function
     L_.set_function("quitApplication", []() {
         PN_INFO("[Lua] Quitting application...\n");
         PAIN::g_shouldQuitApplication = true;
         });
 }


 } // namespace PAIN::Scripting
