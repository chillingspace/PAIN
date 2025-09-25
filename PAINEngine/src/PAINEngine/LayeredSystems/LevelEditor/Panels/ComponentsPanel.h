#pragma once
#ifdef _DEBUG
#ifndef PAIN_EDITOR_COMPONENTS_PANEL_HPP
#define PAIN_EDITOR_COMPONENTS_PANEL_HPP

#include "Panels.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace PAIN {
namespace Editor {
namespace Panel {

// Optional hooks you can fill later to connect to your ECS.
// Leave nullptr/empty to keep the placeholder behavior.
struct ComponentsHooks {
    // return list of available component type names
    std::function<std::vector<std::string>()> listAvailable;

    // return list of components currently on the selected entity
    std::function<std::vector<std::string>()> listOnEntity;

    // add/remove on the selected entity
    std::function<void(const std::string& type)> add;
    std::function<void(const std::string& type)> remove;

    // per-component UI (type -> draw func)
    // void(ImGui&) style placeholder: args are optional, adapt later if needed
    std::function<void(const std::string& type)> drawUI;
};

class ComponentsPanel : public IPanel {
public:
    ComponentsPanel(std::shared_ptr<CommandManager> cm,
                    ComponentsHooks hooks = {});

    void nextWindowSettings() override;   // default
    void onUpdate() override;             // draw inside panel window

    static constexpr const char* getStaticName() { return "##ComponentsPanel"; }

private:
    // Placeholder “entity” state
    std::vector<std::string> available_;     // all possible component types
    std::vector<std::string> onEntity_;      // components currently attached
    std::string pendingRemove_;              // which comp to remove

    // UI state
    bool showAddModal_     = false;
    bool showRemoveModal_  = false;

    // hooks for future integration
    ComponentsHooks hooks_;

private:
    // modal drawers
    void drawAddModal();
    void drawRemoveModal();

    // refresh lists from hooks (or seed defaults)
    void refreshAvailable();
    void refreshOnEntity();

    // helpers
    static bool contains(const std::vector<std::string>& v, const std::string& s);
};

} // namespace Panel
} // namespace Editor
} // namespace PAIN

#endif
#endif
