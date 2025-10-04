#pragma once

#ifndef FMOD_AUDIO_HPP
#define FMOD_AUDIO_HPP

#include "Audio.h"

namespace PAIN {
	namespace Audio {

        class FmodAudio : public Audio {
        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;

#ifdef PN_PLATFORM_ANDROID
            //Only for android
            android_app* m_App = nullptr;

            void fmodJNIAttach();
            void fmodJNIDetach();
#endif

        public:
            FmodAudio(void* app);
            ~FmodAudio() override;

            AudioResult init() override;
            void shutdown() override;
            void onFixedUpdate(AppTiming timing) override {}
            void onUpdate(AppTiming timing) override;

            AudioResult loadSound(std::string_view, bool, bool, bool, float, float) override;
            AudioResult loadPlaylist(const PlaylistDesc&) override;

            std::optional<AudioChannelId> play(std::string_view, const glm::vec3&, float) override;
            std::optional<AudioChannelId> playRandom(std::string_view, const glm::vec3&, float) override;

            AudioResult stop(AudioChannelId) override;
            void        stopAll() override;
            AudioResult setVolumeDb(AudioChannelId, float) override;
            AudioResult setVolumeLinear(AudioChannelId, float) override;
            AudioResult setPosition(AudioChannelId, const glm::vec3&) override;

            void setListener(const glm::vec3&, const glm::vec3&,
                const glm::vec3&, const glm::vec3&) override;

            AudioResult setGroupVolumeDb(const char* group, float db) override;
            AudioResult fadeGroupToDb(const char* group, float targetDb, float seconds) override;

            void onAppPause() override;
            void onAppResume() override;
        };
	}
}

#endif