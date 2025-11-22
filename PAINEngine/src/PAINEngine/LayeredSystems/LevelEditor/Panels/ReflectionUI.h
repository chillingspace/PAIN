/*****************************************************************//**
 * \file   ReflectionUI.h
 * \brief  Declaration of reflection UI for imgui
 *
 * \author Bryan Soh, 2301238, z.soh@digipen.edu (100%)
 * \co-author
 * \date   October 2025
 * All content  2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#ifdef _DEBUG
#pragma once

#ifndef REFELCTION_UI_HPP
#define REFELCTION_UI_HPP

#include <refl.hpp>
#include <imgui.h>
#include "pch.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/constants.hpp>
#include <entt/entt.hpp>
#include "ComponentsPanel.h"
#include "ECS/Components/cLight.h"
#include "ECS/Components/cAudioSource.h"
#include "ECS/Components/cBoundingVolume.h"
#include "ECS/Components/cHierarchy.h"
#include "ECS/Components/cPhysics.h"
#include "ECS/Components/cMeshRenderer.h"
#include "Applications/AppSystem.h"

#include "LayeredSystems/LevelEditor/EditorAttributes.h"

 // Helper: Fuzzy match score
inline int fuzzyMatchScore(const std::string& pattern, const std::string& str) {
    int score = 0;
    size_t pattern_idx = 0;

    for (size_t str_idx = 0; str_idx < str.length() && pattern_idx < pattern.length(); ++str_idx) {
        char p = std::tolower(pattern[pattern_idx]);
        char s = std::tolower(str[str_idx]);

        if (p == s) {
            score += (str_idx == pattern_idx) ? 2 : 1;  // Bonus for consecutive matches
            pattern_idx++;
        }
    }

    // Return score only if all pattern chars matched
    return (pattern_idx == pattern.length()) ? score : 0;
}

// ---------- Asset Selector with Fuzzy Search ----------
inline bool DrawAssetSelectorField(
    const char* label,
    PAIN::Assets::GUID& guid,
    const PAIN::Editor::Attributes::AssetSelector& attr,
    std::shared_ptr<PAIN::Services> services,
    bool readonly = false
) {
    bool changed = false;
    auto asset_service = services->get<PAIN::Assets::Manager>();
    auto assets = asset_service->getAllAssetDataOfType(attr.asset_type);

    // Find current selection
    std::string current_name = "None";
    for (auto const& asset : assets) {
        if (guid.IsValid() && asset->guid == guid) {
            current_name = asset->main_relative_path.filename().string();
            break;
        }
    }

    ImGui::Text("%s", label);

    if (readonly) ImGui::BeginDisabled();

    // Selector button
    std::string button_id = current_name + "###" + std::string(label) + "_selector";
    if (ImGui::Button(button_id.c_str(), ImVec2(-1, 0))) {
        ImGui::OpenPopup(label);
    }

    // Searchable popup
    if (ImGui::BeginPopup(label, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char search_buffer[256] = "";

        // Search input
        ImGui::SetNextItemWidth(400);
        if (ImGui::InputTextWithHint("##search", "Type to search...",
            search_buffer, 256, ImGuiInputTextFlags_AutoSelectAll)) {
            // Search text changed
        }

        ImGui::Separator();

        // Build filtered and sorted list
        struct ScoredAsset {
            std::shared_ptr<const PAIN::Assets::IAsset> asset;
            int score;
            std::string display_name;
        };

        std::vector<ScoredAsset> scored_assets;

        for (auto const& asset : assets) {
            std::string asset_path = asset->main_relative_path.string();
            std::string asset_name = asset->name.empty() ? asset->main_relative_path.filename().string() : asset->name;

            int score = 100;  // Default score

            if (strlen(search_buffer) > 0) {
                score = fuzzyMatchScore(search_buffer, asset_name);
                if (score == 0) {
                    score = fuzzyMatchScore(search_buffer, asset_path);
                }
                if (score == 0) continue;  // No match
            }

            scored_assets.push_back({ asset, score, asset_path });
        }

        // Sort by score (descending)
        std::sort(scored_assets.begin(), scored_assets.end(),
            [](const ScoredAsset& a, const ScoredAsset& b) {
                return a.score > b.score;
            });

        // Display results
        ImGui::BeginChild("AssetList", ImVec2(400, 300), true);

        for (auto const& scored : scored_assets) {
            bool is_selected = (guid.IsValid() && scored.asset->guid == guid);

            // Highlight matched characters
            if (ImGui::Selectable(scored.display_name.c_str(), is_selected)) {
                guid = scored.asset->guid;
                changed = true;
                search_buffer[0] = '\0';  // Clear search
                ImGui::CloseCurrentPopup();
            }

            // Tooltip with full path
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Path: %s", scored.asset->shipped_relative_path.string().c_str());
                ImGui::Text("Type: %s", PAIN::Assets::assetTypeToString(scored.asset->type).c_str());
                if (strlen(search_buffer) > 0) {
                    ImGui::Text("Match Score: %d", scored.score);
                }
                ImGui::EndTooltip();
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        if (scored_assets.empty()) {
            ImGui::TextDisabled("No matching assets");
        }

        ImGui::EndChild();

        // Footer stats
        ImGui::Separator();
        ImGui::Text("Showing: %zu / %zu assets", scored_assets.size(), assets.size());

        ImGui::EndPopup();
    }

    if (readonly) ImGui::EndDisabled();
    return changed;
}

inline bool DrawAssetSelectorField(
    const char* label,
    std::filesystem::path& path,
    const PAIN::Editor::Attributes::AssetSelector& attr,
    std::shared_ptr<PAIN::Services> services,
    bool readonly = false
) {
    bool changed = false;
    auto asset_service = services->get<PAIN::Assets::Manager>();
    auto assets = asset_service->getAllAssetDataOfType(attr.asset_type);

    // Find current selection
    std::string current_name = "None";
    for (auto const& asset : assets) {
        if (!path.empty() && asset->main_relative_path == path) {
            current_name = asset->main_relative_path.filename().string();
            break;
        }
    }

    ImGui::Text("%s", label);

    if (readonly) ImGui::BeginDisabled();

    // Selector button
    std::string button_id = current_name + "###" + std::string(label) + "_selector";
    if (ImGui::Button(button_id.c_str(), ImVec2(-1, 0))) {
        ImGui::OpenPopup(label);
    }

    // Searchable popup
    if (ImGui::BeginPopup(label, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char search_buffer[256] = "";

        // Search input
        ImGui::SetNextItemWidth(400);
        if (ImGui::InputTextWithHint("##search", "Type to search...",
            search_buffer, 256, ImGuiInputTextFlags_AutoSelectAll)) {
            // Search text changed
        }

        ImGui::Separator();

        // Build filtered and sorted list
        struct ScoredAsset {
            std::shared_ptr<const PAIN::Assets::IAsset> asset;
            int score;
            std::string display_name;
        };

        std::vector<ScoredAsset> scored_assets;

        for (auto const& asset : assets) {
            std::string asset_path = asset->main_relative_path.string();
            std::string asset_name = asset->name.empty() ? asset->main_relative_path.filename().string() : asset->name;

            int score = 100;  // Default score

            if (strlen(search_buffer) > 0) {
                score = fuzzyMatchScore(search_buffer, asset_name);
                if (score == 0) {
                    score = fuzzyMatchScore(search_buffer, asset_path);
                }
                if (score == 0) continue;  // No match
            }

            scored_assets.push_back({ asset, score, asset_path });
        }

        // Sort by score (descending)
        std::sort(scored_assets.begin(), scored_assets.end(),
            [](const ScoredAsset& a, const ScoredAsset& b) {
                return a.score > b.score;
            });

        // Display results
        ImGui::BeginChild("AssetList", ImVec2(400, 300), true);

        for (auto const& scored : scored_assets) {
            bool is_selected = (!path.empty() && scored.asset->main_relative_path == path);

            // Highlight matched characters
            if (ImGui::Selectable(scored.display_name.c_str(), is_selected)) {
                path = scored.asset->main_relative_path;
                changed = true;
                search_buffer[0] = '\0';  // Clear search
                ImGui::CloseCurrentPopup();
            }

            // Tooltip with full path
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::Text("Path: %s", scored.asset->shipped_relative_path.string().c_str());
                ImGui::Text("Type: %s", PAIN::Assets::assetTypeToString(scored.asset->type).c_str());
                if (strlen(search_buffer) > 0) {
                    ImGui::Text("Match Score: %d", scored.score);
                }
                ImGui::EndTooltip();
            }

            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        if (scored_assets.empty()) {
            ImGui::TextDisabled("No matching assets");
        }

        ImGui::EndChild();

        // Footer stats
        ImGui::Separator();
        ImGui::Text("Showing: %zu / %zu assets", scored_assets.size(), assets.size());

        ImGui::EndPopup();
    }

    if (readonly) ImGui::EndDisabled();
    return changed;
}


// Mark fields as read-only in the reflected UI
struct ReadOnlyTag : refl::attr::usage::field {};

// ---------- Asset GUID ----------
inline bool DrawField(const char* label, PAIN::Assets::GUID& v) { ImGui::Text(v.ToString().c_str()); return true; }

// ---------- Primitive + std types ----------
inline bool DrawField(const char* label, bool& v) { return ImGui::Checkbox(label, &v); }
inline bool DrawField(const char* label, int& v) { return ImGui::DragInt(label, &v, 1); }
inline bool DrawField(const char* label, float& v) { return ImGui::DragFloat(label, &v, 0.1f); }
inline bool DrawField(const char* label, double& v) { float f = (float)v; bool c = ImGui::DragFloat(label, &f, 0.1f); if (c)v = (double)f; return c; }
inline bool DrawField(const char* label, std::string& s) {
    char buf[512]; std::snprintf(buf, sizeof(buf), "%s", s.c_str());
    if (ImGui::InputText(label, buf, sizeof(buf))) { s = buf; return true; }
    return false;
}

// ---------- GLM drawers (no reflection of GLM) ----------
inline bool DrawField(const char* label, glm::vec2& v) { return ImGui::DragFloat2(label, glm::value_ptr(v), 0.1f); }
inline bool DrawField(const char* label, glm::vec3& v) { return ImGui::DragFloat3(label, glm::value_ptr(v), 0.1f); }
inline bool DrawField(const char* label, glm::vec4& v) { return ImGui::DragFloat4(label, glm::value_ptr(v), 0.1f); }

// Show quat as Euler degrees for editing
inline bool DrawField(const char* label, glm::quat& q) {
    glm::vec3 eulerDeg = glm::degrees(glm::eulerAngles(q));
    if (ImGui::DragFloat3(label, glm::value_ptr(eulerDeg), 1.0f)) {
        q = glm::quat(glm::radians(eulerDeg));
        return true;
    }
    return false;
}

// ----------- Unsigned ints -----------
inline bool DrawField(const char* label, uint8_t& v) { unsigned int uv = v; bool ch = ImGui::InputScalar(label, ImGuiDataType_U8, &uv); if (ch) v = (uint8_t)uv; return ch; }
inline bool DrawField(const char* label, uint16_t& v) { unsigned int uv = v; bool ch = ImGui::InputScalar(label, ImGuiDataType_U16, &uv); if (ch) v = (uint16_t)uv; return ch; }

// Exact type with matching step & format (decimal)
inline bool DrawField(const char* label, uint32_t& v) {
    uint32_t step = 1;
    return ImGui::InputScalar(label, ImGuiDataType_U32, &v, &step, nullptr, "%u",
        ImGuiInputTextFlags_CharsDecimal);
}

inline bool DrawField(const char* label, uint64_t& v) {
    uint64_t step = 1;
    return ImGui::InputScalar(label, ImGuiDataType_U64, &v, &step, nullptr, "%llu",
        ImGuiInputTextFlags_CharsDecimal);
}

// ----- Lighting Enums -----
inline bool DrawField(const char* label, PAIN::TYPES& v) {
    const char* names[] = { "Point", "Directional", "Spotlight" };
    int idx = static_cast<int>(v);
    const int count = 3; // keep in sync
    const char* preview = (idx >= 0 && idx < count) ? names[idx] : "Unknown";
    bool changed = false;

    if (ImGui::BeginCombo(label, preview)) {
        for (int i = 0; i < count; ++i) {
            bool selected = (i == idx);
            if (ImGui::Selectable(names[i], selected)) {
                idx = i;
                v = static_cast<PAIN::TYPES>(i);
                changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

inline bool DrawField(const char* label, PAIN::SHADOW_TYPES& v) {
    const char* names[] = { "None", "Mapped", "Screen Space" };
    int idx = static_cast<int>(v);
    const int count = 3; // keep in sync
    const char* preview = (idx >= 0 && idx < count) ? names[idx] : "Unknown";
    bool changed = false;

    if (ImGui::BeginCombo(label, preview)) {
        for (int i = 0; i < count; ++i) {
            bool selected = (i == idx);
            if (ImGui::Selectable(names[i], selected)) {
                idx = i;
                v = static_cast<PAIN::SHADOW_TYPES>(i);
                changed = true;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

// ----- PAIN::AABB drawer (read-only UI) -----
 namespace PAIN { struct AABB; } // For Bounding Volume

 inline bool DrawField(const char* label, PAIN::AABB& aabb) {
     bool changed = false;
     ImGui::SeparatorText(label);

     ImGui::BeginDisabled(true);
     ImGui::InputFloat3("Min", glm::value_ptr(aabb.min), "%.3f");
     ImGui::InputFloat3("Max", glm::value_ptr(aabb.max), "%.3f");
     ImGui::EndDisabled();

     // Small helpers (don't flip changed flags; they don't edit the struct)
     ImGui::SameLine();
     if (ImGui::Button("Copy##AABB")) {
         char buf[256];
         std::snprintf(buf, sizeof(buf),
             "Min(%.3f, %.3f, %.3f) Max(%.3f, %.3f, %.3f)",
             aabb.min.x, aabb.min.y, aabb.min.z,
             aabb.max.x, aabb.max.y, aabb.max.z);
         ImGui::SetClipboardText(buf);
     }
     return changed; // drawer is read-only
 }

 // ---------- entt::entity drawer ----------
 inline bool DrawField(const char* label, entt::entity& e) {
     using id_t = entt::id_type;
     bool changed = false;

     id_t id = (e == entt::null) ? 0u : static_cast<id_t>(e);
     ImGui::SetNextItemWidth(-1);
     if (ImGui::InputScalar(label, ImGuiDataType_U32, &id, nullptr, nullptr, "%u",
         ImGuiInputTextFlags_CharsDecimal)) {
         e = (id == 0u) ? entt::null : static_cast<entt::entity>(id);
         changed = true;
     }
     ImGui::SameLine();
     if (ImGui::SmallButton("Clear")) { e = entt::null; changed = true; }
     return changed;
 }

 // ---------- generic vector drawer (uses element drawers) ----------
 template <typename T>
 inline bool DrawField(const char* label, std::vector<T>& v) {
     bool changed = false;
     if (ImGui::TreeNodeEx(label, ImGuiTreeNodeFlags_DefaultOpen)) {
         for (size_t i = 0; i < v.size(); ++i) {
             ImGui::PushID(static_cast<int>(i));
             std::string item = "[" + std::to_string(i) + "]";
             changed |= DrawField(item.c_str(), v[i]);
             ImGui::PopID();
         }
         ImGui::TreePop();
     }
     return changed;
 }


// ----- AudioState Enum -----
inline bool DrawField(const char* label, PAIN::Audio::AudioState& v) {
    const char* names[] = { "Stopped", "Playing", "Paused" };
    int idx = static_cast<int>(v);
    if (idx < 0 || idx > 2) idx = 0; // Safety clamp
    const char* preview = names[idx];
    bool changed = false;

    // Make the state read-only in the editor
    ImGui::Text("%s:", label);
    ImGui::SameLine();
    ImGui::TextDisabled("%s", preview);

    return changed; // changed will always be false
}

// ---- SHAPE enum drawer (cPhysics) ----
inline bool DrawField(const char* label, PAIN::SHAPE& v) {
    const char* names[] = { "Box", "Sphere", "Capsule", "Mesh" };
    int idx = static_cast<int>(v);
    bool changed = false;

    if (ImGui::BeginCombo(label, names[idx])) {
        for (int i = 0; i < 4; ++i) {
            bool sel = (i == idx);
            if (ImGui::Selectable(names[i], sel)) { idx = i; v = static_cast<PAIN::SHAPE>(i); changed = true; }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

// ---- JOINT enum drawer (cPhysics) ----
inline bool DrawField(const char* label, PAIN::JOINT_TYPE& v) {
    const char* names[] = { "Fixed", "Hinge" };
    int idx = static_cast<int>(v);
    if (idx < 0 || idx > 1) idx = 0;

    bool changed = false;
    const char* preview = names[idx];

    if (ImGui::BeginCombo(label, preview)) {
        for (int i = 0; i < 2; ++i) {
            bool sel = (i == idx);
            if (ImGui::Selectable(names[i], sel)) {
                v = static_cast<PAIN::JOINT_TYPE>(i);
                changed = true;
            }
            if (sel) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    return changed;
}

// ---- RigidBody3D (cPhysics) ----
inline bool DrawField(const char* label, JPH::BodyID& id) {
    // Jolt exposes either GetIndexAndSequenceNumber() or GetValue() depending on version.
    // Use whichever your Jolt has. If both exist, prefer GetIndexAndSequenceNumber().
    const uint32_t val = id.GetIndexAndSequenceNumber();

    ImGui::BeginDisabled(true);
    ImGui::Text("%s: %u", label, static_cast<unsigned>(val));
    ImGui::EndDisabled();
    return false; // read-only
}


// ---- JPH::ObjectLayer enum drawer (Physics Layers) ----
inline bool DrawField(const char* label, PAIN::Physics::PhysicsLayer& layer) {
    const char* names[] = { "NON-MOVING", "MOVING", "DEBRIS", "SENSOR" };
    const uint16_t values[] = {
        PAIN::Layer::NON_MOVING,
        PAIN::Layer::MOVING,
        PAIN::Layer::DEBRIS,
        PAIN::Layer::SENSOR
    };

    int idx = 1;
    for (int i = 0; i < 4; ++i) {
        if (layer.value == values[i]) {
            idx = i;
            break;
        }
    }

    bool changed = false;  // Track change

    if (ImGui::Combo(label, &idx, names, 4)) {
        layer.value = values[idx];
        changed = true;  // Mark as changed
    }

    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        switch (idx) {
        case 0: ImGui::Text("Static objects (walls, floor)"); break;
        case 1: ImGui::Text("Dynamic objects (player, movable items)"); break;
        case 2: ImGui::Text("Small debris (collides with static only)"); break;
        case 3: ImGui::Text("Trigger volumes (detects moving objects)"); break;
        }
        ImGui::EndTooltip();
    }

    return changed;  // Return the change state
}

// ---- MotionType enum drawer with automatic physics integration ----
inline bool DrawField(const char* label, PAIN::Physics::MotionType& motion_type) {
    const char* names[] = { "Static", "Dynamic", "Kinematic" };
    int idx = static_cast<int>(motion_type);
    bool changed = ImGui::Combo(label, &idx, names, 3);

    if (changed) {
        motion_type = static_cast<PAIN::Physics::MotionType>(idx);
        // Motion type changed - will be applied in next physics sync or via global update
    }

    return changed;
}

// ---- Collider drawer (Manual : Unions don't work in reflection) ----
inline bool DrawField(const char* label, PAIN::Collision::Collider& c) {
    bool changed = false;

    //ImGui::SeparatorText(label);

    // Shape (switching sets defaults for the active union member)
    if (DrawField("Shape", c.shape)) {
        changed = true;
        switch (c.shape) {
        case PAIN::SHAPE::Box:     c.box_size = glm::vec3(1.0f);     break;
        case PAIN::SHAPE::Sphere:  c.sphere_radius = 0.5f;           break;
        case PAIN::SHAPE::Capsule: c.capsule = { 0.25f, 1.0f };      break;
        case PAIN::SHAPE::Mesh:    /* no size fields */              break;
        }
    }

    // Active shape params
    switch (c.shape) {
    case PAIN::SHAPE::Box: {
        glm::vec3 s = c.box_size;
        if (ImGui::DragFloat3("Box Size", glm::value_ptr(s), 0.01f, 0.0f)) {
            c.box_size = glm::max(s, glm::vec3(0.0f));
            changed = true;
        }
    } break;

    case PAIN::SHAPE::Sphere: {
        float r = c.sphere_radius;
        if (ImGui::DragFloat("Radius", &r, 0.01f, 0.0f)) {
            c.sphere_radius = glm::max(r, 0.0f);
            changed = true;
        }
    } break;

    case PAIN::SHAPE::Capsule: {
        float r = c.capsule.radius;
        float h = c.capsule.height;
        if (ImGui::DragFloat("Capsule Radius", &r, 0.01f, 0.0f)) { c.capsule.radius = glm::max(r, 0.0f); changed = true; }
        if (ImGui::DragFloat("Capsule Height", &h, 0.01f, 0.0f)) { c.capsule.height = glm::max(h, 0.0f); changed = true; }
    } break;

    case PAIN::SHAPE::Mesh: {
        ImGui::TextDisabled("Mesh collider uses the mesh's triangles (no size input).");
    } break;
    }

    // Common properties
    {
        float f = c.friction;
        float b = c.restitution;
        if (ImGui::DragFloat("Friction", &f, 0.01f, 0.0f, 1.0f)) { c.friction = std::clamp(f, 0.0f, 1.0f); changed = true; }
        if (ImGui::DragFloat("Restitution", &b, 0.01f, 0.0f, 1.0f)) { c.restitution = std::clamp(b, 0.0f, 1.0f); changed = true; }

        // Existing uint16_t & bool drawers
        changed |= DrawField("Collision Layer", c.collision_layer);
        changed |= DrawField("Is Trigger", c.is_trigger);
    }

    return changed;
}

