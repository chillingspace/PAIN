#include "AudioManager.h"
#include <iostream>
#include <filesystem>
//#include <unistd.h>  // for getcwd

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#define LOG_TAG "AudioManager"
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
#else
#define LOGI(fmt, ...) std::cout << fmt << std::endl
#define LOGE(fmt, ...) std::cerr << fmt << std::endl
#define LOGD(fmt, ...) std::cout << fmt << std::endl
#endif

// Include FMOD headers
// Temporarily disable FMOD for Android to test basic rendering
#ifndef DISABLE_FMOD
#ifdef PLATFORM_ANDROID
    #include "fmod_android.h"
    #include "fmod.h"
    #include "fmod_errors.h"
#else
    #include "fmod.h"
    #include "fmod_errors.h"
#endif
#endif

AudioManager::AudioManager() : m_system(nullptr), m_assetManager(nullptr) {
}

#ifdef PLATFORM_ANDROID
std::string AudioManager::copyAssetToInternalStorage(const std::string& assetPath) {
    if (!m_assetManager) {
        LOGE("Asset manager is null, cannot copy asset: %s", assetPath.c_str());
        return "";
    }
    
    AAssetManager* assetMgr = static_cast<AAssetManager*>(m_assetManager);
    AAsset* asset = AAssetManager_open(assetMgr, assetPath.c_str(), AASSET_MODE_STREAMING);
    
    if (!asset) {
        LOGE("Failed to open asset: %s", assetPath.c_str());
        return "";
    }
    
    // Create internal storage path
    std::string internalPath = "/data/data/com.example.jnicpp/files/" + assetPath;
    
    // Create directory if it doesn't exist
    std::string dirPath = internalPath.substr(0, internalPath.find_last_of('/'));
    std::filesystem::create_directories(dirPath);
    
    // Open file for writing
    FILE* file = fopen(internalPath.c_str(), "wb");
    if (!file) {
        LOGE("Failed to create internal file: %s", internalPath.c_str());
        AAsset_close(asset);
        return "";
    }
    
    // Copy asset data
    const void* buffer = AAsset_getBuffer(asset);
    off_t length = AAsset_getLength(asset);
    
    if (buffer && length > 0) {
        size_t written = fwrite(buffer, 1, length, file);
        if (written != length) {
            LOGE("Failed to write all data to internal file: %s", internalPath.c_str());
            fclose(file);
            AAsset_close(asset);
            return "";
        }
    } else {
        LOGE("Asset buffer is null or length is 0 for: %s", assetPath.c_str());
        fclose(file);
        AAsset_close(asset);
        return "";
    }
    
    fclose(file);
    AAsset_close(asset);
    
    LOGI("Successfully copied asset %s to %s", assetPath.c_str(), internalPath.c_str());
    return internalPath;
}
#endif

AudioManager::~AudioManager() {
    shutdown();
}

bool AudioManager::initialize(void* assetManager) {
#ifdef DISABLE_FMOD
    // FMOD disabled - just store asset manager and return success
    m_assetManager = assetManager;
    LOGI("%s", "AudioManager initialized (FMOD disabled)");
    return true;
#else
    // Create FMOD system
    FMOD_RESULT result = FMOD_System_Create(&m_system, FMOD_VERSION);
    if (result != FMOD_OK) {
        LOGE("Failed to create FMOD system: %s", FMOD_ErrorString(result));
        return false;
    }

    // Initialize FMOD system
#ifdef PLATFORM_ANDROID
    // For Android, use different initialization parameters
    result = FMOD_System_Init(m_system, 32, FMOD_INIT_NORMAL, nullptr);
#else
    result = FMOD_System_Init(m_system, 32, FMOD_INIT_NORMAL, nullptr);
#endif
    if (result != FMOD_OK) {
        LOGE("Failed to initialize FMOD system: %s", FMOD_ErrorString(result));
        return false;
    }
    LOGI("FMOD system initialized successfully");

    // Store asset manager for Android
    m_assetManager = assetManager;
    
#ifdef PLATFORM_ANDROID
    // Set up Android-specific FMOD settings
    if (assetManager) {
        result = FMOD_System_SetOutput(m_system, FMOD_OUTPUTTYPE_AUDIOTRACK);
        if (result != FMOD_OK) {
            LOGE("Failed to set FMOD output type: %s", FMOD_ErrorString(result));
        } else {
            LOGI("Successfully set FMOD output to AUDIOTRACK");
        }
        
        // Try to set the Android asset manager if available
        // This might be needed for proper asset loading
        LOGI("Attempting to set Android asset manager in FMOD");
        
        // Try to set the asset manager - this might be needed for asset loading
        // Note: This function might not be available in all FMOD versions
        // We'll try it and if it fails, we'll continue with the simple path approach
        LOGI("Asset manager pointer: %p", assetManager);
    } else {
        LOGE("Android asset manager is null!");
    }
#endif

    LOGI("%s", "AudioManager initialized successfully");
    
    // Test FMOD system status
    int numDrivers = 0;
    FMOD_System_GetNumDrivers(m_system, &numDrivers);
    LOGI("FMOD: Found %d audio drivers", numDrivers);
    
    return true;
#endif
}

