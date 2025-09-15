#include "pch.h"
#include "AudioManager.h"
#include "PAINEngine/Logging/Log.h" // Include your engine's Log header

#include <fmod.hpp>
#include <fmod_errors.h>
#include <cmath>
#include <random>

// Helper to check for FMOD errors and log them using your engine's logger
#define FMOD_CHECK(result) PAIN_FMOD_CHECK(result, __FILE__, __LINE__)
bool PAIN_FMOD_CHECK(FMOD_RESULT result, const char* file, int line)
{
    if (result != FMOD_OK)
    {
        // Using the Core logger for engine-level errors
        PN_CORE_ERROR("FMOD Error: [{0}] {1} in {2}:{3}", result, FMOD_ErrorString(result), file, line);
        return false;
    }
    return true;
}

FMOD_VECTOR AudioManager::toFMODVector(const glm::vec3& vec) {
    return { vec.x, vec.y, vec.z };
}

AudioManager::AudioManager() 
    : m_system(nullptr), m_nextChannelId(0) {}

AudioManager::~AudioManager() {
    Shutdown();
}

void AudioManager::Initialize(int maxChannels) {
    if (!FMOD_CHECK(FMOD::System_Create(&m_system))) return;

    if (!FMOD_CHECK(m_system->init(maxChannels, FMOD_INIT_NORMAL | FMOD_INIT_3D_RIGHTHANDED, nullptr))) {
        m_system->release();
        m_system = nullptr;
        return;
    }

    m_system->set3DSettings(1.0f, 1.0f, 1.0f);
    
    PN_CORE_INFO("Audio Manager Initialized.");
}

void AudioManager::Shutdown() {
    if (m_system) {
        for (auto const& [key, val] : m_sounds) { val->release(); }
        m_sounds.clear();

        for (auto const& [key, val] : m_playlists) {
            for (auto* sound : val) { sound->release(); }
        }
        m_playlists.clear();

        m_system->close();
        m_system->release();
        m_system = nullptr;
        PN_CORE_INFO("Audio Manager Shutdown.");
    }
}

void AudioManager::Update() {
    if (!m_system) return;

    std::vector<int> stoppedChannels;
    for (auto const& [id, channel] : m_channels) {
        bool isPlaying = false;
        if (channel) channel->isPlaying(&isPlaying);
        if (!isPlaying) stoppedChannels.push_back(id);
    }
    for (int id : stoppedChannels) {
        m_channels.erase(id);
    }

    m_system->update();
}

void AudioManager::LoadSound(const std::string& soundPath, bool is3D, bool isLooping, bool stream) {
    if (m_sounds.count(soundPath)) return;

    FMOD_MODE mode = FMOD_DEFAULT;
    if (is3D) mode |= FMOD_3D;
    if (isLooping) mode |= FMOD_LOOP_NORMAL;
    if (stream) mode |= FMOD_CREATESTREAM;

    FMOD::Sound* sound = nullptr;
    if (FMOD_CHECK(m_system->createSound(soundPath.c_str(), mode, nullptr, &sound))) {
        m_sounds[soundPath] = sound;
    }
}

void AudioManager::LoadPlaylist(const std::string& playlistName, const std::vector<std::string>& soundPaths) {
    if (m_playlists.count(playlistName)) return;

    std::vector<FMOD::Sound*> sounds;
    for (const auto& path : soundPaths) {
        FMOD::Sound* sound = nullptr;
        if (FMOD_CHECK(m_system->createSound(path.c_str(), FMOD_3D, nullptr, &sound))) {
            sounds.push_back(sound);
        }
    }
    m_playlists[playlistName] = sounds;
}

int AudioManager::PlaySound(const std::string& soundPath, const glm::vec3& position, float volumeDb) {
    auto it = m_sounds.find(soundPath);
    if (it == m_sounds.end()) {
        PN_CORE_WARN("Audio Error: Sound not loaded: {0}", soundPath);
        return -1;
    }

    FMOD::Channel* channel = nullptr;
    if (FMOD_CHECK(m_system->playSound(it->second, nullptr, true, &channel))) {
        FMOD_VECTOR pos = toFMODVector(position);
        FMOD_VECTOR vel = {0, 0, 0};
        
        channel->set3DAttributes(&pos, &vel);
        channel->setVolume(dbToVolume(volumeDb));
        channel->setPaused(false);
        
        int channelId = m_nextChannelId++;
        m_channels[channelId] = channel;
        return channelId;
    }
    return -1;
}

int AudioManager::PlayRandomFromPlaylist(const std::string& playlistName, const glm::vec3& position, float volumeDb) {
    auto it = m_playlists.find(playlistName);
    if (it == m_playlists.end() || it->second.empty()) {
        PN_CORE_WARN("Audio Error: Playlist not found or empty: {0}", playlistName);
        return -1;
    }

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, it->second.size() - 1);
    FMOD::Sound* soundToPlay = it->second[dist(rng)];

    FMOD::Channel* channel = nullptr;
    if (FMOD_CHECK(m_system->playSound(soundToPlay, nullptr, true, &channel))) {
        FMOD_VECTOR pos = toFMODVector(position);
        FMOD_VECTOR vel = {0, 0, 0};

        channel->set3DAttributes(&pos, &vel);
        channel->setVolume(dbToVolume(volumeDb));
        channel->setPaused(false);

        int channelId = m_nextChannelId++;
        m_channels[channelId] = channel;
        return channelId;
    }
    return -1;
}

void AudioManager::SetListener(const glm::vec3& position, const glm::vec3& velocity, const glm::vec3& forward, const glm::vec3& up) {
    if (!m_system) return;
    
    FMOD_VECTOR pos = toFMODVector(position);
    FMOD_VECTOR vel = toFMODVector(velocity);
    FMOD_VECTOR fwd = toFMODVector(forward);
    FMOD_VECTOR up_vec = toFMODVector(up);

    m_system->set3DListenerAttributes(0, &pos, &vel, &fwd, &up_vec);
}

void AudioManager::StopChannel(int channelId) {
    auto it = m_channels.find(channelId);
    if (it != m_channels.end() && it->second) {
        it->second->stop();
    }
}

void AudioManager::StopAllChannels() {
    for (auto const& [id, channel] : m_channels) {
        if (channel) channel->stop();
    }
}

void AudioManager::SetChannelVolume(int channelId, float volumeDb) {
    auto it = m_channels.find(channelId);
    if (it != m_channels.end() && it->second) {
        it->second->setVolume(dbToVolume(volumeDb));
    }
}

void AudioManager::SetChannelPosition(int channelId, const glm::vec3& position) {
    auto it = m_channels.find(channelId);
    if (it != m_channels.end() && it->second) {
        FMOD_VECTOR pos = toFMODVector(position);
        it->second->set3DAttributes(&pos, nullptr);
    }
}

float AudioManager::dbToVolume(float db) const {
    return powf(10.0f, 0.05f * db);
}