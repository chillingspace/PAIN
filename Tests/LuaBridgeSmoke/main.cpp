#include <sol/sol.hpp>

#if defined(__ANDROID__)
  #include <android/log.h>
  #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "LuaBridgeSmoke", __VA_ARGS__)
#else
  #include <iostream>
#endif

static void RunLuaBridgeSmoke() {
  sol::state lua;
  lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string);

  // Lua → C++ roundtrip
  lua.script(R"(
    print("Hello from Lua!")
    result = 3 + 4
  )");

  int result = lua["result"].get_or(0);

  // C++ → Lua call
  lua.set_function("cpp_add", [](int a, int b) { return a + b; });
  lua.script(R"(
    print("3 + 5 via C++ function:", cpp_add(3, 5))
  )");

#if defined(__ANDROID__)
  LOGI("Result from Lua: %d", result);
#else
  std::cout << "Result from Lua: " << result << "\n";
#endif
}

#if defined(LUA_BRIDGE_SMOKE_AUTORUN)
// Android path: compiled into PAINEngine; autoruns at startup.
struct _LuaBridgeSmokeAutorun {
  _LuaBridgeSmokeAutorun() { RunLuaBridgeSmoke(); }
} _lua_bridge_smoke_autorun;
#else
// Desktop path: standalone console app.
int main() {
  RunLuaBridgeSmoke();
  return 0;
}
#endif
