#pragma once

#include "Logging/Log.h"
#include "Applications/AppSystem.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>
#include <fmod_common.h>

namespace FMOD {
    class System;
    class Sound;
    class Channel;
}

class AudioManager : public PAIN::AppSystem {
public:
	AudioManager();
	~AudioManager();

	// AppSystem virtual function overrides
	void onAttach() override;
	void onDetach() override;
	void onUpdate() override;
	void onEvent(PAIN::Event::Event& e) override;

	// Public audio API
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