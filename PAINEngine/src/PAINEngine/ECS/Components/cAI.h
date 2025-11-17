/*****************************************************************//**
 * \file   cAI.h
 * \brief  AI components
 *
 * \author Bryan Sohh, 2301238,  z.soh@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content  2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/
#pragma once

#ifndef C_AI_H
#define C_AI_H


#include "pch.h"

namespace PAIN {
	namespace AI {
		using EntityId = entt::entity;

        /*--------------------  Blackboard  --------------------*/
        struct Blackboard {
            // simple, engine-owned KV store. Extend types as needed.
            using Value = std::variant<double, bool, glm::vec3, std::uint32_t>;
            std::unordered_map<std::string, Value> data;

            template<typename T>
            void set(std::string_view key, const T& v) { data[std::string(key)] = v; }

            template<typename T>
            std::optional<T> get(std::string_view key) const {
                auto it = data.find(std::string(key));
                if (it == data.end()) return std::nullopt;
                if (auto p = std::get_if<T>(&it->second)) return *p;
                return std::nullopt;
            }

            bool get_bool(std::string_view key, bool def = false) const {
                if (auto v = get<bool>(key)) return *v; return def;
            }
            double get_num(std::string_view key, double def = 0.0) const {
                if (auto v = get<double>(key)) return *v; return def;
            }
            glm::vec3 get_vec3(std::string_view key, glm::vec3 def = {}) const {
                if (auto v = get<glm::vec3>(key)) return *v; return def;
            }
        }; // Blackboard

        /*--------------------  Controller  --------------------*/
        struct Controller {
            std::string behavior_asset_id; // e.g. "behaviors/guard_patrol.bt"
            bool enabled = true;
            float tick_interval = 0.1f;    // seconds between behavior ticks
            float accum_dt = 0.0f;         // internal accumulator
        }; // Controller

        /*--------------------  Sensors  --------------------*/
        struct SensorsConfig {
            float sight_range = 25.0f;
            float sight_fov_deg = 120.0f;
            float hear_range = 20.0f;
            bool  require_los = true;
            std::uint32_t los_collision_mask = 0xFFFFFFFFu; // layers
        };

        struct Sensors {
            SensorsConfig cfg;
            // rolling perception results (read-only to Lua)
            std::vector<EntityId> visible_targets;
            std::optional<glm::vec3> last_noise_pos;   // world-space
            double last_noise_time = -1.0;
        };
	}
}

#endif