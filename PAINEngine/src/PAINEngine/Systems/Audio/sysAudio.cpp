#include "pch.h"
#include "sysAudio.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cAudioSource.h"
#include "ECS/Components/cEntity.h"

namespace PAIN {
    namespace Audio {

        // ==================== PERSISTENT GLOBAL AUDIO STORAGE ====================
        // Static storage for global audio that persists across scene changes
        
        struct GlobalAudioTrack {
            AudioChannelId channelId{ -1 };
            Assets::GUID assetGuid;
            float currentVolume = -80.0f;
            float targetVolume = -80.0f;
            float fadeSpeed = 0.0f;         // dB per second (0 = no fade)
            bool isActive = false;
        };
        
        static std::vector<GlobalAudioTrack> s_globalTracks;
        static bool s_globalAudioInitialized = false;
        
        // Fade management
        struct FadeRequest {
            int trackIndex;
            float targetDb;
            float durationSeconds;
        };
        static std::vector<FadeRequest> s_pendingFades;

        // ==================== PUBLIC GLOBAL AUDIO API ====================
        // These are accessed by Lua bindings
        
        int GlobalAudio_GetTrackCount() {
            return static_cast<int>(s_globalTracks.size());
        }
        
        float GlobalAudio_GetVolume(int index) {
            if (index < 0 || index >= static_cast<int>(s_globalTracks.size())) {
                return -80.0f;
            }
            return s_globalTracks[index].currentVolume;
        }
        
        void GlobalAudio_SetVolume(int index, float volumeDb, PAIN::Audio::Audio* audioService) {
            if (index < 0 || index >= static_cast<int>(s_globalTracks.size())) {
                PN_CORE_WARN("[GlobalAudio] SetVolume: Invalid track index {}", index);
                return;
            }
            auto& track = s_globalTracks[index];
            track.currentVolume = volumeDb;
            track.targetVolume = volumeDb;
            track.fadeSpeed = 0.0f; // Cancel any ongoing fade
            
            if (track.channelId.isValid() && audioService) {
                audioService->setVolumeDb(track.channelId, volumeDb);
            }
        }
        
        void GlobalAudio_Fade(int index, float targetDb, float durationSeconds) {
            if (index < 0 || index >= static_cast<int>(s_globalTracks.size())) {
                PN_CORE_WARN("[GlobalAudio] Fade: Invalid track index {}", index);
                return;
            }
            if (durationSeconds <= 0.0f) {
                // Instant set
                s_pendingFades.push_back({ index, targetDb, 0.01f });
                return;
            }
            
            auto& track = s_globalTracks[index];
            track.targetVolume = targetDb;
            float deltaDb = targetDb - track.currentVolume;
            track.fadeSpeed = deltaDb / durationSeconds;
            
            PN_CORE_INFO("[GlobalAudio] Fade track {} from {:.1f} to {:.1f} dB over {:.2f}s", 
                index, track.currentVolume, targetDb, durationSeconds);
        }
        
        void GlobalAudio_StopAll(PAIN::Audio::Audio* audioService) {
            for (auto& track : s_globalTracks) {
                if (track.channelId.isValid() && audioService) {
                    audioService->stop(track.channelId);
                }
                track.channelId = { -1 };
                track.isActive = false;
            }
            s_globalTracks.clear();
            s_globalAudioInitialized = false;
            PN_CORE_INFO("[GlobalAudio] All tracks stopped and cleared");
        }
        
        void GlobalAudio_Clear() {
            // Keep tracks playing but mark as not initialized so new scene can take over
            s_globalAudioInitialized = false;
            PN_CORE_INFO("[GlobalAudio] Cleared initialization flag for new track set");
        }
        
        bool GlobalAudio_IsInitialized() {
            return s_globalAudioInitialized;
        }
        
        std::vector<GlobalAudioTrack>& GlobalAudio_GetTracks() {
            return s_globalTracks;
        }
        
        // ==================== SYSTEM IMPLEMENTATION ====================

        System::System(std::shared_ptr<Services> svc) : ISystem(svc)
        {
            PN_CORE_INFO("Audio System Initialized.");
        }

