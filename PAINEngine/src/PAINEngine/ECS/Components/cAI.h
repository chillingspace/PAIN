/*****************************************************************//**
 * \file   cAI.h
 * \brief  AI components
 *
 * \author Bryan Soh, 2301238,  z.soh@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content  2024 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/
#pragma once

#ifndef C_AI_H
#define C_AI_H


#include "pch.h"
#include <variant>

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
            std::string behavior_asset; // e.g. "behaviors/guard_patrol.bt"
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

        /*--------------------  Navigation / Steering  --------------------*/
        struct NavAgent {
            float speed = 4.0f;
            float accel = 20.0f;
            float radius = 0.4f;
            float arrival_radius = 0.5f;

            // path state
            std::vector<glm::vec3> path;
            std::size_t path_index = 0;
            bool has_request_in_flight = false;
            bool arrived = false;

            // targets
            std::optional<glm::vec3> move_target;
            float replan_cooldown = 0.2f;
            float replan_timer = 0.0f;

            // stuck detection
            glm::vec3 last_pos{};
            float dist_accum = 0.0f;
            float stuck_timer = 0.0f;
        };

        struct Steering {
            glm::vec3 desired_velocity{};
            bool avoidance = true;
        };

        /*--------------------  Command Queue (per-entity, filled by behaviors)  --------------------*/
        enum class CommandType : std::uint8_t {
            None, SetMoveTarget, ClearMoveTarget, PlayAnimation, FaceEntity, RequestPath
        };

        struct Command {
            CommandType type{ CommandType::None };
            glm::vec3 v3{};
            std::string str;
            EntityId target{ entt::null };
            float f0{ 0.0f };
        };

        struct CommandQueue {
            std::vector<Command> pending;
            void push(const Command& c) { pending.emplace_back(c); }
            void clear() { pending.clear(); }
        };
	}
}

// Serializer 
namespace nlohmann {

    // Command Queue
    template<>
    struct adl_serializer<PAIN::AI::CommandQueue> {
        static void to_json(json& j, const PAIN::AI::CommandQueue& q) {
            // Don't serialize pending commands; just store empty object
            j = json::object();
            // For debug info
            // j["pending_count"] = q.pending.size();
        }

        static void from_json(const json& j, PAIN::AI::CommandQueue& q) {
            // Ignore whatever is in JSON; always start fresh
            q.pending.clear();
        }
    };
}

// Reflections

namespace PAIN::Editor::Attributes {
    // One global/constexpr instance describing "this field selects Script assets"
    static constexpr AssetSelector BehaviorScriptSelector{ PAIN::Assets::Type::Script };
}

// Blackboard not registered as not needed to expose to ui

// Controller
REFL_TYPE(PAIN::AI::Controller)
    //REFL_FIELD(behavior_asset)
    REFL_FIELD(behavior_asset, PAIN::Editor::Attributes::AssetSelector(PAIN::Assets::Type::Script))
    REFL_FIELD(enabled)
    REFL_FIELD(tick_interval)
    REFL_FIELD(accum_dt)
REFL_END

// Sensors Config
REFL_TYPE(PAIN::AI::SensorsConfig)
    REFL_FIELD(sight_range)
    REFL_FIELD(sight_fov_deg)
    REFL_FIELD(hear_range)
    REFL_FIELD(require_los)
    REFL_FIELD(los_collision_mask)
REFL_END

// Sensors
REFL_TYPE(PAIN::AI::Sensors)
    REFL_FIELD(cfg)
REFL_END

// Nav Agent
REFL_TYPE(PAIN::AI::NavAgent)
    REFL_FIELD(speed)
    REFL_FIELD(accel)
    REFL_FIELD(radius)
    REFL_FIELD(arrival_radius)
    REFL_FIELD(replan_cooldown)
REFL_END

// Steering
REFL_TYPE(PAIN::AI::Steering)
    REFL_FIELD(desired_velocity)
    REFL_FIELD(avoidance)
REFL_END

#endif