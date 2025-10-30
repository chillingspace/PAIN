#pragma once

#ifndef C_AUDIO_SOURCE_H
#define C_AUDIO_SOURCE_H

#include "pch.h"
#include "CoreSystems/Audio/Audio.h"
#include "GLMSerialization.h"
#include "LayeredSystems/LevelEditor/Panels/ReflectionUI.h"

namespace PAIN {
    namespace Audio {

        // Defines the current runtime state of the audio source
        enum class AudioState {
            Stopped,
            Playing,
            Paused
        };

        // Component to attach to an entity to make it play sound
        struct AudioSource {
            // --- CONFIGURATION (Set in Editor) ---
            std::string soundPath;
            bool is3D = true;
            bool looping = false;
            float volumeDb = 0.0f;
            float minDistance = 1.0f;
            float maxDistance = 50.0f;

            // --- STATE (Managed by AudioSystem) ---
            AudioState state = AudioState::Stopped;

            // --- TRIGGERS (Set by other systems/scripts) ---
            // Set to true to make the AudioSystem play this sound.
            // The system will reset this to false after processing.
            bool playTrigger = false;

            // Set to true to make the AudioSystem stop this sound.
            // The system will reset this to false after processing.
            bool stopTrigger = false;

            // --- RUNTIME (Internal handle) ---
            // Do not serialize or edit
            PAIN::Audio::AudioChannelId channelId{ -1 };
        };

    } // namespace Audio
} // namespace PAIN

// --- JSON SERIALIZATION ---
//NLOHMANN_JSON_SERIALIZE_ENUM(PAIN::Audio::AudioState, {
//    {PAIN::Audio::AudioState::Stopped, "Stopped"},
//    {PAIN::Audio::AudioState::Playing, "Playing"},
//    {PAIN::Audio::AudioState::Paused, "Paused"}
//})
//
//namespace nlohmann {
//    template<>
//    struct adl_serializer<PAIN::Audio::AudioSource> {
//        static void to_json(json& j, const PAIN::Audio::AudioSource& src) {
//            j = json{
//                {"soundPath", src.soundPath},
//                {"is3D", src.is3D},
//                {"looping", src.looping},
//                {"volumeDb", src.volumeDb},
//                {"minDistance", src.minDistance},
//                {"maxDistance", src.maxDistance},
//                {"playOnAwake", src.playTrigger} // Save playTrigger as playOnAwake
//            };
//            // Do not serialize runtime state (state, channelId, stopTrigger)
//        }
//
//        static void from_json(const json& j, PAIN::Audio::AudioSource& src) {
//            j.at("soundPath").get_to(src.soundPath);
//            j.at("is3D").get_to(src.is3D);
//            j.at("looping").get_to(src.looping);
//            j.at("volumeDb").get_to(src.volumeDb);
//            j.at("minDistance").get_to(src.minDistance);
//            j.at("maxDistance").get_to(src.maxDistance);
//            
//            // If playOnAwake was saved, set the playTrigger for the system to catch on load
//            if (j.contains("playOnAwake")) {
//                j.at("playOnAwake").get_to(src.playTrigger);
//            }
//
//            // Reset runtime state
//            src.state = PAIN::Audio::AudioState::Stopped;
//            src.stopTrigger = false;
//            src.channelId = { -1 };
//        }
//    };
//}

REFL_TYPE(PAIN::Audio::AudioSource)
    REFL_FIELD(soundPath)
    REFL_FIELD(is3D)
    REFL_FIELD(looping)
    REFL_FIELD(volumeDb)
    REFL_FIELD(minDistance)
    REFL_FIELD(maxDistance)
    REFL_FIELD(state)
    REFL_FIELD(playTrigger)
    REFL_FIELD(stopTrigger)
REFL_END

#endif // C_AUDIO_SOURCE_H