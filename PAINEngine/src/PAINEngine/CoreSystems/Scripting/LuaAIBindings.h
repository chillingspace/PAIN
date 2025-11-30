#pragma once

#include "sol_sanitized.h"
#include <entt/entity/registry.hpp>
#include <glm/vec3.hpp>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

#include "PAINEngine/ECS/Components/cAI.h"          
#include "PAINEngine/ECS/Components/cTransform.h"
#include "PAINEngine/CoreSystems/Scripting/IEngineAPI.h"

namespace PAIN::Scripting {

    // --------------------  Blackboard accessor exposed to Lua  --------------------
    struct BBAccessor {
        entt::registry* reg{ nullptr };
        entt::entity    e{ entt::null };

        AI::Blackboard* bb() const {
            if (!reg || e == entt::null) return nullptr;
            if (!reg->all_of<AI::Blackboard>(e)) return nullptr;
            return &reg->get<AI::Blackboard>(e);
        }

        // setters
        void set_number(const std::string& k, double v) {
            if (auto* b = bb()) b->set<double>(k, v);
        }

        void set_bool(const std::string& k, bool v) {
            if (auto* b = bb()) b->set<bool>(k, v);
        }

        void set_vec3(const std::string& k, float x, float y, float z) {
            if (auto* b = bb()) b->set<glm::vec3>(k, glm::vec3{x, y, z});
        }

        void set_u32(const std::string& k, std::uint32_t v) {
            if (auto* b = bb()) b->set<std::uint32_t>(k, v);
        }

        // getters with defaults (Lua can call get_number("hp") or get_number("hp", 100.0))
        double get_number(const std::string& k, double def = 0.0) const {
            if (auto* b = const_cast<BBAccessor*>(this)->bb()) {
                return b->get_num(k, def);
            }
            return def;
        }

        bool get_bool(const std::string& k, bool def = false) const {
            if (auto* b = const_cast<BBAccessor*>(this)->bb()) {
                return b->get_bool(k, def);
            }
            return def;
        }

        glm::vec3 get_vec3(const std::string& k,
                           float defx = 0.f,
                           float defy = 0.f,
                           float defz = 0.f) const {
            if (auto* b = const_cast<BBAccessor*>(this)->bb()) {
                return b->get_vec3(k, glm::vec3{defx, defy, defz});
            }
            return glm::vec3{defx, defy, defz};
        }

        std::uint32_t get_u32(const std::string& k, std::uint32_t def = 0) const {
            if (auto* b = const_cast<BBAccessor*>(this)->bb()) {
                if (auto v = b->get<std::uint32_t>(k)) return *v;
            }
            return def;
        }
    };

    // --------------------  AI high-level API exposed to Lua  --------------------
    struct AIAPI {
        entt::registry*    reg{ nullptr };
        entt::entity       e{ entt::null };
        PAIN::IEngineAPI*  api{ nullptr }; 

        bool valid() const {
            return reg && e != entt::null;
        }

        AI::Sensors* sensors() const {
            if (!valid()) return nullptr;
            if (!reg->all_of<AI::Sensors>(e)) return nullptr;
            return &reg->get<AI::Sensors>(e);
        }

        AI::CommandQueue* queue() const {
            if (!valid()) return nullptr;
            return &reg->get_or_emplace<AI::CommandQueue>(e);
        }

        // --- Perception snapshots (read-only) ---

        // Returns { u32, u32, ... } of visible targets this frame
        std::vector<std::uint32_t> get_visible_enemies() const {
            std::vector<std::uint32_t> out;
            auto* s = sensors();
            if (!s) return out;
            out.reserve(s->visible_targets.size());
            for (auto t : s->visible_targets) {
                out.push_back(static_cast<std::uint32_t>(t));
            }
            return out;
        }

