#pragma once

#ifndef C_AUDIO_SOURCE_H
#define C_AUDIO_SOURCE_H

#include "pch.h"
#include "CoreSystems/Audio/Audio.h"
#include "GLMSerialization.h"
#include "LayeredSystems/LevelEditor/Panels/ReflectionUI.h"
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

            // --- STATE (Managed by AudioSystem) ---
            AudioState state = AudioState::Stopped;

            bool playOnStart = true;  // NEW: Serialized - determines if sound plays when scene loads

            // --- TRIGGERS (Set by other systems/scripts) ---
            // Set to true to make the AudioSystem play this sound.
            // The system will reset this to false after processing.
            bool playTrigger = true;

            // Set to true to make the AudioSystem stop this sound.
            // The system will reset this to false after processing.
            bool stopTrigger = false;

            // --- RUNTIME (Internal handle) ---
            // Do not serialize or edit
            PAIN::Audio::AudioChannelId channelId{ -1 };
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
    //
    // Intentionally NOT reflecting the runtime fields:
    // - state
    // - stopTrigger
    // - channelId
    // These will be default-initialized when the component is loaded
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::Audio::AudioSource>);

#endif // C_AUDIO_SOURCE_H