#pragma once
#include <refl.hpp>
#include <imgui.h>
#include "pch.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtc/constants.hpp>
#include "ComponentsPanel.h"
#include "ECS/Components/cLight.h"
#include "ECS/Components/cAudioSource.h"
#include "ECS/Components/cBoundingVolume.h"

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

    // Small helpers (don’t flip changed flags; they don’t edit the struct)
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

    return changed; // 'changed' will always be false
}

// ---------- Fallback for unknown types ----------
template <typename T>
inline bool DrawField(const char* label, T&) {
    ImGui::TextDisabled("No drawer for \"%s\"", label);
    return false;
}

// ---------- Reflection driver ----------
 template <typename T>
bool DrawWithReflection(T& obj) {
    bool changed = false;

    constexpr auto type = refl::reflect<T>();
    refl::util::for_each(type.members, [&](auto m) {
        if constexpr (refl::descriptor::is_field(m)) {
            auto& field = m(obj);
            ImGui::PushID(m.name.c_str());

            // If the field has ReadOnlyTag, grey it out
            if constexpr (refl::descriptor::has_attribute<ReadOnlyTag>(m)) {
                ImGui::BeginDisabled(true);
                changed |= DrawField(m.name.c_str(), field);
                ImGui::EndDisabled();
            }
            else {
                changed |= DrawField(m.name.c_str(), field);
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

#ifdef _DEBUG
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
        }
    }
}
#endif