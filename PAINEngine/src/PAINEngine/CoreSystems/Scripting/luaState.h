// #pragma once
// #include <sol/sol.hpp>
// #include <string>
// #include <vector>
// #include <functional>

// namespace PAIN::Scripting {

// struct ScriptSource {
//     enum class Kind { FilePath, MemoryBuffer };
//     Kind kind{};
//     std::string path;                 // if FilePath
//     std::string name;                 // debug name (both kinds)
//     std::vector<char> buffer;         // if MemoryBuffer
// };

// class LuaState {
// public:
//     LuaState();
//     ~LuaState();

//     // Init & teardown
//     void init(bool enableIoOs = false); // desktop: true; android: false

//     // Load/execute (global env)
//     bool doFile(const std::string& filePath);
//     bool doBuffer(const char* data, size_t size, const char* debugName);

//     // Sandboxed per-script execution
//     bool runScriptInEnv(const ScriptSource& src,
//                         sol::environment& outEnv,
//                         std::function<void(sol::environment&)> inject = {});

//     // Binding surface
//     void bindEngineAPI();

//     // Access
//     sol::state& lua() { return L_; }

// private:
//     sol::state L_;

//     // Error handling wrapper
//     static sol::protected_function_result ErrorHandler(lua_State*, sol::protected_function_result pfr);
// };

// } // namespace PAIN::Scripting
