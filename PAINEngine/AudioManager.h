#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

// Forward declarations for FMOD
struct FMOD_SYSTEM;
struct FMOD_SOUND;
struct FMOD_CHANNEL;

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    // Initialize and cleanup
    bool initialize(void* assetManager = nullptr);
    void shutdown();
    void update(); // Call this every frame

    // Sound loading and management
    bool loadSound(const std::string& name, const std::string& filePath, bool loop = false);
    void unloadSound(const std::string& name);
    void unloadAllSounds();

    // Sound playback
    bool playSound(const std::string& name, float volume = 1.0f, float pitch = 1.0f);
    bool playSound3D(const std::string& name, float x, float y, float z, float volume = 1.0f);
    void stopSound(const std::string& name);
    void stopAllSounds();
    void pauseSound(const std::string& name, bool pause = true);
    void pauseAllSounds(bool pause = true);

    // Volume and pitch control
    void setMasterVolume(float volume);
    void setSoundVolume(const std::string& name, float volume);
    void setSoundPitch(const std::string& name, float pitch);

    // 3D audio settings
    void setListenerPosition(float x, float y, float z);
    void setListenerOrientation(float forwardX, float forwardY, float forwardZ,
                               float upX, float upY, float upZ);

    // Utility functions
    bool isSoundLoaded(const std::string& name) const;
    bool isSoundPlaying(const std::string& name) const;
    std::vector<std::string> getLoadedSounds() const;
    
#ifdef PLATFORM_ANDROID
    // Android-specific asset copying to internal storage
    std::string copyAssetToInternalStorage(const std::string& assetPath);
#endif

private:
    FMOD_SYSTEM* m_system;
    std::unordered_map<std::string, FMOD_SOUND*> m_sounds;
    // Removed m_channels - using FMOD example approach with single channel
    void* m_assetManager; // For Android asset loading
    
    // Helper functions
    std::string getFullPath(const std::string& fileName) const;
    void checkFMODError(int result, const std::string& operation) const;
};
