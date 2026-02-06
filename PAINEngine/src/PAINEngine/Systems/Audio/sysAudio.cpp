#include "pch.h"
#include "sysAudio.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cAudioSource.h"
#include "ECS/Components/cEntity.h"
#include <unordered_set>

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
        static std::string s_lastSceneWithGlobalBGM;  // Track which scene initialized global audio
        
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

                // Handle Global_BGM entity - per-track comparison logic
                if (isGlobalBGM) {
                    // Get current scene name for logging
                    std::string currentSceneName;
                    if (scene) {
                        auto guid = scene->getCurrScnID();
                        if (guid.IsValid()) {
                            auto assetData = asset_service->getAssetData(guid);
                            if (assetData) {
                                currentSceneName = assetData->main_relative_path.generic_string();
                            }
                        }
                    }
                    
                    bool hasNewTracks = !audioSrc.audio_tracks.empty();
                    bool hasExistingTracks = !s_globalTracks.empty() && s_globalAudioInitialized;
                    
                    // ========== CASE 1: No new tracks specified ==========
                    // Keep existing tracks playing (muted state continues)
                    if (!hasNewTracks) {
                        if (hasExistingTracks) {
                            // Sync entity to existing tracks (they stay muted, Lua will fade in if needed)
                            audioSrc.track_channel_ids.clear();
                            for (const auto& track : s_globalTracks) {
                                audioSrc.track_channel_ids.push_back(track.channelId);
                            }
                            audioSrc.hasStarted = true;
                            audioSrc.state = AudioState::Playing;
                            PN_CORE_INFO("[AudioSystem] Global_BGM in '{}' has no tracks - keeping {} existing tracks (muted)", 
                                currentSceneName, s_globalTracks.size());
                        } else {
                            PN_CORE_INFO("[AudioSystem] Global_BGM in '{}' has no tracks and no existing tracks", currentSceneName);
                        }
                        continue; // Skip normal processing
                    }
                    
                    // ========== CASE 2: New scene has tracks - per-track comparison ==========
                    // Build set of new track GUIDs for quick lookup
                    std::unordered_set<Assets::GUID> newTrackSet;
                    for (const auto& guid : audioSrc.audio_tracks) {
                        newTrackSet.insert(guid);
                    }
                    
                    // Process existing tracks: keep SAME, remove OLD
                    std::vector<GlobalAudioTrack> tracksToKeep;
                    std::unordered_set<Assets::GUID> keptTrackGuids;
                    
                    for (auto& existingTrack : s_globalTracks) {
                        if (newTrackSet.count(existingTrack.assetGuid) > 0) {
                            // SAME track - check if channel is still valid
                            if (existingTrack.channelId.isValid() && 
                                audioService->isChannelValid(existingTrack.channelId)) {
                                // Keep this track (it will continue from current position)
                                tracksToKeep.push_back(existingTrack);
                                keptTrackGuids.insert(existingTrack.assetGuid);
                                PN_CORE_INFO("[AudioSystem] SAME track kept: channel {}", existingTrack.channelId.value);
                            } else {
                                // Channel invalid - will need to restart this track
                                PN_CORE_WARN("[AudioSystem] SAME track but channel {} invalid - will restart", 
                                    existingTrack.channelId.value);
                            }
                        } else {
                            // OLD track - not in new scene, stop it
                            if (existingTrack.channelId.isValid()) {
                                audioService->stop(existingTrack.channelId);
                                PN_CORE_INFO("[AudioSystem] OLD track stopped: channel {}", existingTrack.channelId.value);
                            }
                        }
                    }
                    
                    // Start NEW tracks (in new scene but not in existing, or channel was invalid)
                    for (const auto& newGuid : audioSrc.audio_tracks) {
                        if (keptTrackGuids.count(newGuid) == 0) {
                            // This is a NEW track - start it
                            auto audio_opt = asset_service->getAsset<Sound>(newGuid);
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
                                newTrack.assetGuid = newGuid;
                                newTrack.currentVolume = -80.0f;
                                newTrack.targetVolume = -80.0f;
                                newTrack.fadeSpeed = 0.0f;
                                newTrack.isActive = true;
                                
                                if (channelOpt.has_value()) {
                                    newTrack.channelId = channelOpt.value();
                                    PN_CORE_INFO("[AudioSystem] NEW track started: channel {}", channelOpt.value().value);
                                } else {
                                    newTrack.channelId = { -1 };
                                    PN_CORE_WARN("[AudioSystem] Failed to start NEW track");
                                }
                                
                                tracksToKeep.push_back(newTrack);
                            } else {
                                GlobalAudioTrack emptyTrack;
                                emptyTrack.assetGuid = newGuid;
                                emptyTrack.channelId = { -1 };
                                emptyTrack.isActive = false;
                                tracksToKeep.push_back(emptyTrack);
                                PN_CORE_WARN("[AudioSystem] NEW track asset not found");
                            }
                        }
                    }
                    
                    // Rebuild s_globalTracks in the ORDER of audioSrc.audio_tracks
                    // This ensures track indices match what Lua expects
                    s_globalTracks.clear();
                    audioSrc.track_channel_ids.clear();
                    for (const auto& newGuid : audioSrc.audio_tracks) {
                        // Find the track with this GUID in tracksToKeep
                        bool found = false;
                        for (const auto& track : tracksToKeep) {
                            if (track.assetGuid == newGuid) {
                                s_globalTracks.push_back(track);
                                audioSrc.track_channel_ids.push_back(track.channelId);
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            // Should not happen, but handle gracefully
                            GlobalAudioTrack emptyTrack;
                            emptyTrack.assetGuid = newGuid;
                            emptyTrack.channelId = { -1 };
                            emptyTrack.isActive = false;
                            s_globalTracks.push_back(emptyTrack);
                            audioSrc.track_channel_ids.push_back(AudioChannelId{-1});
                        }
                    }
                    
                    audioSrc.hasStarted = true;
                    audioSrc.state = AudioState::Playing;
                    s_globalAudioInitialized = true;
                    s_lastSceneWithGlobalBGM = currentSceneName;
                    
                    PN_CORE_INFO("[AudioSystem] Global BGM processed for '{}': {} tracks total", 
                        currentSceneName, s_globalTracks.size());
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