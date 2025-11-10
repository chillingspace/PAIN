#pragma once

#ifndef AUDIO_HPP
#define AUDIO_HPP

#include "Applications/AppSystem.h"

#include "AssetTypes.h"

namespace PAIN {
    namespace Audio {

        struct AudioChannelId {
            int value = -1;
            bool isValid() const { return value >= 0; }
            friend bool operator==(const AudioChannelId& a, const AudioChannelId& b) { return a.value == b.value; }
        };

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
        
        //Sound defaults
        const float MIN_DISTANCE_3D = 1.0f;
        const float MAX_DISTANCE_3D = 50.0f;

        //Fix group names
        const std::vector<std::string> group_names {
            "music", "sfx", "ui"
        };

        struct Sound : public Assets::IAsset {
            virtual ~Sound() = default;
            virtual std::string getPath() const = 0;
            virtual void release() = 0;

            bool stream = false;
        };

        class Audio : public AppSystem {
        private:

        public:
            virtual ~Audio() = default;

            // lifetime
            virtual AudioResult init() = 0;
            virtual void shutdown() = 0;

            // assets
            virtual std::shared_ptr<Sound> createSound(std::string const& virtual_path) = 0;
            virtual AudioResult loadPlaylist(const PlaylistDesc& desc) = 0;

            // playback
            virtual std::optional<AudioChannelId> play(std::shared_ptr<Sound> sound, std::string const& group = "", float vol = 0.0f, float pitch = 0.0f, bool looping = false, bool is3D = false, const glm::vec3& pos = glm::vec3(0), float min_dist = MIN_DISTANCE_3D, float max_dist = MAX_DISTANCE_3D) = 0;
            virtual std::optional<AudioChannelId> playRandom(std::string_view playlistName,
                const glm::vec3& pos = { 0,0,0 },
                float volumeDb = 0.0f) = 0;

            // control (dB)
            virtual AudioResult stop(AudioChannelId ch) = 0;
            virtual void        stopAll() = 0;
            virtual AudioResult pauseChannel(AudioChannelId ch) = 0;
            virtual AudioResult resumeChannel(AudioChannelId ch) = 0;
            virtual void pauseAll() = 0;
            virtual void resumeAll() = 0;
            virtual AudioResult setMuteAll(bool mute) = 0;
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

            void onFixedUpdate(AppTiming timing) override {}
            virtual void onUpdate(AppTiming timing) override = 0;
            virtual void onAppPause() override {}
            virtual void onAppResume() override {}

            virtual void onEvent(Event::Event& e) override {}

            //Static function to create audio class
            static Audio* create(void* app);
        };

    }
}

#endif
