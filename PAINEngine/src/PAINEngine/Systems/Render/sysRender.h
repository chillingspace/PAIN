#pragma once

#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "ECS/System/ISystem.h"
#include "CoreSystems/Renderer/sRenderer.h"

namespace PAIN {
    namespace Render {

        //Rendering system
        class System : public ECS::System::ISystem {
        private:

            //Internal helpers
            void InitializeModelRenderer(entt::entity entity, ModelRenderer& component);

            //Individual render passes
            void shadowPass(entt::registry& registry);
            void geometryPass(entt::registry& registry);
            void reflectionPass(entt::registry& registry);
            void lightingPass(entt::registry& registry);
            void debugPass(entt::registry& registry, int debug_mode);
            void uiPass(entt::registry& registry);

        public:
            explicit System(std::shared_ptr<Services> svc);
            ~System() override = default;

            // ISystem interface
            void onUpdate(AppTiming timing, entt::registry& registry) override;
            void onFixedUpdate(AppTiming timing, entt::registry& registry) override;
            void onEvent(Event::Event& e) override;

            std::string getSysName() const override { return "RenderSystem"; }
        };

    } // namespace Render
} // namespace PAIN

#endif // RENDER_SYSTEM_HPP
