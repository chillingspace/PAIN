# PAIN Engine

[![Language: C++](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://isocpp.org/)
[![Graphics: OpenGL](https://img.shields.io/badge/Graphics-OpenGL-green.svg)](https://www.opengl.org/)
[![Platform: Windows | Android](https://img.shields.io/badge/Platform-Windows%20%7C%20Android-lightgrey.svg)]()
[![Contributors](https://img.shields.io/badge/Contributors-10-orange.svg)]()

> A cross-platform 3D game engine developed by DigiPen (Singapore) students, featuring custom OpenGL rendering pipeline, automated asset compilation, and multi-platform deployment capabilities.

## 📋 Table of Contents
- [Overview](#overview)
- [Key Features](#key-features)
- [Technical Stack](#technical-stack)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [Installation](#installation)
  - [Building for Windows](#building-for-windows)
  - [Building for Android](#building-for-android)
- [Project Structure](#project-structure)
- [Development Guidelines](#development-guidelines)
  - [Coding Conventions](#coding-conventions)
  - [GLSL Shader Standards](#glsl-shader-standards)
- [Engine Architecture](#engine-architecture)
- [API Reference](#api-reference)
- [Contributing](#contributing)
- [Team](#team)
- [License](#license)

---

## 🎮 Overview

PAIN is a lightweight, performance-focused 3D game engine built from scratch. The engine demonstrates modern C++ architecture, cross-platform deployment strategies, and custom graphics pipeline implementation using OpenGL/GLSL.

### Project Goals
- **Educational Excellence**: Showcase advanced understanding of game engine architecture and systems programming
- **Cross-Platform Development**: Single codebase deployment to Windows and Android platforms
- **Performance Optimization**: Efficient memory management with automated garbage collection
- **Clean Architecture**: Component-based design with clear separation of concerns

---

## ✨ Key Features

| Feature | Description |
|---------|-------------|
| **Custom Rendering Pipeline** | OpenGL-based 3D graphics with shader support and vertex/fragment processing |
| **Cross-Platform Support** | Native builds for Windows (Visual Studio) and Android (Android Studio) |
| **Asset Management System** | String-based asset referencing with compile-time processing |
| **Memory Management** | Automated garbage collection preventing memory leaks at program termination |
| **Component Architecture** | Modular, extensible design for game object management |
| **Debug Logging System** | Core logging utilities (`PN_CORE_*` macros) for development debugging |
| **Precompiled Headers** | Optimized compilation times via `pch.h` |

---

## 🛠️ Technical Stack

### Languages & APIs
- **Primary Language**: C++ 
- **Graphics API**: OpenGL 3.3+ / GLSL
- **Build System**: CMake 
- **Shaders**: GLSL 
- **Platform Tools**: Batch scripts, Java 

### Core Technologies
- **Rendering**: OpenGL with custom shader pipeline
- **Window Management**: GLFW / Platform-specific windowing
- **Mathematics**: Custom linear algebra implementation
- **Build Tools**: CMake, Visual Studio MSBuild, Gradle

### Target Platforms
- Windows 10/11 (x64)
- Android API 23+ (ARM/ARM64)

---

## 🚀 Getting Started

### Prerequisites

#### For Windows Development
- **Visual Studio 2019 or newer** with C++ development tools
- **CMake 3.15+**
- **Windows 10 SDK**
- **OpenGL 3.3+ compatible drivers**
- **Git** with submodule support

#### For Android Development
- **Android Studio Arctic Fox (2020.3.1) or newer**
- **Android NDK r21+**
- **Android SDK with API Level 23+**
- **Gradle 7.0+**

### Installation

```bash
# Clone the repository
git clone https://github.com/chillingspace/PAIN.git
cd PAIN

# Initialize and update all submodules (REQUIRED)
git submodule update --init --recursive

# Configure Git to automatically update submodules on pull
git config submodule.recurse true
```

### Building for Windows

```bash
# Run the automated build script
build.bat

# Navigate to the build directory
cd build

# Open the Visual Studio solution
start PAIN.sln

# Build and run from Visual Studio (F5 or Ctrl+F5)
```

**Note**: Assets are automatically compiled during the build process.

### Building for Android

```bash
# First, run the Windows build script to generate necessary files
build.bat

# Open Android Studio
# File → Open → Navigate to the 'android' folder in the project directory

# Sync Gradle files when prompted

# Build the project
# Build → Make Project (or click the Build icon)

# Run on device/emulator
# Run → Run 'app' (or Shift+F10)
```

---

## 📁 Project Structure

```
PAIN/
├── PAINENGINE/                        
│   ├── src/                   # All C++ source files (.cpp)
│
├── include/                    # Header files (.h)
│   ├── components/             # Game component definitions
│   └── systems/                # System interfaces         
│
├── assets/                     # Game assets
├── android/                    # Android-specific project files
│   ├── app/                    # Android app module
│   └── build.gradle            # Android build configuration
│
├── build/                      # Build output (generated, not in Git)
│   └── PAIN.sln               # Visual Studio solution (generated)
│
├── build.bat                   # Windows build script
├── CMakeLists.txt             # CMake configuration
└── README.md                   # This file
```

---

## 💻 Development Guidelines

### Coding Conventions

Our team follows strict naming conventions to maintain code consistency across 10 contributors.

#### Naming Standards

| Element | Convention | Example |
|---------|-----------|---------|
| **Variables** | `snake_case` | `int player_health;` |
| **Functions** | `camelCase` | `void updatePlayer();` |
| **Constructors/Destructors** | `camelCase` | `GameObject();` |
| **Classes** | `PascalCase` | `class GameEngine { };` |
| **Namespaces** | `PascalCase` | `namespace Graphics { }` |
| **Constants** | `SNAKE_CASE` | `const int MAX_ENTITIES = 1000;` |
| **Enums** | `SNAKE_CASE` | `enum State { IDLE, RUNNING };` |
| **Typedefs** | `SNAKE_CASE` | `typedef int ENTITY_ID;` |
| **Defines** | `SNAKE_CASE` | `#define MAX_BUFFER_SIZE 512` |

#### File Naming

- **General files**: `snake_case.cpp` (e.g., `game_manager.cpp`)
- **Class definition files**: `PascalCase.h` (e.g., `GameObject.h`)
- **Implementation files**: Match header name (e.g., `GameObject.cpp`)

#### Code Example

```cpp
// GameObject.h - PascalCase for class files
#pragma once
#include "pch.h"

namespace Core {
    class GameObject {
    private:
        const int MAX_COMPONENTS = 32;      // SNAKE_CASE for constants
        int entity_id;                       // snake_case for variables
        float position_x, position_y;

    public:
        GameObject();                        // camelCase for constructors
        ~GameObject();

        void updatePosition(float dx, float dy);  // camelCase for functions
        void renderSprite();

        int getEntityId() const { return entity_id; }
    };
}
```

### GLSL Shader Standards

#### Variable Naming
- **All shader variables**: `snake_case`
- **Vertex attributes**: `a_` prefix → `a_position`, `a_color`
- **Fragment attributes**: `f_` prefix → `f_color`, `f_tex_coord`
- **Uniform attributes**: `u_` prefix → `u_projection_matrix`, `u_time`

#### Standard Attribute Locations

**Vertex Shader Inputs**:
```glsl
layout(location = 0) in vec2 a_position;         // Vertex position (required)
layout(location = 1) in vec4 a_color;            // Vertex color
layout(location = 2) in vec2 a_tex_coord;        // Texture coordinates
```

**Fragment Shader Outputs**:
```glsl
layout(location = 0) out vec4 f_color;           // Final fragment color
```

#### Complete Shader Example

```glsl
// vertex_shader.vert
#version 330 core

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_tex_coord;

uniform mat4 u_projection_matrix;
uniform mat4 u_view_matrix;
uniform mat4 u_model_matrix;

out vec4 f_color;
out vec2 f_tex_coord;

void main() {
    gl_Position = u_projection_matrix * u_view_matrix * u_model_matrix * vec4(a_position, 0.0, 1.0);
    f_color = a_color;
    f_tex_coord = a_tex_coord;
}
```

```glsl
// fragment_shader.frag
#version 330 core

in vec4 f_color;
in vec2 f_tex_coord;

uniform sampler3D u_texture;
uniform float u_alpha;

out vec4 frag_color;

void main() {
    vec4 tex_color = texture(u_texture, f_tex_coord);
    frag_color = tex_color * f_color * vec4(1.0, 1.0, 1.0, u_alpha);
}
```

---

## 🏗️ Engine Architecture

### Core Systems

#### Asset Management
- **String-based referencing**: All assets/states use string identifiers for simplicity
- **Compile-time processing**: Assets are processed and optimized during build
- **Runtime loading**: Efficient asset streaming with caching

#### Memory Management
- **Automated garbage collection**: Runs at program termination to prevent leaks
- **Manual cleanup optional**: Critical resources can be manually managed
- **Debug tracking**: All allocations tracked in debug builds

#### Rendering Pipeline
1. Scene graph traversal
2. Culling and frustum checks
3. Batch rendering by material/texture
4. Shader binding and uniform updates
5. Draw call submission
6. Post-processing effects

### Component System
The engine uses a lightweight component-based architecture for game objects, allowing flexible composition of behaviors and properties.

---

## 📚 API Reference

### Core Logging System

Use the debug logging macros for runtime debugging:

```cpp
PN_CORE_INFO("Engine initialized successfully");
PN_CORE_WARN("Low memory warning: {0} MB remaining", memory_mb);
PN_CORE_ERROR("Failed to load texture: {0}", texture_path);
PN_CORE_TRACE("Player position: ({0}, {1})", pos_x, pos_y);
```

---

## 🤝 Contributing

### Code Quality Standards

Before pushing code to the repository:

1. **No warnings**: Code must compile without warnings
2. **No memory leaks**: Run memory leak detection tools
3. **Follow conventions**: Adhere to all naming and coding standards
4. **Test thoroughly**: Verify functionality on target platforms
5. **Ask for help**: If unsure, reach out to team members

### Development Workflow

```bash
# Pull latest changes before starting work
git pull origin main
git submodule update --remote

# Create a feature branch
git checkout -b feature/your-feature-name

# Make changes and commit frequently
git add .
git commit -m "feat: add player movement system"

# Push your branch
git push origin feature/your-feature-name

# Create a Pull Request for review
```

### Commit Message Format

Use conventional commit format:
- `feat:` New feature
- `fix:` Bug fix
- `docs:` Documentation changes
- `refactor:` Code refactoring
- `test:` Adding tests
- `chore:` Build process or tooling changes

---

## 👥 Team

This project is developed by a team of 12 DigiPen (Singapore) students as part of their game engine development project.

**Contributors**: [View all contributors](https://github.com/chillingspace/PAIN/graphs/contributors)

### Key Roles
- Engine Architecture
- Graphics Programming
- Physics Systems
- Audio Systems
- Asset Pipeline
- Platform Integration
- Tools Development

---

## 📄 License

This project is developed for educational purposes at DigiPen Institute of Technology Singapore.

---

## 🙏 Acknowledgments

- **DigiPen Institute of Technology Singapore** for curriculum and resources
- **OpenGL Community** for graphics API documentation
- **GLFW** for cross-platform window management
- All open-source libraries used in this project

---

## 📞 Contact & Support

For questions, bug reports, or feature requests:
- **Issues**: [GitHub Issues](https://github.com/chillingspace/PAIN/issues)
- **Repository**: [https://github.com/chillingspace/PAIN](https://github.com/chillingspace/PAIN)

---

## 🔍 Common Issues & Troubleshooting

### Build Failures

**Issue**: CMake configuration fails
```bash
# Solution: Update submodules
git submodule update --init --recursive
```

**Issue**: Missing dependencies on Windows
```bash
# Solution: Install Visual Studio C++ tools and Windows SDK
# Ensure OpenGL drivers are up to date
```

### Runtime Issues

**Issue**: Asset loading failures
- Verify asset paths use forward slashes (`/`)
- Check that assets are present in the `assets/` directory
- Review console logs using `PN_CORE_ERROR` output

**Issue**: OpenGL context errors
- Update graphics drivers
- Verify OpenGL 3.3+ support with `glxinfo` (Linux) or GPU specs
- Check shader compilation logs in debug output

### Android Build Issues

**Issue**: Gradle sync fails
- Verify Android NDK and SDK versions match prerequisites
- Clean and rebuild: `Build → Clean Project` then `Build → Make Project`

**Issue**: App crashes on startup
- Check logcat output for native crashes
- Verify all native libraries are included in APK
- Test on physical device (emulators may have OpenGL limitations)

---

**Note**: This engine is under active development. Features and APIs may change. Always refer to the latest documentation in the repository.

---

<div align="center">

**Made with 💻 and ☕ by DigiPen Singapore students**

[⬆ Back to Top](#pain-engine)

</div>
