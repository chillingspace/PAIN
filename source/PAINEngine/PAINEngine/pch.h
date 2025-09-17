// pch.h: This is a precompiled header file.

#pragma once

#ifndef PCH_H
#define PCH_H

// --- Standard Library ---
#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <sstream>
#include <functional>
#include <unordered_map>
#include <stack>
// (Add any other frequently used STL headers here)

// --- Fetched Libraries (paths are now managed by CMake) ---
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

/*
// --- Manually Linked Libraries (Temporarily Disabled) ---
#include "Jolt/Jolt.h"
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/Body.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
*/

// --- Engine Core ---
#include "PAINEngine/Core.h"
#include "PAINEngine/Logging/Log.h"

#ifdef PN_PLATFORM_WINDOWS
    #include <Windows.h>
#endif

#endif //PCH_H