        // Returns vec3 or nil
        std::optional<glm::vec3> get_last_noise_pos() const {
            auto* s = sensors();
            if (!s) return std::nullopt;
            if (!s->last_noise_pos.has_value()) return std::nullopt;
            return s->last_noise_pos;
        }

        // Safe position query (world-space)
        std::optional<glm::vec3> get_entity_pos(std::uint32_t eid) const {
            if (!reg) return std::nullopt;
            entt::entity te = static_cast<entt::entity>(eid);
            if (!reg->all_of<PAIN::LocalTransform>(te)) return std::nullopt;
            const auto& t = reg->get<PAIN::LocalTransform>(te);
            return t.position;
        }

        // --- High-level actions (enqueue commands; Navigation/Steering will act) ---

        void set_move_target(float x, float y, float z) {
            auto* q = queue();
            if (!q) return;
            AI::Command c;
            c.type = AI::CommandType::SetMoveTarget;
            c.v3   = glm::vec3{x, y, z};
            q->push(c);
        }

        void clear_move_target() {
            auto* q = queue();
            if (!q) return;
            AI::Command c;
            c.type = AI::CommandType::ClearMoveTarget;
            q->push(c);
        }

        void request_path() {
            auto* q = queue();
            if (!q) return;
            AI::Command c;
            c.type = AI::CommandType::RequestPath;
            q->push(c);
        }

        void play_anim(const std::string& name) {
            auto* q = queue();
            if (!q) return;
            AI::Command c;
            c.type = AI::CommandType::PlayAnimation;
            c.str  = name;
            q->push(c);
        }

        void face_entity(std::uint32_t target) {
            auto* q = queue();
            if (!q) return;
            AI::Command c;
            c.type   = AI::CommandType::FaceEntity;
            c.target = static_cast<entt::entity>(target);
            q->push(c);
        }

        std::uint32_t get_self_id() const {
            return static_cast<std::uint32_t>(e);
        }

        glm::vec3 get_self_pos() const {
            if (!reg || e == entt::null) return glm::vec3(0.f);
            if (!reg->all_of<PAIN::LocalTransform>(e)) return glm::vec3(0.f);
            return reg->get<PAIN::LocalTransform>(e).position;
        }
    };

    // --------------------  Context creation & registration  --------------------
    inline sol::table make_ai_context(sol::state_view L,
                                      entt::registry& reg,
                                      entt::entity e,
                                      PAIN::IEngineAPI* api)
    {
        static bool registered = false;
        if (!registered) {
            registered = true;

            L.new_usertype<BBAccessor>("BBAccessor",
                sol::no_constructor,
                "set_number", &BBAccessor::set_number,
                "set_bool",   &BBAccessor::set_bool,
                "set_vec3",   &BBAccessor::set_vec3,
                "set_u32",    &BBAccessor::set_u32,
                "get_number", &BBAccessor::get_number,
                "get_bool",   &BBAccessor::get_bool,
                "get_vec3",   &BBAccessor::get_vec3,
                "get_u32",    &BBAccessor::get_u32
            );

            L.new_usertype<AIAPI>("AIAPI",
                sol::no_constructor,
                "get_visible_enemies", &AIAPI::get_visible_enemies,
                "get_last_noise_pos",  &AIAPI::get_last_noise_pos,
                "get_entity_pos",      &AIAPI::get_entity_pos,
                "set_move_target",     &AIAPI::set_move_target,
                "clear_move_target",   &AIAPI::clear_move_target,
                "request_path",        &AIAPI::request_path,
                "play_anim",           &AIAPI::play_anim,
                "face_entity",         &AIAPI::face_entity,
                "get_self_id",         &AIAPI::get_self_id,     
                "get_self_pos",        &AIAPI::get_self_pos
            );
        }

        sol::table ctx = L.create_table();
        ctx["bb"] = BBAccessor{ &reg, e };
        ctx["ai"] = AIAPI{ &reg, e, api };
        return ctx;
    }

} // namespace PAIN::Scripting
