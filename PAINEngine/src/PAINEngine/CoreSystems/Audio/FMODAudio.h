#pragma once

#ifndef FMOD_AUDIO_HPP
#define FMOD_AUDIO_HPP

#include "Audio.h"
#include "CoreSystems/Path/Path.h"

namespace PAIN {
    namespace Audio {

        class FmodAudio final : public Audio {
        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;

#ifdef PN_PLATFORM_ANDROID
            //Only for android
            android_app* m_App = nullptr;

            void fmodJNIAttach();
            void fmodJNIDetach();
#endif

            //Private path service
            std::shared_ptr<Path::Path> path_service;

        public:
            FmodAudio(void* app);
            ~FmodAudio() override;

            AudioResult init() override;
            void shutdown() override;
            void onFixedUpdate(AppTiming timing) override {}
            void onUpdate(AppTiming timing) override;

            std::shared_ptr<Sound> createSound(std::string const&) override;
            AudioResult loadPlaylist(const PlaylistDesc&) override;

            std::optional<AudioChannelId> play(std::shared_ptr<Sound> sound, std::string const& group, float vol, float pitch, bool looping, bool is3D, const glm::vec3& pos, float min_dist, float max_dist) override;
            std::optional<AudioChannelId> playRandom(std::string_view, const glm::vec3&, float) override;

            // New direct file playback methods
            std::optional<AudioChannelId> playFile(const std::string& filename, 
                const std::string& group, float volumeDb, bool looping, bool is3D, 
                const glm::vec3& pos, float minDist, float maxDist) override;
            std::optional<AudioChannelId> playSFX(const std::string& filename, 
                bool looping, float volumeDb) override;
            std::optional<AudioChannelId> playSFXAt(const std::string& filename,
                const glm::vec3& pos, float volumeDb, bool looping,
                float minDist, float maxDist) override;
            std::optional<AudioChannelId> playRandomFromList(
                const std::vector<std::string>& files, float volumeDb, bool is3D,
                const glm::vec3& pos, float minDist, float maxDist) override;
            std::optional<AudioChannelId> playBGM(const std::string& filename, 
                bool overlay, float volumeDb) override;
            void transitionBGM(const std::string& newFilename, 
                float transitionTime, float volumeDb) override;
            void transitionBGMWithSFX(const std::string& newBGMFilename,
                const std::string& sfxFilename, float transitionTime, float volumeDb) override;


            AudioResult stop(AudioChannelId) override;
            bool isChannelValid(AudioChannelId ch) override;
            void        stopAll() override;
            AudioResult pauseChannel(AudioChannelId) override;
            AudioResult resumeChannel(AudioChannelId) override;
            void        pauseAll() override;
            void        resumeAll() override;
            AudioResult setVolumeDb(AudioChannelId, float) override;
            AudioResult setVolumeLinear(AudioChannelId, float) override;
            AudioResult setMuteAll(bool mute) override;
            AudioResult setPosition(AudioChannelId, const glm::vec3&) override;

            void setListener(const glm::vec3&, const glm::vec3&,
                const glm::vec3&, const glm::vec3&) override;

            std::vector<std::string> getAllGroups() override;

            float getGroupVolumeDb(const char* group) override;
            AudioResult setGroupVolumeDb(const char* group, float db) override;
            AudioResult fadeGroupToDb(const char* group, float targetDb, float seconds) override;
            bool checkGroupIsPlaying(const char* group) override;
            bool checkGroupPaused(const char* group) override;
            void pauseGroup(const char* group) override;
            void resumeGroup(const char* group) override;
            void stopGroup(const char* group) override;

            void onAppPause() override;
            void onAppResume() override;
        };
    }
}

#endif
