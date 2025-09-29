#include "pch.h"
#include "ComponentsPanel.h"

#ifdef _DEBUG
#ifdef PN_PLATFORM_WINDOWS
#include <algorithm>

namespace PAIN {
namespace Editor {
namespace Panel {

static std::vector<std::string> kSeedAvailable = {
    "Transform",
    "SpriteRenderer",
    "Camera",
    "RigidBody2D",
    "BoxCollider2D",
    "AudioSource",
    "Script"
};

ComponentsPanel::ComponentsPanel(std::shared_ptr<CommandManager> cm, ComponentsHooks hooks)
    : IPanel(std::move(cm)), hooks_(std::move(hooks)) {

    name  = "Components";
    flags = ImGuiWindowFlags_None;

    refreshAvailable();
    refreshOnEntity(); // placeholder starts empty
}

void ComponentsPanel::nextWindowSettings() {
    // default window behavior
}

bool ComponentsPanel::contains(const std::vector<std::string>& v, const std::string& s) {
    return std::find(v.begin(), v.end(), s) != v.end();
}

void ComponentsPanel::refreshAvailable() {
    if (hooks_.listAvailable) {
        available_ = hooks_.listAvailable();
        if (available_.empty()) available_ = kSeedAvailable;
    } else {
        available_ = kSeedAvailable;
    }
}

void ComponentsPanel::refreshOnEntity() {
    if (hooks_.listOnEntity) {
        onEntity_ = hooks_.listOnEntity();
    } else {
        onEntity_.shrink_to_fit(); // keep placeholder empty unless user adds
    }
}

void ComponentsPanel::drawAddModal() {
    if (!showAddModal_) return;
    ImGui::OpenPopup("Add Component");
    if (ImGui::BeginPopupModal("Add Component", &showAddModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextUnformatted("Select a component to add:");
        ImGui::Spacing();

        for (const auto& type : available_) {
            if (contains(onEntity_, type)) continue; // already added
            if (ImGui::Button(type.c_str(), ImVec2(220, 0))) {
                // placeholder add
                if (hooks_.add) hooks_.add(type);
                if (!contains(onEntity_, type)) onEntity_.push_back(type);
                std::sort(onEntity_.begin(), onEntity_.end());
                showAddModal_ = false;
                ImGui::CloseCurrentPopup();
                break;
            }
        }

        ImGui::Spacing();
        if (ImGui::Button("Cancel", ImVec2(220, 0))) {
            showAddModal_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ComponentsPanel::drawRemoveModal() {
    if (!showRemoveModal_) return;
    ImGui::OpenPopup("Remove Component");
    if (ImGui::BeginPopupModal("Remove Component", &showRemoveModal_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(ImVec4(1,0,0,1), "This action cannot be undone!");
        ImGui::Text("Remove component: %s ?", pendingRemove_.c_str());
        ImGui::Spacing();

        if (ImGui::Button("Ok", ImVec2(100, 0))) {
            if (hooks_.remove) hooks_.remove(pendingRemove_);
            onEntity_.erase(std::remove(onEntity_.begin(), onEntity_.end(), pendingRemove_), onEntity_.end());
            pendingRemove_.clear();
            showRemoveModal_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            pendingRemove_.clear();
            showRemoveModal_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void ComponentsPanel::onUpdate() {
    // header line
    ImGui::Text("Selected Entity: %s", onEntity_.empty() ? "(placeholder)" : "(placeholder)");
    ImGui::Text("Components: %d", (int)onEntity_.size());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Add component
    if (ImGui::Button("Add Component")) {
        refreshAvailable();
        showAddModal_ = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // List each component as a collapsible section
    for (const auto& type : onEntity_) {
        if (ImGui::CollapsingHeader(type.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {

            // Placeholder UI per component
            if (hooks_.drawUI) {
                hooks_.drawUI(type);
            } else {
                // Simple generic dummy controls so UI feels alive
                if (type == "Transform") {
                    static float pos[3] = {0,0,0};
                    static float scl[3] = {1,1,1};
                    static float rot    = 0.0f;
                    ImGui::DragFloat3("Position", pos, 0.1f);
                    ImGui::DragFloat3("Scale",    scl, 0.01f, 0.001f, 1000.0f);
                    ImGui::DragFloat("Rotation",  &rot, 0.1f, -360.0f, 360.0f);
                } else if (type == "SpriteRenderer") {
                    static float tint[4] = {1,1,1,1};
                    ImGui::ColorEdit4("Tint", tint);
                    static int layer = 0; ImGui::DragInt("Layer", &layer, 1.0f, -100, 100);
                    static char path[256] = "assets/textures/placeholder.png";
                    ImGui::InputText("Texture", path, IM_ARRAYSIZE(path));
                } else if (type == "Camera") {
                    static float zoom = 1.0f; ImGui::DragFloat("Zoom", &zoom, 0.01f, 0.01f, 100.0f);
                    static bool  ortho = true; ImGui::Checkbox("Orthographic", &ortho);
                } else if (type == "RigidBody2D") {
                    static float mass=1.0f, friction=0.2f; 
                    ImGui::DragFloat("Mass", &mass, 0.01f, 0.0f, 1000.0f);
                    ImGui::DragFloat("Friction", &friction, 0.01f, 0.0f, 1.0f);
                    static bool kinematic=false; ImGui::Checkbox("Kinematic", &kinematic);
                } else if (type == "BoxCollider2D") {
                    static float size[2] = {1,1}; ImGui::DragFloat2("Size", size, 0.01f, 0.01f, 1000.0f);
                    static float offset[2] = {0,0}; ImGui::DragFloat2("Offset", offset, 0.01f, -1000.0f, 1000.0f);
                    static bool  isTrigger=false; ImGui::Checkbox("Is Trigger", &isTrigger);
                } else if (type == "AudioSource") {
                    static float volume = 0.0f; // dB
                    ImGui::SliderFloat("Volume (dB)", &volume, -80.0f, 10.0f, "%.1f");
                    static bool loop=false; ImGui::Checkbox("Loop", &loop);
                    static char clip[256] = "assets/audio/SFX/example.wav";
                    ImGui::InputText("Clip", clip, IM_ARRAYSIZE(clip));
                } else if (type == "Script") {
                    static char klass[128] = "MyBehaviour";
                    ImGui::InputText("Class", klass, IM_ARRAYSIZE(klass));
                    ImGui::TextDisabled("Fields will appear after runtime binding.");
                } else {
                    ImGui::TextDisabled("No UI registered yet for \"%s\".", type.c_str());
                }
            }

            ImGui::Spacing();
            if (ImGui::Button(std::string("Remove Component##" + type).c_str())) {
                pendingRemove_   = type;
                showRemoveModal_ = true;
            }
        }
    }

    // Modals
    drawAddModal();
    drawRemoveModal();
}

} // namespace Panel
} // namespace Editor
} // namespace PAIN

#endif
#endif
