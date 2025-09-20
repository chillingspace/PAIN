/*****************************************************************//**
 * \file   sSerialization.h
 * \brief  Declaration of serialization service
 *
 * \author Bryan Soh, 2301238, z.soh@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content  2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/
#pragma once
#ifndef SERIALIZATION_SERVICE_H
#define SERIALIZATION_SERVICE_H
#include "pch.h"
#include "Applications/AppSystem.h"

namespace PAIN {
	namespace Serialization {

		//Temporary Disable DLL Export Warning
		//#pragma warning(disable: 4251)

		// ----------------------------
		// JSON <-> reflected structs
		// ----------------------------
		template <typename T>
		nlohmann::json to_json_reflected(const T& obj);

		template <typename T>
		void from_json_reflected(T& obj, const nlohmann::json& j);


        // ----------------------------
        // Service (inherits AppSystem)
        // ----------------------------
        class Service : public AppSystem {
        public:
            Service() = default;
            virtual ~Service() = default;

            // AppSystem lifecycle
            void onAttach() override {}             // optional: load autosave, etc.
            void onDetach() override {}             // optional: flush saves
            void onUpdate() override {}             // keep empty for now (no ECS yet)
            void onEvent(Event::Event& e) override {} // wire hotkeys later (Ctrl+S, etc.)

            // File helpers
            bool saveJsonFile(const std::string& file_path, const nlohmann::json& data);
            nlohmann::json loadJsonFile(const std::string& file_path);

            // Scene-level placeholders (no entities yet)
            bool saveSceneToFile(const std::string& file_path);
            bool loadSceneFromFile(const std::string& file_path);

            const std::string& getCurrSceneFile() const { return curr_scene_file_; }

        private:
            std::string curr_scene_file_;
        };

        // ----------------------------------------
        // Reflection helpers (header-only templates)
        // ----------------------------------------

        template <typename T>
        nlohmann::json to_json_reflected(const T& obj) {
            // Check for non-reflected types
            static_assert(refl::trait::is_reflectable_v<T>,
                "Type T is not reflected with REFL_TYPE/REFL_FIELD");
            using namespace refl;
            nlohmann::json j = nlohmann::json::object();

            constexpr auto type = reflect<T>();
            refl::util::for_each(type.members, [&](auto member) {
                if constexpr (refl::trait::is_field(member)) {
                    constexpr auto cname = member.name;
                    const auto& value = member.get(obj);
                    j[std::string(cname.str())] = value; // requires to_json for custom value types
                }
                });
            return j;
        }

        template <typename T>
        void from_json_reflected(T& obj, const nlohmann::json& j) {
            // Check for non-reflected types
            static_assert(refl::trait::is_reflectable_v<T>,
                "Type T is not reflected with REFL_TYPE/REFL_FIELD");

            using namespace refl;
            constexpr auto type = reflect<T>();
            refl::util::for_each(type.members, [&](auto member) {
                if constexpr (refl::trait::is_field(member)) {
                    const std::string key = std::string(member.name.str());
                    if (j.contains(key)) {
                        using FieldT = std::decay_t<decltype(member.get(obj))>;
                        member.set(obj, j.at(key).get<FieldT>());
                    }
                }
                });
        }

        // ----------------------------------------
        // Adapters for common external value types
        // (Optional; useful once you reflect types using GLM)
        // ----------------------------------------
        #ifdef GLM_VERSION
        inline void to_json(nlohmann::json& j, const glm::vec2& v) { j = nlohmann::json::array({ v.x, v.y }); }
        inline void from_json(const nlohmann::json& j, glm::vec2& v) { v = { j.at(0).get<float>(), j.at(1).get<float>() }; }

        inline void to_json(nlohmann::json& j, const glm::vec3& v) { j = nlohmann::json::array({ v.x, v.y, v.z }); }
        inline void from_json(const nlohmann::json& j, glm::vec3& v) { v = { j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>() }; }

        inline void to_json(nlohmann::json& j, const glm::vec4& v) { j = nlohmann::json::array({ v.x, v.y, v.z, v.w }); }
        inline void from_json(const nlohmann::json& j, glm::vec4& v) { v = { j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>(), j.at(3).get<float>() }; }

        inline void to_json(nlohmann::json& j, const glm::quat& q) { j = nlohmann::json::array({ q.x, q.y, q.z, q.w }); }
        inline void from_json(const nlohmann::json& j, glm::quat& q) { q = glm::quat{ j.at(3).get<float>(), j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>() }; }
        #endif

	}
}
#endif