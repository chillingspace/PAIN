#include "ShaderManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

// Define LOG_TAG for this file
#define LOG_TAG "ShaderManager"

ShaderManager::ShaderManager() : initialized(false), shaderDirectory("")
#ifdef PLATFORM_ANDROID
, assetManager(nullptr)
#endif
{
    LOGI("ShaderManager constructor called");
}

ShaderManager::~ShaderManager() {
    LOGI("ShaderManager destructor called");
    cleanup();
}

bool ShaderManager::initialize(void* assetMgr, const std::string& customShaderDir) {
    LOGI("Initializing ShaderManager");
    
#ifdef PLATFORM_ANDROID
    if (assetMgr == nullptr) {
        setError("AssetManager is required on Android platform");
        return false;
    }
    assetManager = static_cast<AAssetManager*>(assetMgr);
    LOGI("AssetManager set for Android platform");
#else
    // On desktop, set custom shader directory if provided
    (void)assetMgr; // Suppress unused parameter warning
    if (!customShaderDir.empty()) {
        shaderDirectory = customShaderDir;
        // Ensure directory ends with separator
        if (shaderDirectory.back() != '/' && shaderDirectory.back() != '\\') {
            shaderDirectory += '/';
        }
        LOGI("Custom shader directory set: %s", shaderDirectory.c_str());
    } else {
        LOGI("Using default shader directory search");
    }
#endif

    initialized = true;
    LOGI("ShaderManager initialized successfully");
    return true;
}

std::string ShaderManager::loadShader(const std::string& filename) {
    if (!initialized) {
        setError("ShaderManager not initialized");
        return "";
    }

    LOGI("Loading shader file: %s", filename.c_str());

#ifdef PLATFORM_ANDROID
    return loadFileAndroid(filename);
#else
    return loadFileDesktop(filename);
#endif
}

GLuint ShaderManager::compileShader(const std::string& source, GLenum type) {
    if (source.empty()) {
        setError("Empty shader source code");
        return 0;
    }

    LOGI("Compiling shader (type: %s)", 
         type == GL_VERTEX_SHADER ? "vertex" : "fragment");

    // Create shader
    GLuint shader = glCreateShader(type);
    if (shader == 0) {
        setError("Failed to create shader");
        return 0;
    }

    // Set source and compile
    const char* sourcePtr = source.c_str();
    glShaderSource(shader, 1, &sourcePtr, nullptr);
    glCompileShader(shader);

    // Check compilation status
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetShaderInfoLog(shader, sizeof(infoLog), nullptr, infoLog);
        setError("Shader compilation failed: " + std::string(infoLog));
        glDeleteShader(shader);
        return 0;
    }

    LOGI("Shader compiled successfully");
    return shader;
}

GLuint ShaderManager::createProgram(GLuint vertexShader, GLuint fragmentShader) {
    if (vertexShader == 0 || fragmentShader == 0) {
        setError("Invalid shader IDs provided");
        return 0;
    }

    LOGI("Creating shader program");

    // Create program
    GLuint program = glCreateProgram();
    if (program == 0) {
        setError("Failed to create shader program");
        return 0;
    }

    // Attach shaders
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    // Link program
    glLinkProgram(program);

    // Check linking status
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        GLchar infoLog[1024];
        glGetProgramInfoLog(program, sizeof(infoLog), nullptr, infoLog);
        setError("Shader program linking failed: " + std::string(infoLog));
        glDeleteProgram(program);
        return 0;
    }

    LOGI("Shader program created successfully");
    return program;
}

GLuint ShaderManager::loadAndCompileShader(const std::string& filename, GLenum type) {
    std::string source = loadShader(filename);
    if (source.empty()) {
        return 0; // Error already set by loadShader
    }

    return compileShader(source, type);
}

