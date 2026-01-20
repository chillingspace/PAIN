// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for
// future builds. This also affects IntelliSense performance, including code
// completion and many code browsing features. However, files listed here are
// ALL re-compiled if any one of them is updated between builds. Do not add
// files here that you will be updating frequently as this negates the
// performance advantage.

#pragma once

#ifndef PCH_H
#define PCH_H

/*****************************************************************/ /**
                                                                     * PREDEFINED
                                                                     * Macros
                                                                     *********************************************************************/
#define CLASS_STR(T) #T // Convert Class To Str

// add headers that you want to pre-compile here

/*****************************************************************/ /**
                                                                     * Physics
                                                                     * Library
                                                                     *********************************************************************/
#include "Jolt/Jolt.h"
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

/*****************************************************************/ /**
                                                                     * Engine
                                                                     * Specific
                                                                     * Library
                                                                     *********************************************************************/

// shortcuts
#define GS GraphicsSettings::get()

// key events
#include "./Utility/KeyCodes.h"

// Android GL vs Window GL
#ifdef PN_PLATFORM_ANDROID
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <android/native_activity.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>

#else
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#endif

/*****************************************************************/ /**
                                                                     * Asset
                                                                     * Parser
                                                                     * includes
                                                                     *********************************************************************/
#ifdef PN_PLATFORM_WINDOWS
// #include "gli/gli.hpp"
#endif

// Math Lib
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

// Entt lib
#include <entt/entt.hpp>

// Memory
#define _CRTDBG_MAP_ALLOC

/*****************************************************************/ /**
                                                                     * Windows
                                                                     * Application
                                                                     *********************************************************************/
#ifdef PN_PLATFORM_WINDOWS
#include <Windows.h>
#endif

/*****************************************************************/ /**
                                                                     * Filewatcher
                                                                     *********************************************************************/
#ifdef PN_PLATFORM_WINDOWS
#include "FileWatch.hpp"
#endif

/*****************************************************************/ /**
                                                                     * CORE
                                                                     * HEADER
                                                                     *********************************************************************/
#include "Core.h"

/*****************************************************************/ /**
                                                                     * Seri
                                                                     * HEADER
                                                                     *********************************************************************/
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <refl.hpp>

/*****************************************************************/ /**
                                                                     * STL
                                                                     *********************************************************************/
#include <algorithm>
#include <array>
#include <bitset>
#include <cctype>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <optional>
#include <queue>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <thread>
#include <time.h>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

/*****************************************************************/ /**
                                                                     * UTILITY
                                                                     *********************************************************************/
#include "Utility/Log.h"

// Ban normal logging
// #define cout  PN_IOSTREAM_FORBIDDEN__use_logger_instead
// #define cerr  PN_IOSTREAM_FORBIDDEN__use_logger_instead

#ifdef PN_PLATFORM_ANDROID
#include "./Utility/AndroidFs.h"
#endif

#define glCheck(call)                                                          \
  call;                                                                        \
  {                                                                            \
    GLenum err = glGetError();                                                 \
    if (err != GL_NO_ERROR) {                                                  \
      PN_CORE_ERROR("OpenGL error {} at {}:{} - {}", err, __FILE__, __LINE__,  \
                    #call);                                                    \
    }                                                                          \
  }

#endif // PCH_H
