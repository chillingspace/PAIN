#include "pch.h"
#include "sysAudio.h"
#include "CoreSystems/Audio/Audio.h"
#include "CoreSystems/Scene/Scene.h"
#include "ECS/Components/cTransform.h"
#include "ECS/Components/cAudioSource.h"

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
            auto view = registry.view<AudioSource, LocalTransform, WorldTransform>();
            for (auto [entity, audioSrc, transform, worldtransform] : view.each())
            {
                //auto& audioSrc = view.get<AudioSource>(entity);
                //auto& transform = view.get<Transform>(entity);

                static std::unordered_set<entt::entity> initialized;
                if (audioSrc.playOnStart && initialized.find(entity) == initialized.end()) {
                    audioSrc.playTrigger = true;
                    initialized.insert(entity);
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