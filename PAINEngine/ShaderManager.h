#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include "Platform.h"
#include <string>
#include <unordered_map>
#include <memory>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

/**
 * ShaderManager - Cross-platform shader loading and management
 * 
 * This class provides a unified interface for loading shader files on both
 * desktop and Android platforms:
 * 
 * Desktop (Windows/Linux/Mac):
 * - Loads shader files from the file system using standard file I/O
 * - Supports relative and absolute paths
 * 
 * Android:
 * - Loads shader files from the APK assets using AssetManager
 * - Files must be placed in the assets/ directory in the Android project
 * - Supports the same file structure as desktop for consistency
 * 
 * Usage:
 * 1. Initialize with platform-specific parameters
 * 2. Load shader files using loadShader()
 * 3. Compile shaders using compileShader()
 * 4. Create shader programs using createProgram()
 * 5. Clean up resources when done
 */
class ShaderManager {
public:
    ShaderManager();
    ~ShaderManager();

    /**
     * Initialize the shader manager
     * @param assetManager Android AssetManager (only needed on Android)
     * @param shaderDirectory Custom shader directory for desktop (optional)
     * @return true if initialization successful
     */
    bool initialize(void* assetManager = nullptr, const std::string& shaderDirectory = "");

    /**
     * Load shader source code from file
     * @param filename Path to shader file (relative to assets/ on Android)
     * @return Shader source code as string, empty string on failure
     */
    std::string loadShader(const std::string& filename);

    /**
     * Compile a shader from source code
     * @param source Shader source code
     * @param type Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
     * @return OpenGL shader ID, 0 on failure
     */
    GLuint compileShader(const std::string& source, GLenum type);

    /**
     * Create a shader program from vertex and fragment shaders
     * @param vertexShader Vertex shader ID
     * @param fragmentShader Fragment shader ID
     * @return OpenGL program ID, 0 on failure
     */
    GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);

    /**
     * Load and compile a shader from file
     * @param filename Path to shader file
     * @param type Shader type (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER)
     * @return OpenGL shader ID, 0 on failure
     */
    GLuint loadAndCompileShader(const std::string& filename, GLenum type);

    /**
     * Create a complete shader program from two shader files
     * @param vertexFilename Path to vertex shader file
     * @param fragmentFilename Path to fragment shader file
     * @return OpenGL program ID, 0 on failure
     */
    GLuint createProgramFromFiles(const std::string& vertexFilename, 
                                 const std::string& fragmentFilename);

    /**
     * Get the last error message
     * @return Error message string
     */
    const std::string& getLastError() const { return lastError; }

    /**
     * Check if the manager is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized; }

    /**
     * Clean up all resources
     */
    void cleanup();

private:
    bool initialized;
    std::string lastError;
    std::string shaderDirectory;
    
#ifdef PLATFORM_ANDROID
    AAssetManager* assetManager;
#endif

    /**
     * Load file from desktop file system
     * @param filename File path
     * @return File contents as string
     */
    std::string loadFileDesktop(const std::string& filename);

    /**
     * Load file from Android assets
     * @param filename Asset file path
     * @return File contents as string
     */
    std::string loadFileAndroid(const std::string& filename);

    /**
     * Set error message
     * @param error Error message
     */
    void setError(const std::string& error);

    /**
     * Check for OpenGL errors and set error message if found
     * @param operation Description of the operation being checked
     * @return true if no errors, false if error found
     */
    bool checkGLError(const std::string& operation);
};

#endif // SHADERMANAGER_H