// ---------- Fallback for unknown types ----------
template <typename T>
inline bool DrawField(const char* label, T&) {
    ImGui::TextDisabled("No drawer for \"%s\"", label);
    return false;
}

// ========== MaterialInstance Custom Drawer ==========
// Place this with the other custom drawers in ReflectionUI.h

namespace PAIN { struct MaterialInstance; }

inline bool DrawField(const char* label, PAIN::MaterialInstance& mat, PAIN::Editor::Panel::ComponentsPanel* panel = nullptr) {
    bool changed = false;

    ImGui::PushID(&mat); // Use pointer as unique ID

    // Material Asset Selector
    if (panel) {
        changed |= DrawAssetSelectorField("Select A Material",
            mat.materialGUID,
            PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Material),
            panel->services);
    }
    else {
        ImGui::Text("Material GUID: %s", mat.materialGUID.ToString().c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Override Toggle
    if (ImGui::Checkbox("Use Property Overrides", &mat.useOverrides)) {
        changed = true;
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Enable to override base material properties for this instance");
    }

    // Only show override controls when enabled
    if (mat.useOverrides) {
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.3f, 0.4f, 0.8f));

        if (ImGui::CollapsingHeader("Material Overrides", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Indent(10.0f);

            //Textures override dropdown
            if(ImGui::CollapsingHeader("Texture Overrides")) {
                DrawAssetSelectorField("Albedo Texture Override",
                    mat.albedoTextureOverride,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    panel->services);

                DrawAssetSelectorField("Normal Texture Override",
                    mat.normalTextureOverride,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    panel->services);

                DrawAssetSelectorField("Metallic Texture Override",
                    mat.metallicTextureOverride,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    panel->services);

                DrawAssetSelectorField("Roughness Texture Override",
                    mat.roughnessTextureOverride,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    panel->services);

                DrawAssetSelectorField("AO Texture Override",
                    mat.aoTextureOverride,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    panel->services);

                DrawAssetSelectorField("Emissive Texture Override",
                    mat.emissiveTextureOverride,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    panel->services);

                DrawAssetSelectorField("Height Texture Override",
                    mat.heightTextureOverride,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    panel->services);

                DrawAssetSelectorField("Opacity Texture Override",
                    mat.opacityTextureOverride,
                    PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Texture),
                    panel->services);
            }

            // Base Color Override
            if (ImGui::ColorEdit3("Base Color", glm::value_ptr(mat.baseColorOverride))) {
                changed = true;
            }

            // Metallic Override
            if (ImGui::SliderFloat("Metallic", &mat.metallicOverride, 0.0f, 1.0f, "%.2f")) {
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("0 = Dielectric (non-metal), 1 = Metallic");
            }

            // Roughness Override
            if (ImGui::SliderFloat("Roughness", &mat.roughnessOverride, 0.0f, 1.0f, "%.2f")) {
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("0 = Smooth/Shiny, 1 = Rough/Matte");
            }

            // Emissive Override
            if (ImGui::ColorEdit3("Emissive Color", glm::value_ptr(mat.emissiveOverride))) {
                changed = true;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Self-illumination color (HDR values supported)");
            }

            ImGui::Unindent(10.0f);
        }

        ImGui::PopStyleColor();
    }

    ImGui::PopID();

    return changed;
}

