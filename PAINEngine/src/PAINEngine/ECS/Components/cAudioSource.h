#pragma once

#ifndef C_AUDIO_SOURCE_H
#define C_AUDIO_SOURCE_H

#include "pch.h"
#include "CoreSystems/Audio/Audio.h"
#include "GLMSerialization.h"
#include <refl.hpp>
#include "LayeredSystems/LevelEditor/EditorAttributes.h"

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
            std::vector<std::string> audio_paths_storage;
            Assets::GUID selected_audio;
            std::string group_name;
            bool is3D = true;
            bool looping = false;
            float volumeDb = 0.0f;
            float pitchDb = 0.0f;
            float minDistance = 1.0f;
            float maxDistance = 50.0f;
            glm::vec3 pos = glm::vec3(0);

            // --- MULTI-TRACK SUPPORT (For Global BGM) ---
            // List of audio assets for multi-track playback
            std::vector<Assets::GUID> audio_tracks;
            // Individual volume for each track (dB, serialized)
            std::vector<float> track_volumes;

            // --- STATE (Managed by AudioSystem) ---
            AudioState state = AudioState::Stopped;

            bool playOnStart = false;  // NEW: Serialized - determines if sound plays when scene loads

            bool hasStarted = false; // internal
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
            // Runtime channel handles for multi-track (not serialized)
            std::vector<PAIN::Audio::AudioChannelId> track_channel_ids;

            //Serialization flag
            static constexpr bool ShouldSerialize = true;
        };


    } // namespace Audio
} // namespace PAIN

// --- REFLECTION ---
REFL_TYPE(PAIN::Audio::AudioSource)
    REFL_FIELD(selected_audio,
    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Audio),
    PAIN::Editor::Attributes::DisplayName("Select a Audio asset"),
    PAIN::Editor::Attributes::Tooltip("Select a Audio asset"))
    REFL_FIELD(group_name)
    REFL_FIELD(is3D)          // Serialized
    REFL_FIELD(looping)       // Serialized
    REFL_FIELD(volumeDb)      // Serialized
    REFL_FIELD(pitchDb)      // Serialized
    REFL_FIELD(minDistance)   // Serialized
    REFL_FIELD(maxDistance)   // Serialized
    REFL_FIELD(pos)   // Serialized
    REFL_FIELD(playOnStart)   // Serialized (will save as "playOnStart")
    // Multi-track support for Global BGM
    REFL_FIELD(audio_tracks)  // Serialized list of audio assets
    REFL_FIELD(track_volumes) // Serialized per-track volumes
    //
    // Intentionally NOT reflecting the runtime fields:
    // - state
    // - stopTrigger
    // - channelId
    // - track_channel_ids
    // These will be default-initialized when the component is loaded
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Audio::AudioSource>);

#endif // C_AUDIO_SOURCE_H