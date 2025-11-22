#pragma once

#ifndef TRANSFORM_SYSTEM_HPP
#define TRANSFORM_SYSTEM_HPP

#include "ECS/Components/cTransform.h"
#include "ECS/Components/cEntity.h"
#include "ECS/System/ISystem.h"
#include "ECS/Controller.h"

namespace PAIN {
    namespace Transform {

        class System : public ECS::System::ISystem {
        public:
            explicit System(std::shared_ptr<Services> svc):ISystem(svc){}

            std::string getSysName() const override { return "Transform System"; }

            void onUpdate(AppTiming timing,entt::registry& registry) override;
            void markDirty(entt::entity e, entt::registry& registry);

            // Hierarchy API
            void setParent(entt::entity child, entt::entity parent, entt::registry& registry);
            void removeParent(entt::entity child, entt::registry& registry);

        private:
            void updateRecursive(entt::entity e, const glm::mat4& parentWorld, entt::registry& registry);
            void propagateDirty(entt::entity e, entt::registry& registry);
        };

    }
}

#endif // TRANSFORM_SYSTEM_H
