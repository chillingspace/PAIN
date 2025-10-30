#pragma once

#ifndef SYS_AUDIO_H
#define SYS_AUDIO_H

#include "pch.h"
#include "ECS/System/ISystem.h"
#include "CoreSystems/Audio/Audio.h"

namespace PAIN {
    namespace Audio {

        class System : public ECS::System::ISystem
        {
        public:
            explicit System(std::shared_ptr<Services> svc);
            ~System() override = default;

            void onUpdate(AppTiming timing, entt::registry& reg) override;
            void onEvent(Event::Event& e) override;
            std::string getSysName() const override { return "Audio System"; }
        };

    } // namespace Audio
} // namespace PAIN

#endif // SYS_AUDIO_H