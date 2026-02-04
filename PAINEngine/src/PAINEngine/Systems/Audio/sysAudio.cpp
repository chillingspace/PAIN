#include "pch.h"
#include "sysAudio.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cAudioSource.h"
#include "ECS/Components/cEntity.h"

namespace PAIN {
    namespace Audio {

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

            // 1. Update Listener Position from Camera
            Camera* camera = scene->GetActiveCamera();
            if (camera) {
                audioService->setListener(camera->pos, { 0,0,0 }, camera->forward, camera->up);
            }

            // 2. Iterate all entities with AudioSource and Transform
            // Optimized using group
            auto group = registry.group<AudioSource>(entt::get<LocalTransform, WorldTransform>);
            
            for (auto [entity, audioSrc, transform, worldtransform] : group.each())
            {
                //auto& audioSrc = view.get<AudioSource>(entity);
                //auto& transform = view.get<Transform>(entity);

                // Check if this is the Global_BGM entity (by name)
                bool isGlobalBGM = false;
                if (registry.all_of<Entity::Name>(entity)) {
                    const auto& nameComp = registry.get<Entity::Name>(entity);
                    if (nameComp.name == "Global_BGM") {
                        isGlobalBGM = true;
                    }
                }

                // Handle Global_BGM entity with multi-track audio
                if (isGlobalBGM && !audioSrc.audio_tracks.empty() && !audioSrc.hasStarted) {
                    PN_CORE_INFO("[AudioSystem] Starting Global BGM with {} tracks", audioSrc.audio_tracks.size());
                    
                    // Clear any existing channel handles
                    audioSrc.track_channel_ids.clear();
                    
                    // Ensure track_volumes has correct size
                    while (audioSrc.track_volumes.size() < audioSrc.audio_tracks.size()) {
                        audioSrc.track_volumes.push_back(-80.0f); // Default muted
                    }
                    
                    // Start all tracks simultaneously at muted volume
                    for (size_t i = 0; i < audioSrc.audio_tracks.size(); ++i) {
                        auto audio_opt = asset_service->getAsset<Sound>(audioSrc.audio_tracks[i]);
                        if (audio_opt.has_value()) {
                            // Play in music group, looping, at -80dB (effectively muted)
                            auto channelOpt = audioService->play(
                                audio_opt.value(),
                                "music",                    // BGM group
                                -80.0f,                     // Start muted
                                0.0f,                       // No pitch change
                                true,                       // Looping
                                false                       // Not 3D
                            );
                            if (channelOpt.has_value()) {
                                audioSrc.track_channel_ids.push_back(channelOpt.value());
                                PN_CORE_INFO("[AudioSystem] Global BGM track {} started on channel {}", 
                                    i, channelOpt.value().value);
                            } else {
                                // Push invalid channel to keep indices aligned
                                audioSrc.track_channel_ids.push_back(AudioChannelId{-1});
                                PN_CORE_WARN("[AudioSystem] Failed to start Global BGM track {}", i);
                            }
                        } else {
                            audioSrc.track_channel_ids.push_back(AudioChannelId{-1});
                            PN_CORE_WARN("[AudioSystem] Global BGM track {} asset not found", i);
                        }
                    }
                    
                    audioSrc.hasStarted = true;
                    audioSrc.state = AudioState::Playing;
                    continue; // Skip normal single-audio processing for Global_BGM
                }

                if (audioSrc.playOnStart && !audioSrc.hasStarted) {
                    audioSrc.playTrigger = true;
                    audioSrc.hasStarted = true;
                }

                // 3. Handle Play/Stop Triggers
                if (audioSrc.playTrigger)
                {
                    // If already playing, stop it first
                    if (audioSrc.channelId.isValid()) {
                        audioService->stop(audioSrc.channelId);
                    }

                    //Chjeck for valid id
                    auto audio_opt = asset_service->getAsset<Sound>(audioSrc.selected_audio);
                    if (audio_opt.has_value()) {
                        // Play the sound
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
                    audioService->setPosition(audioSrc.channelId, transform.position);
                }

                // 5. (Future) Check if channel stopped playing
                // ...
            }
        }

        void System::onEvent(Event::Event& e)
        {
            // Handle audio-related events if needed
        }

    } // namespace Audio
} // namespace PAIN