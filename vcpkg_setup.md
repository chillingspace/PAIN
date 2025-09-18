# Android Build Guide with vcpkg

This guide explains how to build the Android version of this project using vcpkg for dependency management.

## Prerequisites

### Required Software
- **Android Studio** (latest version)
- **Android NDK** (version 27.0.12077973 or compatible)
- **vcpkg** installed at `C:\vcpkg\vcpkg`
- **CMake** (comes with Android Studio)

### Environment Setup
- Android NDK should be installed at your system based on your installation: In my case let suppose it is installed at: `E:\Users\Parminder\AppData\Local\Android\Sdk\ndk\27.0.12077973`
- vcpkg should be installed at: `<yourgame>\libs\vcpkg`
  - This is installed by the `script_vcpkg_setup.bat`


## Project Structure

```
project/
├── android/
│   └── app/
│       └── src/
│           └── main/
│               └── cpp/
│                   └── CMakeLists.txt          # Android CMake configuration
├── cmake/
│   ├── vcpkg_android.cmake                     # vcpkg Android integration
│   └── vcpkg_windows.cmake                     # vcpkg Windows management
├── libs/
│   └── vcpkg/
├── vcpkg.json                                  # vcpkg manifest
└── script_vcpkg_setup.bat                     # Build script
```

## vcpkg Configuration

### Android Triplets
The project uses custom Android triplets for vcpkg:

- **x64-android**: For Android emulator (x86_64 architecture)
- **arm64-android**: For real Android devices (ARM64 architecture)

These triplets are automatically copied to your system vcpkg installation.

### Dependencies
The project uses the following vcpkg packages:
- **imgui[opengl3-binding]**: For Android builds (no GLFW binding needed)
- **imgui[opengl3-binding,glfw-binding]**: For Windows builds

## Building the Project

Here’s a short intro you can paste above the script or send to anyone running it:

------

# How to run this batch file (what it does & what you’ll see)

**What this script does (Windows):**

- Run `script_vcpkg_setup.bat`

- Uses your **current folder** as the project root, creates `libs\` if needed, and clones **vcpkg** into `libs\vcpkg`.

- Sets **`ANDROID_NDK_HOME`** (and `ANDROID_NDK`) **for this session only** to your NDK path.

- Bootstraps `vcpkg.exe` if it isn’t built yet.

- Installs **Dear ImGui** with the OpenGL3 backend for **Android ARM64** and **Android x64** (emulator) using **classic mode**:

  - `imgui[opengl3-binding]:arm64-android`
  - `imgui[opengl3-binding]:x64-android`

  > Please note for Android it seems the vcpkg.json cannot be read(need to explore more options), there we need to install manually through script and fetch the android specific libs.

**Prerequisites:**

- **Git for Windows** on your PATH.
- Internet access (to clone vcpkg and fetch ports).
- A valid Android **NDK** at the path already hard-coded in the script (edit it if yours differs).

**How to run it:**

1. Place `script_vcpkg_setup.bat` in your **project root** (the folder that should contain `libs\`).

2. Open **Command Prompt** (or “x64 Native Tools Command Prompt for VS 2022”).

3. Run:

   ```
   call script_vcpkg_setup.bat
   ```

   (Using `call` keeps your terminal open to read messages.)

**How to verify it worked:**

- Check that `libs\vcpkg\vcpkg.exe` exists.
- Check `libs\vcpkg\installed\arm64-android\include\imgui.h` and `libs\vcpkg\installed\x64-android\include\imgui.h`.

### Windows Build

Create a build folder and execute cmake command to build the Native or PC project. The vcpkg configuration will automatically:

- Make use of `<root proj>/cmake/vcpkg_windows.cmake` to automate vcpkg
- It will automatically install the libraries for Windows with the help of `vcpkg.json`
- In this example, we are using `ImGUI` with `GLFW` support. Link the correct ImGui libraries
- If you notice we commented out `Imgui` and `GLFW` libraries from CMake because we want to use `vcpkg`

### Android Build

1. **Open Android Studio**
2. **Open the project** from the root directory (location of `android` folder in your project)
3. **Build the project** using Android Studio's build system

The vcpkg configuration will automatically:
- Make use of `<root proj>/cmake/vcpkg_android.cmake` to automate vcpkg

- Please not when we run `script_vcpkg_setup.bat`automatically install the libraries for Android for example `imgui` libs.

  - > Please note `vcpkg.json` is not used for Android (not sure if its a limitation)

- Set up the correct toolchain chaining (vcpkg → Android NDK)

- Use the appropriate Android triplet based on target architecture

- Link the correct ImGui libraries

The project supports building for different Android architectures:

| Architecture | Triplet | Use Case |
|--------------|---------|----------|
| x86_64 | x64-android | Android emulator |
| ARM64 | arm64-android | Real Android devices |

## References

- [vcpkg Android Support](https://learn.microsoft.com/en-us/vcpkg/users/platforms/android)
- [Android NDK CMake Guide](https://developer.android.com/ndk/guides/cmake)
- [ImGui Documentation](https://github.com/ocornut/imgui)
