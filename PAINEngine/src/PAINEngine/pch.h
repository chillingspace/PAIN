// pch.h: This is a precompiled header file.
#pragma once

#ifndef PCH_H
#define PCH_H

 /*****************************************************************//**
 * PREDEFINED Macros
 *********************************************************************/
#define CLASS_STR(T) #T //Convert Class To Str

// --- Platform Specific Includes ---
#if defined(PN_PLATFORM_WINDOWS)
    // Windows Desktop: Include GLEW and GLFW for OpenGL
    #include <GL/glew.h>
    #include <GLFW/glfw3.h>
    #include <Windows.h>
    #undef APIENTRY // To prevent conflict with Jolt
#elif defined(PLATFORM_ANDROID)
    // Android: Include GLES3 headers
    #include <GLES3/gl3.h>
#endif

 /*****************************************************************//**
 * Physics Library
 *********************************************************************/
#include "Jolt/Jolt.h"
#include <Jolt/Core/Factory.h>          
#include <Jolt/RegisterTypes.h>         
#include <Jolt/Physics/PhysicsSystem.h> 
#include <Jolt/Physics/Body/Body.h>     
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h> 
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

 /*****************************************************************//**
 * Engine Specific Library
 *********************************************************************/

// Math Lib
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"

// Imgui headers
#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/headers/imgui.h"

#if defined(PN_PLATFORM_WINDOWS)
    #include "ImGui/headers/imgui_impl_opengl3.h"
    #include "ImGui/headers/imgui_impl_glfw.h"
#elif defined(PLATFORM_ANDROID)
    #include "ImGui/headers/imgui_impl_opengl3.h"
#endif

//Memory
#define _CRTDBG_MAP_ALLOC

 /*****************************************************************//**
 * CORE HEADER
 *********************************************************************/
#include "Core.h"

 /*****************************************************************//**
 * Seri HEADER
 *********************************************************************/
#include "nlohmann/json.hpp"
using json = nlohmann::json;

 /*****************************************************************//**
 * STL
 *********************************************************************/
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <array>
#include <string>
#include <unordered_map>
#include <cctype>
#include <sstream>
#include <time.h>
#include <list>
#include <memory>
#include <cmath>
#include <iterator>
#include <queue>
#include <set>
#include <unordered_set>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <thread>
#include <optional>
#include <stack>
#include <regex>
#include <limits>
#include <random>
#include <bitset>

 /*****************************************************************//**
 * LOGGING
 *********************************************************************/
#include "Logging/Log.h"

//Ban normal logging
#define cout  PN_IOSTREAM_FORBIDDEN__use_logger_instead
#define cerr  PN_IOSTREAM_FORBIDDEN__use_logger_instead

#endif //PCH_H