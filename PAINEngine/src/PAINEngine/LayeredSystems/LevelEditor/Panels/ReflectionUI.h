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
            changed |= DrawField(m.name.c_str(), field);
            ImGui::PopID();
        }
        });

    return changed;
}

namespace PAIN {
    namespace Editor {
        namespace Panel {
            template <typename T>
            inline void RegisterReflected(ComponentsPanel& panel, const char* title) {
                panel.registerCompUIFunc<T>([title](ComponentsPanel&, T& comp) {
                    ImGui::TextUnformatted(title ? title : "Component");
                    ImGui::Separator();
                    DrawWithReflection(comp);
                    });
            }
        }
    }
}