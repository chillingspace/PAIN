#pragma once

#ifndef C_BOUNDING_VOLUME_H
#define C_BOUNDING_VOLUME_H

// Include AABB definition globally if needed by other files including this one
#include "CoreSystems/Collision/BoundingVolume.h"
#include "pch.h" // Include pch inside namespace
#include "imgui.h" // Direct include for ImGui functions
#include "glm/gtc/type_ptr.hpp" // For glm::value_ptr
#include "LayeredSystems/LevelEditor/Panels/ComponentsPanel.h" // For Panel type definition

namespace PAIN {

    // ECS Component holding bounding volume data for an entity
    struct BoundingVolume {
        AABB localAABB; // AABB relative to model origin
        AABB worldAABB; // AABB in world space
        int bvhNodeIndex = -1; // Index in BVH node pool, -1 if not added
        bool needsUpdate = true; // Flag for world AABB recalculation

        //Serialization flag
        static constexpr bool ShouldSerialize = true;
    };

#ifdef _DEBUG
    // Includes specific to the debug UI function

// Inline function definition for debug UI registration
// Marked UNUSED as per previous instruction to not modify editor panels for now
    //inline void RegisterBoundingVolumeUI_UNUSED(PAIN::Editor::Panel::ComponentsPanel& panel) {
    //    // Register a lambda function to draw the UI for cBoundingVolume
    //    panel.registerCompUIFunc<cBoundingVolume>([](PAIN::Editor::Panel::ComponentsPanel& comp_panel, cBoundingVolume& volume) {
    //        // Use ImGui calls to display component data
    //        ImGui::Text("Bounding Volume");
    //        ImGui::Separator();

    //        ImGui::Text("Local AABB:");
    //        // Display min/max vectors, read-only
    //        ImGui::InputFloat3("Min##Local", glm::value_ptr(volume.localAABB.min), "%.3f", ImGuiInputTextFlags_ReadOnly);
    //        ImGui::InputFloat3("Max##Local", glm::value_ptr(volume.localAABB.max), "%.3f", ImGuiInputTextFlags_ReadOnly);

    //        ImGui::Spacing();

    //        ImGui::Text("World AABB:");
    //        ImGui::InputFloat3("Min##World", glm::value_ptr(volume.worldAABB.min), "%.3f", ImGuiInputTextFlags_ReadOnly);
    //        ImGui::InputFloat3("Max##World", glm::value_ptr(volume.worldAABB.max), "%.3f", ImGuiInputTextFlags_ReadOnly);

    //        ImGui::Spacing();
    //        // Display internal state
    //        ImGui::Text("BVH Node Index: %d", volume.bvhNodeIndex);
    //        ImGui::Text("Needs Update: %s", volume.needsUpdate ? "Yes" : "No");
    //        });
    //}
#endif // _DEBUG

} // namespace PAIN

//
// --- THIS IS THE FIX ---
// The nlohmann namespace block MUST be outside the PAIN namespace
//
//namespace nlohmann {
//    template<>
//    struct adl_serializer<PAIN::BoundingVolume> {
//        static void to_json(json& j, const PAIN::BoundingVolume& bv) {
//            j = json{
//                {"localAABB", bv.localAABB},
//                {"worldAABB", bv.worldAABB},
//                {"bvhNodeIndex", bv.bvhNodeIndex},
//                {"needsUpdate", bv.needsUpdate}
//            };
//        }
//
//        static void from_json(const json& j, PAIN::BoundingVolume& bv) {
//            j.at("localAABB").get_to(bv.localAABB);
//            j.at("worldAABB").get_to(bv.worldAABB);
//            j.at("bvhNodeIndex").get_to(bv.bvhNodeIndex);
//            j.at("needsUpdate").get_to(bv.needsUpdate);
//        }
//    };
//}

// Reflection
REFL_TYPE(PAIN::BoundingVolume)
REFL_FIELD(localAABB)
REFL_FIELD(worldAABB)
REFL_FIELD(bvhNodeIndex)
REFL_FIELD(needsUpdate)
REFL_END

static_assert(refl::trait::is_reflectable_v<PAIN::BoundingVolume>);



#endif // C_BOUNDING_VOLUME_H