void AudioManager::shutdown() {
#ifdef DISABLE_FMOD
    LOGI("%s", "AudioManager shutdown (FMOD disabled)");
#else
    if (m_system) {
        // Stop all sounds
        stopAllSounds();
        
        // Unload all sounds
        unloadAllSounds();
        
        // Close and release FMOD system
        FMOD_System_Close(m_system);
        FMOD_System_Release(m_system);
        m_system = nullptr;
        
        LOGI("%s", "AudioManager shutdown complete");
    }
#endif
}

void AudioManager::update() {
#ifdef DISABLE_FMOD
    // FMOD disabled - do nothing
#else
    if (m_system) {
        FMOD_RESULT result = FMOD_System_Update(m_system);
        if (result != FMOD_OK) {
            LOGE("FMOD_System_Update failed: %s", FMOD_ErrorString(result));
        }
        // No need to manage individual channels - FMOD handles this automatically
    } else {
        LOGE("FMOD system is null in update()");
    }
#endif
}

bool AudioManager::loadSound(const std::string& name, const std::string& filePath, bool loop) {
#ifdef DISABLE_FMOD
    LOGI("loadSound called (FMOD disabled): %s", name.c_str());
    return true;
#else
    LOGI("loadSound called: name='%s', filePath='%s', loop=%s", name.c_str(), filePath.c_str(), loop ? "true" : "false");
    
#ifdef PLATFORM_ANDROID
    // Debug: Log that we're loading sound for Android
    LOGI("Loading sound for Android platform");
#endif
    
    if (!m_system) {
        LOGE("AudioManager not initialized");
        return false;
    }

    // Check if sound is already loaded
    if (m_sounds.find(name) != m_sounds.end()) {
        LOGI("Sound '%s' is already loaded", name.c_str());
        return true;
    }

    FMOD_SOUND* sound = nullptr;
    FMOD_MODE mode = FMOD_DEFAULT;
    
    if (loop) {
        mode |= FMOD_LOOP_NORMAL;
    }
    
#ifdef PLATFORM_ANDROID
    // For Android, add CREATESTREAM flag which is often needed
    mode |= FMOD_CREATESTREAM;
    LOGI("Added FMOD_CREATESTREAM flag for Android");
#endif
    
    LOGI("FMOD mode flags: %d (loop: %s)", mode, loop ? "true" : "false");

    FMOD_RESULT result;
#ifdef PLATFORM_ANDROID
    // For Android, copy asset to internal storage first
    std::string assetPath = "audio/sfx/" + filePath;
    LOGI("Copying Android asset to internal storage: %s", assetPath.c_str());
    
    std::string internalPath = copyAssetToInternalStorage(assetPath);
    if (internalPath.empty()) {
        LOGE("Failed to copy asset to internal storage: %s", assetPath.c_str());
        return false;
    }
    
    LOGI("Loading sound from internal storage: %s", internalPath.c_str());
    result = FMOD_System_CreateSound(m_system, internalPath.c_str(), mode, nullptr, &sound);
    if (result != FMOD_OK) {
        LOGE("Failed to load sound from internal storage '%s': %s", internalPath.c_str(), FMOD_ErrorString(result));
        return false;
    }
    LOGI("Successfully loaded sound from internal storage: %s", internalPath.c_str());
    LOGI("Sound loaded with mode: %d", mode);
#else
    // For PC, use file system path
    std::string fullPath = getFullPath(filePath);
    
    // Check if file exists
    if (!std::filesystem::exists(fullPath)) {
        LOGE("Audio file not found: %s", fullPath.c_str());
        return false;
    }
    
    result = FMOD_System_CreateSound(m_system, fullPath.c_str(), mode, nullptr, &sound);
#endif
    if (result != FMOD_OK) {
        LOGE("Failed to load sound '%s': %s", name.c_str(), FMOD_ErrorString(result));
        return false;
    }

    m_sounds[name] = sound;
    LOGI("Successfully loaded sound: %s, total sounds loaded: %zu", name.c_str(), m_sounds.size());
#ifdef PLATFORM_ANDROID
    LOGI("Loaded sound: %s (Android asset)", name.c_str());
#else
    LOGI("Loaded sound: %s from %s", name.c_str(), fullPath.c_str());
#endif
    return true;
#endif
}

