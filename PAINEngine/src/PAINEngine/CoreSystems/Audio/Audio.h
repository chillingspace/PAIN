#pragma once

#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "Applications/AppSystem.h"

namespace PAIN {
	namespace Audio {

        struct AudioChannelId { int value = -1; };
        inline bool operator==(AudioChannelId a, AudioChannelId b) { return a.value == b.value; }
        inline bool isValid(AudioChannelId ch) { return ch.value >= 0; }

        enum class AudioResult {
            Ok,
            NotInitialized,
            AlreadyInitialized,
            NotFound,
            InvalidArg,
            BackendError
        };

        struct PlaylistDesc {
            std::string name;
            std::vector<std::string> paths;
        };

        class Audio : public AppSystem {
        private:

        public:
            virtual ~Audio() = default;

            // lifetime
            virtual AudioResult init() = 0;
            virtual void shutdown() = 0;

            // assets
            virtual AudioResult loadSound(std::string_view path,
                bool is3D = true, bool looping = false, bool stream = false) = 0;
            virtual AudioResult loadPlaylist(const PlaylistDesc& desc) = 0;

            // playback
            virtual std::optional<AudioChannelId> play(std::string_view soundPath,
                const glm::vec3& pos = { 0,0,0 },
                float volumeDb = 0.0f) = 0;
            virtual std::optional<AudioChannelId> playRandom(std::string_view playlistName,
                const glm::vec3& pos = { 0,0,0 },
                float volumeDb = 0.0f) = 0;

            // control (dB)
            virtual AudioResult stop(AudioChannelId ch) = 0;
            virtual void        stopAll() = 0;
            virtual AudioResult setVolumeDb(AudioChannelId ch, float volumeDb) = 0;
            virtual AudioResult setVolumeLinear(AudioChannelId ch, float volume01) = 0;
            virtual AudioResult setPosition(AudioChannelId ch, const glm::vec3& pos) = 0;

            // listener
            virtual void setListener(const glm::vec3& pos,
                const glm::vec3& vel,
                const glm::vec3& fwd,
                const glm::vec3& up) = 0;

            // groups (built-in: "master","music","sfx","ui")
            virtual AudioResult setGroupVolumeDb(const char* group, float db) = 0;
            virtual AudioResult fadeGroupToDb(const char* group, float targetDb, float seconds) = 0;

            //Functions from app system interface
            void onAttach() override { init(); }
            void onDetach() override { shutdown(); }
            virtual void onUpdate(float dt) override = 0;
            virtual void onAppPause() override {}
            virtual void onAppResume() override {}

            virtual void onEvent(Event::Event& e) override {}

            //Static function to create audio class
            static Audio* create(void* app);
        };

	}
}

#endif