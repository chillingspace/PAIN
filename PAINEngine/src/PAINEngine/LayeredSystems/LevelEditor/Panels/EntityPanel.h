#pragma once

#include "Panels.h"
#include "CoreSystems/Scene/Scene.h"
#ifdef _DEBUG
namespace PAIN {
    namespace Editor {
        namespace Panel {

            class EntityPanel : public IPanel {
            public:
                EntityPanel();
                void onUpdate(PAIN::AppTiming timing) override;
                void nextWindowSettings() override;

            private:
                std::vector<std::string> entities;  // List of entities


                int total_entities;
                int selectedEntityIndex = -1;       // Selected entity index

                // Button actions
                void createEntity();
                void removeEntity();
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
#endif