void AudioManager::unloadSound(const std::string& name) {
#ifdef DISABLE_FMOD
    LOGI("unloadSound called (FMOD disabled): %s", name.c_str());
#else
    auto it = m_sounds.find(name);
    if (it != m_sounds.end()) {
        // Stop the sound if it's playing
        stopSound(name);
        
        // Release the sound
        FMOD_Sound_Release(it->second);
        m_sounds.erase(it);
        
        LOGI("Unloaded sound: %s", name.c_str());
    }
#endif
}

void AudioManager::unloadAllSounds() {
#ifdef DISABLE_FMOD
    LOGI("%s", "unloadAllSounds called (FMOD disabled)");
#else
    for (auto& pair : m_sounds) {
        FMOD_Sound_Release(pair.second);
    }
    m_sounds.clear();
    // No need to clear channels - using FMOD example approach
    LOGI("%s", "Unloaded all sounds");
#endif
}

bool AudioManager::playSound(const std::string& name, float volume, float pitch) {
#ifdef DISABLE_FMOD
    LOGI("playSound called (FMOD disabled): %s vol=%.2f pitch=%.2f", name.c_str(), volume, pitch);
    return true;
#else
    static int playSoundCallCount = 0;
    playSoundCallCount++;
    LOGI("=== PLAYSOUND CALLED #%d: %s vol=%.2f pitch=%.2f ===", playSoundCallCount, name.c_str(), volume, pitch);
    
    if (!m_system) {
        LOGE("AudioManager not initialized");
        return false;
    }

    auto it = m_sounds.find(name);
    if (it == m_sounds.end()) {
        std::string availableSounds;
        for (const auto& sound : m_sounds) {
            availableSounds += sound.first + " ";
        }
        LOGE("Sound '%s' not loaded. Available sounds: %s", name.c_str(), availableSounds.c_str());
        return false;
    }

    // Use FMOD example approach: single channel for all sounds
    FMOD_CHANNEL* channel = nullptr;
    LOGI("=== ATTEMPTING TO PLAY SOUND: %s ===", name.c_str());
    
    // Play sound using the same channel approach as FMOD example
    FMOD_RESULT result = FMOD_System_PlaySound(m_system, it->second, nullptr, false, &channel);
    if (result != FMOD_OK) {
        LOGE("Failed to play sound '%s': %s", name.c_str(), FMOD_ErrorString(result));
        return false;
    }

    // Set volume and pitch
    if (channel) {
        FMOD_Channel_SetVolume(channel, volume);
        FMOD_Channel_SetPitch(channel, pitch);
        LOGI("=== SUCCESSFULLY PLAYING SOUND: %s ===", name.c_str());
        LOGI("Channel pointer: %p, Volume: %.2f, Pitch: %.2f", channel, volume, pitch);
    } else {
        LOGE("No channel returned for sound: %s", name.c_str());
    }
    
    return true;
#endif
}

