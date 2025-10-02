#pragma once

#include "Panels.h"
#include <vector>
#include <string>

namespace PAIN {
    namespace Editor {
        namespace Panel {

            class EntityPanel : public IPanel {
            public:
                EntityPanel();
                void onUpdate() override;
                void nextWindowSettings() override;

            private:
                std::vector<std::string> entities;  // List of entities
                int selectedEntityIndex = -1;       // Selected entity index

                // Button actions
                void createEntity();
                void removeEntity();
            };

        } // namespace Panel
    } // namespace Editor
} // namespace PAIN
