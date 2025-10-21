// #include "LuaState.h"
// #include <iostream>

// namespace PAIN::Scripting {

// LuaState::LuaState() = default;
// LuaState::~LuaState() = default;

// void LuaState::init(bool enableIoOs) {
//     // Open a curated set of libs
//     if (enableIoOs) {
//         L_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table,
//                           sol::lib::string, sol::lib::package, sol::lib::io, sol::lib::os);
//     } else {
//         L_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::table,
//                           sol::lib::string, sol::lib::package);
//     }

//     bindEngineAPI();
// }

// bool LuaState::doFile(const std::string& filePath) {
//     try {
//         sol::protected_function_result r =
//             L_.safe_script_file(filePath, &LuaState::ErrorHandler);
//         return r.valid();
//     } catch (const sol::error& e) {
//         std::cerr << "[Lua] " << e.what() << "\n";
//         return false;
//     }
// }

// bool LuaState::doBuffer(const char* data, size_t size, const char* debugName) {
//     try {
//         sol::load_result chunk = L_.load_buffer(data, size, debugName);
//         if (!chunk.valid()) {
//             sol::error e = chunk;
//             std::cerr << "[Lua load] " << e.what() << "\n";
//             return false;
//         }
//         sol::protected_function f = chunk;
//         sol::protected_function_result r = f();
//         if (!r.valid()) {
//             sol::error e = r;
//             std::cerr << "[Lua run] " << e.what() << "\n";
//             return false;
//         }
//         return true;
//     } catch (const sol::error& e) {
//         std::cerr << "[Lua] " << e.what() << "\n";
//         return false;
//     }
// }

// bool LuaState::runScriptInEnv(const ScriptSource& src,
//                               sol::environment& outEnv,
//                               std::function<void(sol::environment&)> inject) {
//     outEnv = sol::environment(L_, sol::create, L_.globals());
//     if (inject) inject(outEnv);

//     sol::protected_function_result r;
//     try {
//         if (src.kind == ScriptSource::Kind::FilePath) {
//             r = L_.safe_script_file(src.path, outEnv, &LuaState::ErrorHandler);
//         } else {
//             if (src.buffer.empty()) return false;
//             sol::load_result chunk = L_.load_buffer(src.buffer.data(), src.buffer.size(),
//                                                     src.name.c_str());
//             if (!chunk.valid()) { sol::error e = chunk; std::cerr << e.what() << "\n"; return false; }
//             sol::protected_function f = chunk;
//             f.environment(outEnv);
//             r = f();
//         }
//     } catch (const sol::error& e) {
//         std::cerr << "[Lua] " << e.what() << "\n";
//         return false;
//     }

//     if (!r.valid()) {
//         sol::error e = r;
//         std::cerr << "[Lua env run] " << e.what() << "\n";
//         return false;
//     }
//     return true;
// }

// sol::protected_function_result LuaState::ErrorHandler(lua_State*, sol::protected_function_result pfr) {
//     sol::error err = pfr;
//     std::string msg = err.what();
//     // Add Lua stack trace
//     luaL_traceback(L, L, msg.c_str(), 1);
//     std::cerr << "[Lua Error] " << lua_tostring(L, -1) << "\n";
//     lua_pop(L, 1);
//     return pfr;
// }

// void LuaState::bindEngineAPI() {
//     // Minimal example:

//     // Logging
//     L_.set_function("log_info", [](const std::string& s) {
//         std::cout << "[Lua] " << s << "\n";
//     });

//     // Example type
//     struct Vec3 { float x{}, y{}, z{}; };
//     L_.new_usertype<Vec3>("Vec3",
//         sol::constructors<Vec3(), Vec3(float,float,float)>(),
//         "x", &Vec3::x,
//         "y", &Vec3::y,
//         "z", &Vec3::z
//     );

//     // Registration hooks
//     // L_.set_function("registerUpdate",  [&](sol::protected_function f){ /* push to queue */ });
//     // L_.set_function("registerOnCollision", [&](sol::protected_function f){ /* store */ });
// }

// } // namespace PAIN::Scripting
