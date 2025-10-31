#pragma once

// ---- Platform hygiene (Windows) ----
#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#endif

// ---- sol2 safety/config knobs ----
// Turn on runtime checks/assertions in debug builds.
#ifndef SOL_ALL_SAFETIES_ON
#if !defined(NDEBUG)
#define SOL_ALL_SAFETIES_ON 1
#endif
#endif

// If you're NOT using LuaJIT (typical on Android unless you built it):
#ifndef SOL_LUAJIT
#define SOL_LUAJIT 0
#endif

// If your build disables exceptions (common in some NDK setups), uncomment:
// #if defined(__ANDROID__) && !defined(__EXCEPTIONS)
//   #define SOL_NO_EXCEPTIONS 1
// #endif

// ---- Shield sol2 from hostile macros (cout/cerr/clog, min/max, etc.) ----
// Save & undef only for the duration of the include
#pragma push_macro("cout")
#pragma push_macro("cerr")
#pragma push_macro("clog")
#pragma push_macro("min")
#pragma push_macro("max")
#ifdef cout
#undef cout
#endif
#ifdef cerr
#undef cerr
#endif
#ifdef clog
#undef clog
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// Include sol2
#include <sol/sol.hpp>

// Restore macros
#pragma pop_macro("max")
#pragma pop_macro("min")
#pragma pop_macro("clog")
#pragma pop_macro("cerr")
#pragma pop_macro("cout")
