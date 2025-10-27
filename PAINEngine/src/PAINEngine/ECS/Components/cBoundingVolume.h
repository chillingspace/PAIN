#pragma once

#ifndef C_BOUNDING_VOLUME_H
#define C_BOUNDING_VOLUME_H

// Include dependencies outside namespace if they might be needed globally by this header
#include "CoreSystems/Collision/BoundingVolume.h" // Includes AABB definition

namespace PAIN {

#include "pch.h" // Include pch inside namespace if AABB doesn't depend on it directly

// ECS Component holding bounding volume data for an entity
struct cBoundingVolume {
    AABB localAABB;
    AABB worldAABB;
    int bvhNodeIndex = -1;
    bool needsUpdate = true;
};

#ifdef _DEBUG
// Includes for the debug UI function need to be inside the _DEBUG block
#include "imgui.h" // Include ImGui header directly
#include "glm/gtc/type_ptr.hpp" // For glm::value_ptr
// Include the specific header defining ComponentsPanel
#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h"

// Define the UI function within the PAIN namespace
inline void RegisterBoundingVolumeUI_UNUSED(Editor::Panel::ComponentsPanel& panel) {
    // Use the fully qualified type for the lambda parameter if necessary
    panel.registerCompUIFunc<cBoundingVolume>([](PAIN::Editor::Panel::ComponentsPanel& comp_panel, cBoundingVolume& volume) {
        ImGui::Text("Bounding Volume");
        ImGui::Separator();

        ImGui::Text("Local AABB:");
        ImGui::InputFloat3("Min##Local", glm::value_ptr(volume.localAABB.min), "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("Max##Local", glm::value_ptr(volume.localAABB.max), "%.3f", ImGuiInputTextFlags_ReadOnly);

        ImGui::Spacing();

        ImGui::Text("World AABB:");
        ImGui::InputFloat3("Min##World", glm::value_ptr(volume.worldAABB.min), "%.3f", ImGuiInputTextFlags_ReadOnly);
        ImGui::InputFloat3("Max##World", glm::value_ptr(volume.worldAABB.max), "%.3f", ImGuiInputTextFlags_ReadOnly);

        ImGui::Spacing();
        ImGui::Text("BVH Node Index: %d", volume.bvhNodeIndex);
        ImGui::Text("Needs Update: %s", volume.needsUpdate ? "Yes" : "No");
    });
}
#endif // _DEBUG

} // namespace PAIN

#endif // C_BOUNDING_VOLUME_H