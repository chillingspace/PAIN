// NO LONGER A PRECOMPILED FILE
// Android does not support precompiled headers, this is now just a standard header file

#pragma once
#ifndef PCH_H
#define PCH_H

// Graphics
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// ImGui (using local headers)
#include "ImGui/headers/imgui.h"
#include "ImGui/headers/imgui_impl_glfw.h"
#include "ImGui/headers/imgui_impl_opengl3.h"

// Physics
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/Body.h>

// Logging
#include <spdlog/spdlog.h>
#include "Logging/Log.h"

// Standard Library
#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

#endif //PCH_H