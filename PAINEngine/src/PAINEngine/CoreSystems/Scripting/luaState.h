#pragma once
//#include <sol/sol.hpp>
#include "sol_sanitized.h"
#include <string>
#include <vector>
#include <functional>

namespace PAIN::Scripting {

     struct ScriptSource {
         enum class Kind { FilePath, MemoryBuffer };
         Kind kind{};
         std::string path;                
         std::string name;                 
         std::vector<char> buffer;         
     };

     class LuaState {
     public:
         LuaState();
         ~LuaState();

         void init(bool enableIoOs = false); // desktop: true, android: false
         bool doFile(const std::string& filePath);
         bool doBuffer(const char* data, size_t size, const char* debugName);

         bool runScriptInEnv(const ScriptSource& src, sol::environment& outEnv, std::function<void(sol::environment&)> inject = {}); // sandboxed per-script execution
         void bindEngineAPI();
         sol::state& lua() { return L_; }

     private:
         sol::state L_;
         static sol::protected_function_result ErrorHandler(lua_State* L, sol::protected_function_result pfr);
     };

} // namespace PAIN::Scripting