bool AudioManager::playSound3D(const std::string& name, float x, float y, float z, float volume) {
#ifdef DISABLE_FMOD
    LOGI("playSound3D called (FMOD disabled): %s at (%.2f, %.2f, %.2f)", name.c_str(), x, y, z);
    return true;
#else
    if (!m_system) {
        LOGE("AudioManager not initialized");
        return false;
    }

    auto it = m_sounds.find(name);
    if (it == m_sounds.end()) {
        LOGE("Sound '%s' not loaded", name.c_str());
        return false;
    }

    FMOD_CHANNEL* channel = nullptr;
    FMOD_RESULT result = FMOD_System_PlaySound(m_system, it->second, nullptr, false, &channel);
    if (result != FMOD_OK) {
        LOGE("Failed to play 3D sound '%s': %s", name.c_str(), FMOD_ErrorString(result));
        return false;
    }

    // Set 3D position
    FMOD_VECTOR position = { x, y, z };
    FMOD_Channel_Set3DAttributes(channel, &position, nullptr);
    FMOD_Channel_SetVolume(channel, volume);

    // No need to store channel - using FMOD example approach
    LOGI("Playing 3D sound: %s at (%.2f, %.2f, %.2f)", name.c_str(), x, y, z);
    return true;
#endif
}

void AudioManager::stopSound(const std::string& name) {
#ifdef DISABLE_FMOD
    LOGI("stopSound called (FMOD disabled): %s", name.c_str());
#else
    // Using FMOD example approach - can't stop individual sounds by name
    // This would require tracking channels, which we're avoiding
    LOGI("stopSound called for: %s (not implemented with FMOD example approach)", name.c_str());
#endif
}

void AudioManager::stopAllSounds() {
#ifdef DISABLE_FMOD
    LOGI("%s", "stopAllSounds called (FMOD disabled)");
#else
    LOGI("=== STOPALLSOUNDS CALLED ===");
    // FMOD doesn't have a direct "stop all sounds" function
    // We'll just log this for now since we're using the FMOD example approach
    LOGI("=== STOPALLSOUNDS - Using FMOD example approach (no action needed) ===");
#endif
}

void AudioManager::pauseSound(const std::string& name, bool pause) {
#ifdef DISABLE_FMOD
    LOGI("pauseSound called (FMOD disabled): %s pause=%s", name.c_str(), pause ? "true" : "false");
#else
    // Using FMOD example approach - can't pause individual sounds by name
    LOGI("pauseSound called for: %s (not implemented with FMOD example approach)", name.c_str());
#endif
}

void AudioManager::pauseAllSounds(bool pause) {
#ifdef DISABLE_FMOD
    LOGI("pauseAllSounds called (FMOD disabled): pause=%s", pause ? "true" : "false");
#else
    // Using FMOD example approach - can't pause all sounds without tracking channels
    LOGI("pauseAllSounds called (not implemented with FMOD example approach): pause=%s", pause ? "true" : "false");
#endif
}

void AudioManager::setMasterVolume(float volume) {
#ifdef DISABLE_FMOD
    LOGI("setMasterVolume called (FMOD disabled): %.2f", volume);
#else
    if (m_system) {
        FMOD_CHANNELGROUP* masterGroup = nullptr;
        FMOD_RESULT result = FMOD_System_GetMasterChannelGroup(m_system, &masterGroup);
        if (result == FMOD_OK && masterGroup) {
            FMOD_ChannelGroup_SetVolume(masterGroup, volume);
            LOGI("Set master volume to: %.2f", volume);
        } else {
            LOGE("Failed to get master channel group: %s", FMOD_ErrorString(result));
        }
    }
#endif
}

void AudioManager::setSoundVolume(const std::string& name, float volume) {
#ifdef DISABLE_FMOD
    LOGI("setSoundVolume called (FMOD disabled): %s vol=%.2f", name.c_str(), volume);
#else
    // Using FMOD example approach - can't set volume for individual sounds by name
    LOGI("setSoundVolume called for: %s (not implemented with FMOD example approach)", name.c_str());
#endif
}

