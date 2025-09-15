#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

// Use GLM for vector math
#include "glm/glm.hpp"

// Include the FMOD header that defines FMOD_VECTOR
#include <fmod_common.h>

// Forward-declare FMOD class types
namespace FMOD {
    class System;
    class Sound;
    class Channel;
}

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    void Initialize(int maxChannels = 512);
    void Shutdown();
    void Update(); // Must be called every game loop frame

    void LoadSound(const std::string& soundPath, bool is3D = true, bool isLooping = false, bool stream = false);
    void LoadPlaylist(const std::string& playlistName, const std::vector<std::string>& soundPaths);

    int PlaySound(const std::string& soundPath, const glm::vec3& position = glm::vec3(0), float volumeDb = 0.0f);
    int PlayRandomFromPlaylist(const std::string& playlistName, const glm::vec3& position = glm::vec3(0), float volumeDb = 0.0f);

    void SetListener(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& forward, const glm::vec3& up);

    void StopChannel(int channelId);
    void StopAllChannels();
    void SetChannelVolume(int channelId, float volumeDb);
    void SetChannelPosition(int channelId, const glm::vec3& position);

private:
    float dbToVolume(float db) const;
    static FMOD_VECTOR toFMODVector(const glm::vec3& vec);

    FMOD::System* m_system;
    int m_nextChannelId;

    std::unordered_map<std::string, FMOD::Sound*> m_sounds;
    std::unordered_map<std::string, std::vector<FMOD::Sound*>> m_playlists;
    std::unordered_map<int, FMOD::Channel*> m_channels;
};