// ========== std::vector<MaterialInstance> Drawer ==========
// This handles the entire vector with proper indexing

inline bool DrawField(const char* label, std::vector<PAIN::MaterialInstance>& materials, PAIN::Editor::Panel::ComponentsPanel* panel = nullptr) {
    bool changed = false;

    ImGui::PushID(label);

    // Header with material count
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.9f));

    std::string header = std::string(label) + " (" + std::to_string(materials.size()) + ")";
    bool open = ImGui::CollapsingHeader(header.c_str());

    ImGui::PopStyleColor();

    if (open) {
        ImGui::Indent(5.0f);

        if (materials.empty()) {
            ImGui::Spacing();
            ImGui::TextDisabled("No materials (model may not be loaded yet)");
            ImGui::Spacing();
        }
        else {
            for (size_t i = 0; i < materials.size(); ++i) {
                ImGui::PushID(static_cast<int>(i));

                // Material slot header
                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.25f, 0.35f, 0.8f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.3f, 0.4f, 0.9f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.35f, 0.45f, 1.0f));

                std::string slot_label = "(Submesh " + std::to_string(i) + ")";
                bool slot_open = ImGui::CollapsingHeader(slot_label.c_str());

                ImGui::PopStyleColor(3);

                if (slot_open) {
                    ImGui::Indent(10.0f);

                    // Draw the material instance
                    if (DrawField("##MaterialInstance", materials[i], panel)) {
                        changed = true;
                    }

                    ImGui::Unindent(10.0f);
                }

                ImGui::PopID();
                ImGui::Spacing();
            }
        }

        ImGui::Unindent(5.0f);
    }

    ImGui::PopID();

    return changed;
}

