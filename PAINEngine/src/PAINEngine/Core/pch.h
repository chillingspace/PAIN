// pch.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#pragma once

#ifndef PCH_H
#define PCH_H

// add headers that you want to pre-compile here

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

 /*****************************************************************//**
 * Engine Specific Library
 *********************************************************************/

// graphics headers
#include "GL/glew.h"
#include "GLFW/glfw3.h"

// GLM headers
#include "glm/glm.hpp"
#include <glm/gtc/quaternion.hpp>

// Imgui headers
#include "ImGui/headers/imgui.h"
#include "ImGui/headers/imgui_impl_opengl3.h"
#include "ImGui/headers/imgui_impl_glfw.h"

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
#include <sstream>
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

#undef APIENTRY

#endif //PCH_H
