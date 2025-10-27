#pragma once

#ifndef GLM_SERI
#define GLM_SERI

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace glm {
    // Serialize glm::vec3
    inline void to_json(nlohmann::json& j, const glm::vec3& v) {
        j = nlohmann::json::array({ v.x, v.y, v.z });
    }

    inline void from_json(const nlohmann::json& j, glm::vec3& v) {
        v.x = j[0].get<float>();
        v.y = j[1].get<float>();
        v.z = j[2].get<float>();
    }

    // Serialize glm::quat
    inline void to_json(nlohmann::json& j, const glm::quat& q) {
        j = nlohmann::json::array({ q.x, q.y, q.z, q.w });
    }

    inline void from_json(const nlohmann::json& j, glm::quat& q) {
        q.x = j[0].get<float>();
        q.y = j[1].get<float>();
        q.z = j[2].get<float>();
        q.w = j[3].get<float>();
    }
}  

#endif
