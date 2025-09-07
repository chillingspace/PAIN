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

 /*****************************************************************//**
 * Engine Specific Library
 *********************************************************************/

// graphics headers
#include "GL/glew.h"
#include "GLFW/glfw3.h"

// Imgui headers
#include "ImGui/headers/imgui.h"
#include "ImGui/headers/imgui_impl_opengl3.h"
#include "ImGui/headers/imgui_impl_glfw.h"

#undef APIENTRY

#endif //PCH_H