        void System::onUpdate(AppTiming timing, entt::registry& registry)
        {
            auto audioService = getServices()->get<PAIN::Audio::Audio>();
            auto scene = getServices()->get<Scene::SceneManager>();
            auto pathService = getServices()->get<Path::Path>();
            auto asset_service = getServices()->get<Assets::Manager>();

            if (!audioService || !scene) {
                return; // Services not ready
            }

            float dt = timing.dt;

            // ==================== UPDATE GLOBAL AUDIO FADES ====================
            for (auto& track : s_globalTracks) {
                if (track.fadeSpeed != 0.0f && track.isActive) {
                    float prevVolume = track.currentVolume;
                    track.currentVolume += track.fadeSpeed * dt;
                    
                    // Check if we've reached target
                    bool reachedTarget = false;
                    if (track.fadeSpeed > 0.0f && track.currentVolume >= track.targetVolume) {
                        track.currentVolume = track.targetVolume;
                        reachedTarget = true;
                    } else if (track.fadeSpeed < 0.0f && track.currentVolume <= track.targetVolume) {
                        track.currentVolume = track.targetVolume;
                        reachedTarget = true;
                    }
                    
                    if (reachedTarget) {
                        track.fadeSpeed = 0.0f;
                        PN_CORE_INFO("[GlobalAudio] Track fade complete, volume now {:.1f} dB", track.currentVolume);
                    }
                    
                    // Apply volume to channel
                    if (track.channelId.isValid()) {
                        audioService->setVolumeDb(track.channelId, track.currentVolume);
                    }
                }
            }

            // 1. Update Listener Position from Camera
            Camera* camera = scene->GetActiveCamera();
            if (camera) {
                audioService->setListener(camera->pos, { 0,0,0 }, camera->forward, camera->up);
            }

            // 2. Iterate all entities with AudioSource and Transform
            auto group = registry.group<AudioSource>(entt::get< WorldTransform>);
            
            for (auto [entity, audioSrc, worldtransform] : group.each())
            {
                // Check if this is the Global_BGM entity (by name)
                bool isGlobalBGM = false;
                if (registry.all_of<Entity::Name>(entity)) {
                    const auto& nameComp = registry.get<Entity::Name>(entity);
                    if (nameComp.name == "Global_BGM") {
                        isGlobalBGM = true;
                    }
                }

                // Handle Global_BGM entity with multi-track audio
                if (isGlobalBGM && !audioSrc.audio_tracks.empty()) {
                    // Check if these tracks match what's already playing globally
                    bool tracksMatch = (s_globalTracks.size() == audioSrc.audio_tracks.size());
                    if (tracksMatch) {
                        for (size_t i = 0; i < audioSrc.audio_tracks.size() && tracksMatch; ++i) {
                            if (s_globalTracks[i].assetGuid != audioSrc.audio_tracks[i]) {
                                tracksMatch = false;
                            }
                        }
                    }
                    
                    if (tracksMatch && s_globalAudioInitialized) {
                        // Same tracks already playing, sync entity's channel IDs with static storage
                        audioSrc.track_channel_ids.clear();
                        for (const auto& track : s_globalTracks) {
                            audioSrc.track_channel_ids.push_back(track.channelId);
                        }
                        audioSrc.hasStarted = true;
                        audioSrc.state = AudioState::Playing;
                        continue; // Audio already playing, nothing to start
                    }
                    
                    // New or different tracks - start them
                    if (!audioSrc.hasStarted || !tracksMatch) {
                        PN_CORE_INFO("[AudioSystem] Starting Global BGM with {} tracks (persistent)", audioSrc.audio_tracks.size());
                        
                        // Clear previous global tracks (but don't stop if crossfading)
                        // For Option B crossfade, we keep old playing until new ones are ready
                        std::vector<GlobalAudioTrack> oldTracks = s_globalTracks;
                        s_globalTracks.clear();
                        audioSrc.track_channel_ids.clear();
                        
                        // Ensure track_volumes has correct size
                        while (audioSrc.track_volumes.size() < audioSrc.audio_tracks.size()) {
                            audioSrc.track_volumes.push_back(-80.0f);
                        }
                        
                        // Start all new tracks at muted volume
                        for (size_t i = 0; i < audioSrc.audio_tracks.size(); ++i) {
                            auto audio_opt = asset_service->getAsset<Sound>(audioSrc.audio_tracks[i]);
                            if (audio_opt.has_value()) {
                                auto channelOpt = audioService->play(
                                    audio_opt.value(),
                                    "music",
                                    -80.0f,     // Start muted
                                    0.0f,
                                    true,       // Looping
                                    false       // Not 3D
                                );
                                
                                GlobalAudioTrack newTrack;
                                newTrack.assetGuid = audioSrc.audio_tracks[i];
                                newTrack.currentVolume = -80.0f;
                                newTrack.targetVolume = -80.0f;
                                newTrack.fadeSpeed = 0.0f;
                                newTrack.isActive = true;
                                
                                if (channelOpt.has_value()) {
                                    newTrack.channelId = channelOpt.value();
                                    audioSrc.track_channel_ids.push_back(channelOpt.value());
                                    PN_CORE_INFO("[AudioSystem] Global BGM track {} started on channel {}", 
                                        i, channelOpt.value().value);
                                } else {
                                    newTrack.channelId = { -1 };
                                    audioSrc.track_channel_ids.push_back(AudioChannelId{-1});
                                    PN_CORE_WARN("[AudioSystem] Failed to start Global BGM track {}", i);
                                }
                                
                                s_globalTracks.push_back(newTrack);
                            } else {
                                GlobalAudioTrack emptyTrack;
                                emptyTrack.channelId = { -1 };
                                emptyTrack.isActive = false;
                                s_globalTracks.push_back(emptyTrack);
                                audioSrc.track_channel_ids.push_back(AudioChannelId{-1});
                                PN_CORE_WARN("[AudioSystem] Global BGM track {} asset not found", i);
                            }
                        }
                        
                        // Stop old tracks now that new ones are playing (Option B crossfade)
                        for (auto& old : oldTracks) {
                            if (old.channelId.isValid()) {
                                audioService->stop(old.channelId);
                            }
                        }
                        
                        audioSrc.hasStarted = true;
                        audioSrc.state = AudioState::Playing;
                        s_globalAudioInitialized = true;
                    }
                    continue; // Skip normal processing for Global_BGM
                }

                if (audioSrc.playOnStart && !audioSrc.hasStarted) {
                    audioSrc.playTrigger = true;
                    audioSrc.hasStarted = true;
                }

                // 3. Handle Play/Stop Triggers
                if (audioSrc.playTrigger)
                {
                    if (audioSrc.channelId.isValid()) {
                        audioService->stop(audioSrc.channelId);
                    }

                    auto audio_opt = asset_service->getAsset<Sound>(audioSrc.selected_audio);
                    if (audio_opt.has_value()) {
                        auto channelOpt = audioService->play(
                            audio_opt.value(),
                            audioSrc.group_name,
                            audioSrc.volumeDb,
                            audioSrc.pitchDb,
                            audioSrc.looping,
                            audioSrc.is3D
                        );
                        if (channelOpt.has_value()) {
                            audioSrc.channelId = channelOpt.value();
                            audioSrc.state = AudioState::Playing;
                        }
                        else {
                            PN_CORE_WARN("Failed to play sound.");
                            audioSrc.state = AudioState::Stopped;
                        }
                    }
                    else {
                        PN_CORE_WARN("Failed to play sound.");
                        audioSrc.state = AudioState::Stopped;
                    }
                    audioSrc.playTrigger = false;
                    audioSrc.stopTrigger = false;
                    audioSrc.playOnStart = false;
                }
                else if (audioSrc.stopTrigger)
                {
                    if (audioSrc.channelId.isValid()) {
                        audioService->stop(audioSrc.channelId);
                    }
                    audioSrc.channelId = { -1 };
                    audioSrc.state = AudioState::Stopped;
                    audioSrc.stopTrigger = false;
                }

                // 4. Update 3D position for active, 3D sounds
                if (audioSrc.state == AudioState::Playing && audioSrc.is3D && audioSrc.channelId.isValid())
                {
                    glm::vec3 worldPos = glm::vec3(worldtransform.matrix[3]);
                    audioService->setPosition(audioSrc.channelId, worldPos);
                }
            }
        }

        void System::onEvent(Event::Event& e)
        {
            // Handle audio-related events if needed
        }

    } // namespace Audio
} // namespace PAIN