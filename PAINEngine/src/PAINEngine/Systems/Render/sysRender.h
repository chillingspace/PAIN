#pragma once

#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "ECS/System/ISystem.h"
#include "CoreSystems/Renderer/sRenderer.h"
#include "Systems/Render/MinimapStyle.h"

namespace PAIN {
    namespace Render {

        //Rendering system
        class System : public ECS::System::ISystem {
        private:
            float minimap_threat_time_ = 0.0f;
            std::vector<entt::entity> minimap_marker_entities_;
            std::vector<entt::entity> minimap_danger_entities_;
            std::vector<glm::vec2> minimap_danger_fill_vertices_;
            std::vector<glm::vec2> minimap_danger_line_vertices_;
            std::vector<glm::vec2> minimap_danger_inner_fill_vertices_;
            std::vector<glm::vec2> minimap_danger_inner_line_vertices_;
            std::vector<glm::vec2> minimap_grid_minor_line_vertices_;
            std::vector<glm::vec2> minimap_grid_major_line_vertices_;
            std::vector<MinimapWallCacheFingerprintEntry> minimap_wall_fingerprint_entries_;
            std::unordered_set<uint32_t> minimap_entity_dedupe_;

            //Internal helpers
            void InitializeModelRenderer(entt::entity entity, ModelRenderer& component);

            //Individual render passes
            void shadowPass(entt::registry& registry);
            void geometryPass(entt::registry& registry);
            void reflectionPass(entt::registry& registry);
            void lightingPass(entt::registry& registry);
            void debugPass(entt::registry& registry, int debug_mode);
            void uiPass(entt::registry& registry);
            void particlePass(entt::registry& registry);
            void minimapPass(entt::registry& registry);
            void volumetricPass(entt::registry& registry);

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