// ---------- Reflection driver ----------
 template <typename T>
bool DrawWithReflection(T& obj, PAIN::Editor::Panel::ComponentsPanel* panel = nullptr) {
    bool changed = false;

    constexpr auto type = refl::reflect<T>();
    refl::util::for_each(type.members, [&](auto m) {
        if constexpr (refl::descriptor::is_field(m)) {
            auto& field = m(obj);
            ImGui::PushID(m.name.c_str());

            // ------- Check for DisplayName -------
            const char* display_name = m.name.c_str();
            if constexpr (refl::descriptor::has_attribute<PAIN::Editor::Attributes::DisplayName>(m)) {
                display_name = refl::descriptor::get_attribute<PAIN::Editor::Attributes::DisplayName>(m).name;
            }

            // ------- Tooltip -------
            if constexpr (refl::descriptor::has_attribute<PAIN::Editor::Attributes::Tooltip>(m)) {
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", refl::descriptor::get_attribute<PAIN::Editor::Attributes::Tooltip>(m).text);
                }
            }

            // ------- ReadOnly -------
            bool readonly = false;
            if constexpr (refl::descriptor::has_attribute<ReadOnlyTag>(m)
                || refl::descriptor::has_attribute<PAIN::Editor::Attributes::ReadOnly>(m)) {
                readonly = true;
            }

            // ------- AssetSelector / Custom Draw -------
            if constexpr (refl::descriptor::has_attribute<PAIN::Editor::Attributes::AssetSelector>(m)) {
                // The field must be Asset GUID, or something compatible
                if (panel) {
                    auto attr = refl::descriptor::get_attribute<PAIN::Editor::Attributes::AssetSelector>(m);
                    // Replace with your asset dropdown renderer
                    changed |= DrawAssetSelectorField(display_name, field, attr, panel->services, readonly);
                }
            }
            // ------- Range, Dropdown, etc. -------
            else if constexpr (refl::descriptor::has_attribute<PAIN::Editor::Attributes::Range>(m)) {
                auto attr = refl::descriptor::get_attribute<PAIN::Editor::Attributes::Range>(m);
                if (!readonly) {
                    changed |= ImGui::SliderFloat(display_name, &field, attr.min, attr.max);
                }
                else {
                    ImGui::BeginDisabled();
                    ImGui::SliderFloat(display_name, &field, attr.min, attr.max);
                    ImGui::EndDisabled();
                }
            }
            // ------- Default Drawing Logic -------
            else {
                if (!readonly) {
                    changed |= DrawField(display_name, field);
                }
                else {
                    ImGui::BeginDisabled();
                    changed |= DrawField(display_name, field);
                    ImGui::EndDisabled();
                }
            }

            ImGui::PopID();
        }
        });
    return changed;
}