void AudioManager::setSoundPitch(const std::string& name, float pitch) {
#ifdef DISABLE_FMOD
    LOGI("setSoundPitch called (FMOD disabled): %s pitch=%.2f", name.c_str(), pitch);
#else
    // Using FMOD example approach - can't set pitch for individual sounds by name
    LOGI("setSoundPitch called for: %s (not implemented with FMOD example approach)", name.c_str());
#endif
}

void AudioManager::setListenerPosition(float x, float y, float z) {
#ifdef DISABLE_FMOD
    LOGI("setListenerPosition called (FMOD disabled): (%.2f, %.2f, %.2f)", x, y, z);
#else
    if (m_system) {
        FMOD_VECTOR position = { x, y, z };
        FMOD_VECTOR velocity = { 0, 0, 0 };
        FMOD_VECTOR forward = { 0, 0, 1 };
        FMOD_VECTOR up = { 0, 1, 0 };
        
        FMOD_System_Set3DListenerAttributes(m_system, 0, &position, &velocity, &forward, &up);
    }
#endif
}

void AudioManager::setListenerOrientation(float forwardX, float forwardY, float forwardZ,
                                         float upX, float upY, float upZ) {
#ifdef DISABLE_FMOD
    LOGI("%s", "setListenerOrientation called (FMOD disabled)");
#else
    if (m_system) {
        FMOD_VECTOR position = { 0, 0, 0 };
        FMOD_VECTOR velocity = { 0, 0, 0 };
        FMOD_VECTOR forward = { forwardX, forwardY, forwardZ };
        FMOD_VECTOR up = { upX, upY, upZ };
        
        FMOD_System_Set3DListenerAttributes(m_system, 0, &position, &velocity, &forward, &up);
    }
#endif
}

bool AudioManager::isSoundLoaded(const std::string& name) const {
#ifdef DISABLE_FMOD
    return false; // Always return false when FMOD is disabled
#else
    return m_sounds.find(name) != m_sounds.end();
#endif
}

bool AudioManager::isSoundPlaying(const std::string& name) const {
#ifdef DISABLE_FMOD
    return false; // Always return false when FMOD is disabled
#else
    // Using FMOD example approach - can't check if individual sounds are playing by name
    LOGI("isSoundPlaying called for: %s (not implemented with FMOD example approach)", name.c_str());
    return false;
#endif
}

std::vector<std::string> AudioManager::getLoadedSounds() const {
#ifdef DISABLE_FMOD
    return std::vector<std::string>(); // Return empty vector when FMOD is disabled
#else
    std::vector<std::string> sounds;
    for (const auto& pair : m_sounds) {
        sounds.push_back(pair.first);
    }
    return sounds;
#endif
}

std::string AudioManager::getFullPath(const std::string& fileName) const {
#ifdef PLATFORM_ANDROID
    // For Android, this method is not used since we copy assets to internal storage
    // This is only used for PC builds
    return "audio/sfx/" + fileName;
#else
    // For PC, try to find the audio file in the game-assets directory
    std::filesystem::path currentPath = std::filesystem::current_path();
    
    // Try different possible paths
    std::vector<std::filesystem::path> possiblePaths = {
        currentPath / "game-assests" / "audio" / "sfx" / fileName,
        currentPath / ".." / "game-assests" / "audio" / "sfx" / fileName,
        currentPath / ".." / ".." / "game-assests" / "audio" / "sfx" / fileName,
        currentPath / ".." / ".." / ".." / "game-assests" / "audio" / "sfx" / fileName
    };
    
    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path)) {
            return path.string();
        }
    }
    
    // If not found, return the original filename
    return fileName;
#endif
}

void AudioManager::checkFMODError(int result, const std::string& operation) const {
#ifdef DISABLE_FMOD
    // Do nothing when FMOD is disabled
#else
    if (result != FMOD_OK) {
        LOGE("FMOD Error in %s: %s", operation.c_str(), FMOD_ErrorString(static_cast<FMOD_RESULT>(result)));
    }
#endif
}