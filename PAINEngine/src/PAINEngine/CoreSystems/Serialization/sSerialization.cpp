/*****************************************************************//**
 * \file   sSerialization.cpp
 * \brief  Definition of serialization service
 *
 * \author Bryan Soh, 2301238, z.soh@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content  2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "sSerialization.h"

 // ---- quick reflected test type ----
struct _SerSmokeTransform {
    float x{}, y{}, z{};
};
REFL_TYPE(_SerSmokeTransform)
REFL_FIELD(x)
REFL_FIELD(y)
REFL_FIELD(z)
REFL_END

//static_assert(refl::trait::is_reflectable_v<_SerSmokeTransform>,
//    "_SerSmokeTransform is not reflectable in this TU");

template <typename T>
void DebugDumpReflectionAndJson(const T& obj, const nlohmann::json& j) {
    using namespace refl;
    constexpr auto type = reflect<T>();

    PN_CORE_INFO("[DEBUG] JSON dump:\n{0}\n", j.dump(2));
    PN_CORE_INFO("[DEBUG] Members:");

    util::for_each(type.members, [&](auto member) {
        using MemberT = decltype(member);
        if constexpr (refl::trait::is_field_v<MemberT>) {
            // Try both .c_str() and .str() depending on refl-cpp version
            std::string key;
            if constexpr (requires { member.name.c_str(); }) key = member.name.c_str();
            else key = std::string(member.name.str());


            PN_CORE_INFO("  - field name: {0}, in JSON? {1}", key, j.contains(key) ? "yes" : "no");

        }
        });
}


namespace fs = std::filesystem;

namespace PAIN {
    namespace Serialization {
        void PAIN::Serialization::Service::onAttach() {
            // 1) Write a minimal scene file
            const std::string scenePath = "assets/smoke.scene.json";
            saveSceneToFile(scenePath);

            // 2) Round-trip a reflected struct
            _SerSmokeTransform t{ 1.0f, 2.0f, 3.0f };
            nlohmann::json j = to_json_reflected(t);

            // DEBUG: dump what we serialized
            PN_CORE_INFO("[Serialization] Serialized JSON:\n{0}", j.dump(2));

            const std::string dataPath = "assets/smoke.data.json";
            saveJsonFile(dataPath, j);

            // DEBUG: confirm file written
            if (!std::filesystem::exists(dataPath)) {
                PN_CORE_INFO("[Serialization] File does not exist: {0}", dataPath);
            }

            // Load back and compare
            _SerSmokeTransform t2{};
            auto jIn = loadJsonFile(dataPath);

            // DEBUG: dump what we loaded
            PN_CORE_INFO("[Serialization] Loaded JSON:\n{0}", jIn.dump(2));

            if (!jIn.is_null() && !jIn.empty()) {
                from_json_reflected(t2, jIn);
            }

            // Optional: simple assert/log
            const bool ok = (t.x == t2.x && t.y == t2.y && t.z == t2.z);
            if (!ok) {
                PN_CORE_INFO("[Serialization] Round-trip FAILED "
                    "(expected: {0},{1},{2} got: {3},{4},{5})",
                    t.x, t.y, t.z, t2.x, t2.y, t2.z);
            }
            else {
                PN_CORE_INFO("[Serialization] Round-trip OK");
            }
        }

        // END OF DEBUG


        // ----------------------------
        // File helpers
        // ----------------------------
        bool Service::saveJsonFile(const std::string& file_path, const nlohmann::json& data) {
            try {
                fs::path p{ file_path };
                if (p.has_parent_path()) {
                    fs::create_directories(p.parent_path());
                }
                std::ofstream out(file_path, std::ios::binary | std::ios::trunc);
                if (!out) return false;
                out << data.dump(4);
                return true;
            }
            catch (...) {
                return false;
            }
        }

        nlohmann::json Service::loadJsonFile(const std::string& file_path) {
            nlohmann::json j;
            try {
                std::ifstream in(file_path, std::ios::binary);
                if (!in) return j; // empty on failure
                in >> j;
            }
            catch (...) {
                // leave empty
            }
            return j;
        }

        // ----------------------------
        // Scene-level placeholders
        // ----------------------------
        bool Service::saveSceneToFile(const std::string& file_path) {
            nlohmann::json scene;
            scene["Scene"] = {
                { "name",    "Untitled" },
                { "version", 1 },
                { "savedAt", std::time(nullptr) }
            };
            const bool ok = saveJsonFile(file_path, scene);
            if (ok) curr_scene_file_ = file_path;
            return ok;
        }

        bool Service::loadSceneFromFile(const std::string& file_path) {
            auto j = loadJsonFile(file_path);
            if (j.is_null() || j.empty()) return false;
            // Later: read camera/editor/system blocks via from_json_reflected
            curr_scene_file_ = file_path;
            return true;
        }
    }
}