//template <typename T>
//bool DrawWithReflection(T& obj) {
//    bool changed = false;
//
//    constexpr auto type = refl::reflect<T>();         
//    refl::util::for_each(type.members, [&](auto m) {   
//        if constexpr (refl::descriptor::is_field(m)) {
//            auto& field = m(obj);
//            ImGui::PushID(m.name.c_str());
//            changed |= DrawField(m.name.c_str(), field);
//            ImGui::PopID();
//        }
//        });
//
//    return changed;
//}

namespace PAIN {
    namespace Editor {
        namespace Panel {
            template <typename T>
            inline void RegisterReflected(ComponentsPanel& panel, const char* title) {
                panel.registerCompUIFunc<T>([title](ComponentsPanel&, T& comp) {
                    //ImGui::TextUnformatted(title ? title : "Component");
                    //ImGui::Separator();
                    DrawWithReflection(comp);
                    });
            }

            //template <typename T>
            //inline void RegisterReflected(PAIN::Editor::Panel::ComponentsPanel& panel) {
            //    const char* ecs_key = getComponentName<T>();
            //    panel.registerCompUIFunc<T>(ecs_key, [](auto&, T& comp) {
            //        DrawWithReflection(comp); 
            //        });
            //}


            // Manual UI for collider comp
            inline void RegisterColliderUI(PAIN::Editor::Panel::ComponentsPanel& panel) {
                panel.registerCompUIFunc<Collision::Collider>("Collider",
                    [](PAIN::Editor::Panel::ComponentsPanel&, Collision::Collider& c) {
                        DrawField("Collider", c);  
                    });
            }

        }
    }
}
#endif
#endif