GLuint ShaderManager::createProgramFromFiles(const std::string& vertexFilename, 
                                           const std::string& fragmentFilename) {
    LOGI("Creating program from files: %s, %s", 
         vertexFilename.c_str(), fragmentFilename.c_str());

    // Load and compile vertex shader
    GLuint vertexShader = loadAndCompileShader(vertexFilename, GL_VERTEX_SHADER);
    if (vertexShader == 0) {
        return 0; // Error already set
    }

    // Load and compile fragment shader
    GLuint fragmentShader = loadAndCompileShader(fragmentFilename, GL_FRAGMENT_SHADER);
    if (fragmentShader == 0) {
        glDeleteShader(vertexShader);
        return 0; // Error already set
    }

    // Create program
    GLuint program = createProgram(vertexShader, fragmentShader);
    
    // Clean up individual shaders (they're now part of the program)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

void ShaderManager::cleanup() {
    LOGI("Cleaning up ShaderManager");
    initialized = false;
    lastError.clear();
    
#ifdef PLATFORM_ANDROID
    assetManager = nullptr;
#endif
}

std::string ShaderManager::loadFileDesktop(const std::string& filename) {
    LOGI("Loading file from desktop filesystem: %s", filename.c_str());

    // Build list of possible paths
    std::vector<std::string> possiblePaths;
    
    // If custom shader directory is set, try it first
    if (!shaderDirectory.empty()) {
        possiblePaths.push_back(shaderDirectory + filename);
    }
    
    // Add other common paths
    possiblePaths.push_back(filename);  // Original path
    possiblePaths.push_back("my_gl_app/" + filename);  // Relative to project root
    possiblePaths.push_back("../" + filename);  // One level up
    possiblePaths.push_back("../my_gl_app/" + filename);  // One level up then into my_gl_app
    possiblePaths.push_back("shaders/" + filename);  // Direct shaders directory
    possiblePaths.push_back("my_gl_app/shaders/" + filename);  // my_gl_app/shaders/

    std::ifstream file;
    std::string actualPath;
    
    for (const auto& path : possiblePaths) {
        file.open(path);
        if (file.is_open()) {
            actualPath = path;
            LOGI("Found shader file at: %s", path.c_str());
            break;
        }
    }

    if (!file.is_open()) {
        std::string errorMsg = "Failed to open shader file. Tried paths: ";
        for (size_t i = 0; i < possiblePaths.size(); ++i) {
            errorMsg += possiblePaths[i];
            if (i < possiblePaths.size() - 1) errorMsg += ", ";
        }
        setError(errorMsg);
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    std::string content = buffer.str();
    LOGI("File loaded successfully from %s, size: %zu bytes", actualPath.c_str(), content.size());
    return content;
}

std::string ShaderManager::loadFileAndroid(const std::string& filename) {
    LOGI("Loading file from Android assets: %s", filename.c_str());

#ifdef PLATFORM_ANDROID
    if (assetManager == nullptr) {
        setError("AssetManager not initialized");
        return "";
    }

    // Open asset
    AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_BUFFER);
    if (asset == nullptr) {
        setError("Failed to open asset: " + filename);
        return "";
    }

    // Get file size
    off_t length = AAsset_getLength(asset);
    if (length <= 0) {
        AAsset_close(asset);
        setError("Invalid asset size: " + filename);
        return "";
    }

    // Read file content
    const char* buffer = static_cast<const char*>(AAsset_getBuffer(asset));
    if (buffer == nullptr) {
        AAsset_close(asset);
        setError("Failed to read asset buffer: " + filename);
        return "";
    }

    std::string content(buffer, length);
    AAsset_close(asset);

    LOGI("Asset loaded successfully, size: %zu bytes", content.size());
    return content;
#else
    setError("Android asset loading called on non-Android platform");
    return "";
#endif
}

void ShaderManager::setError(const std::string& error) {
    lastError = error;
    LOGE("ShaderManager error: %s", error.c_str());
}

bool ShaderManager::checkGLError(const std::string& operation) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        setError("OpenGL error in " + operation + ": " + std::to_string(error));
        return false;
    }
    return true;
}
