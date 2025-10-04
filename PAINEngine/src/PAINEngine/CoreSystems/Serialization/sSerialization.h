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
#ifdef PN_PLATFORM_WINDOWS
#ifndef SERIALIZATION_SERVICE_H
#define SERIALIZATION_SERVICE_H
#include "pch.h"
#include "Applications/AppSystem.h"
#include "PAINEngine/ECS/Controller.h"
#include "PAINEngine/ECS/Components/cTransform.h"


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

        struct SceneDoc {
            int grid_id = 0;
            std::string active_cam_id;
            std::vector<std::string> meta_tags;

            struct Layer {
                int id = 0;         
                int mask = 1;        
                bool enabled = true; // B_State (visibility)
                // @todo add entities later
            };
            std::vector<Layer> layers;
            std::vector<std::vector<bool>> mask_matrix;
            bool dirty = false;
        };

        // ----------------------------
        // Service (inherits AppSystem)
        // ----------------------------
        class Service : public AppSystem {
        public:
            Service() = default;
            ~Service() = default;

            // AppSystem lifecycle
            void onAttach() override;         
            void onDetach() override;
            void onFixedUpdate(AppTiming tinming) override {}  
            void onUpdate(AppTiming tinming) override {}       
            void onAppPause() override {}
            void onAppResume() override {}
            void onEvent(Event::Event& e) override {}
            void modifyScene() { isModifiedScene = true; }

            // File helpers
            bool saveJsonFile(const std::string& file_path, const nlohmann::json& data);
            nlohmann::json loadJsonFile(const std::string& file_path);

            // Scene-level placeholders
            bool saveSceneToFile(const std::string& file_path);
            bool loadSceneFromFile(const std::string& file_path);

            const std::string& getCurrSceneFile() const { return curr_scene_file_; }
            static std::string MakeScenePathFromBase(std::string_view base); 

            const SceneDoc& doc() const { return doc_; }

            // Mutators
            void setGrid(int g);
            void setActiveCam(std::string id);
            void setTags(std::vector<std::string> t);

            void addLayer();
            void removeLayer(unsigned idx);
            void setLayerVisible(unsigned idx, bool v);
            void setLayerYSort(unsigned idx, bool v);
            void setMask(unsigned i, unsigned j, bool v);
            void ensureMaskSize();

            bool createNewScene(std::string_view baseName);  
            bool saveCurrentScene(); 
            bool saveSceneAs(std::string_view baseName);  
            bool loadSceneById(std::string_view sceneIdWithExt);  
            bool deleteSceneById(std::string_view sceneIdWithExt); 

        private:
            std::string curr_scene_file_;
            bool isModifiedScene = false;
            SceneDoc doc_;

            // helpers to convert 
            nlohmann::json to_json_from_doc_() const;
            void           doc_from_json_(const nlohmann::json& j);
        };

        // ----------------------------------------
        // Reflection helpers (header-only templates)
        // ----------------------------------------
        namespace detail {
            template <typename ConstString>
            inline std::string name_to_string(ConstString cs) {
                // refl::const_string supports .c_str() (or .str() depending on version).
                // Prefer .c_str() if available, else fall back to .str().
                if constexpr (requires { cs.c_str(); }) {
                    return std::string(cs.c_str());
                }
                else {
                    return std::string(cs.str());
                }
            }

            // Forward decls
            template <typename T>
            nlohmann::json to_json_value(const T& v);

            template <typename T>
            void from_json_value(T& out, const nlohmann::json& j);

            // ---------- detect reflectable ----------
            template <typename T>
            inline constexpr bool is_reflectable_v = refl::trait::is_reflectable_v<std::remove_cv_t<std::remove_reference_t<T>>>;

            // ---------- containers ----------
            // optional
            template <typename T>
            nlohmann::json to_json_value(const std::optional<T>& v) {
                if (!v) return nullptr;
                return to_json_value(*v);
            }
            template <typename T>
            void from_json_value(std::optional<T>& out, const nlohmann::json& j) {
                if (j.is_null()) { out.reset(); return; }
                T tmp{};
                from_json_value(tmp, j);
                out = std::move(tmp);
            }

            // vector
            //template <typename T>
            //nlohmann::json to_json_value(const std::vector<T>& vec) {
            //    nlohmann::json arr = nlohmann::json::array();
            //    for (const auto& e : vec) arr.push_back(to_json_value(e));
            //    return arr;
            //}

            template <typename T>
            nlohmann::json to_json_value(const std::vector<T>& vec) {
                nlohmann::json::array_t arr;
                arr.reserve(vec.size());            // reserve on the underlying vector
                for (const auto& e : vec) arr.emplace_back(to_json_value(e));
                return nlohmann::json(std::move(arr));
            }


            template <typename T>
            void from_json_value(std::vector<T>& out, const nlohmann::json& j) {
                out.clear();
                if (!j.is_array()) return;
                out.reserve(j.size());              //reserving the std::vector we output to
                for (const auto& je : j) {
                    T v{};
                    from_json_value(v, je);
                    out.push_back(std::move(v));
                }
            }


            // maps with string-like keys
            template <typename V>
            nlohmann::json to_json_value(const std::map<std::string, V>& m) {
                nlohmann::json obj = nlohmann::json::object();
                for (auto& [k, v] : m) obj[k] = to_json_value(v);
                return obj;
            }
            template <typename V>
            void from_json_value(std::map<std::string, V>& out, const nlohmann::json& j) {
                out.clear();
                if (!j.is_object()) return;
                for (auto it = j.begin(); it != j.end(); ++it) {
                    V v{};
                    from_json_value(v, it.value());
                    out.emplace(it.key(), std::move(v));
                }
            }
            template <typename V>
            nlohmann::json to_json_value(const std::unordered_map<std::string, V>& m) {
                nlohmann::json obj = nlohmann::json::object();
                for (auto& [k, v] : m) obj[k] = to_json_value(v);
                return obj;
            }
            template <typename V>
            void from_json_value(std::unordered_map<std::string, V>& out, const nlohmann::json& j) {
                out.clear();
                if (!j.is_object()) return;
                for (auto it = j.begin(); it != j.end(); ++it) {
                    V v{};
                    from_json_value(v, it.value());
                    out.emplace(it.key(), std::move(v));
                }
            }

            // ---------- enums: default to underlying ----------
            template <typename E>
            nlohmann::json enum_to_json(E e, std::true_type /*is_enum*/) {
                using U = std::underlying_type_t<E>;
                return nlohmann::json(static_cast<U>(e));
            }
            template <typename T>
            nlohmann::json enum_to_json(const T& v, std::false_type /*not enum*/) {
                return nlohmann::json(v); // fallback
            }

            template <typename E>
            void enum_from_json(E& out, const nlohmann::json& j, std::true_type /*is_enum*/) {
                using U = std::underlying_type_t<E>;
                out = static_cast<E>(j.get<U>());
            }
            template <typename T>
            void enum_from_json(T& out, const nlohmann::json& j, std::false_type /*not enum*/) {
                out = j.get<T>();
            }

            // ---------- general value -> json ----------
            template <typename T>
            nlohmann::json to_json_value(const T& v) {
                if constexpr (is_reflectable_v<T>) {
                    return ::PAIN::Serialization::to_json_reflected(v);
                }
                else if constexpr (std::is_enum_v<std::remove_cv_t<std::remove_reference_t<T>>>) {
                    return enum_to_json(v, std::true_type{});
                }
                else {
                    // Let nlohmann handle it (including your GLM adapters and any ADL to_json)
                    return nlohmann::json(v);
                }
            }

            // ---------- json -> general value ----------
            template <typename T>
            void from_json_value(T& out, const nlohmann::json& j) {
                if constexpr (is_reflectable_v<T>) {
                    ::PAIN::Serialization::from_json_reflected(out, j);
                }
                else if constexpr (std::is_enum_v<std::remove_cv_t<std::remove_reference_t<T>>>) {
                    enum_from_json(out, j, std::true_type{});
                }
                else {
                    // Fallback to nlohmann's get
                    out = j.get<T>();
                }
            }
        } // namespace detail

      // ---------- T -> json for reflected types ----------
        template <typename T>
        nlohmann::json to_json_reflected(const T& obj) {
            // Check for non-reflected types
            static_assert(refl::trait::is_reflectable_v<T>,
                "Type T is not reflected with REFL_TYPE/REFL_FIELD");

            using namespace refl;
            nlohmann::json j = nlohmann::json::object();

            constexpr auto type = reflect<T>();
            util::for_each(type.members, [&](auto member) {
                using MemberT = decltype(member);
                if constexpr (refl::trait::is_field_v<MemberT>) {
                    constexpr auto cname = member.name;
                    const auto key = detail::name_to_string(cname);

                    const auto& value = member.get(obj);
                    using V = std::remove_cv_t<std::remove_reference_t<decltype(value)>>;

                    // Fast path for primitives & common scalars
                    if constexpr (std::is_arithmetic_v<V> ||
                        std::is_same_v<V, std::string> ||
                        std::is_same_v<V, bool>) {
                        j[key] = value;
                    }
                    else {
                        j[key] = detail::to_json_value(value);
                    }
                }
                });

            return j;
        }

        // ---------- json -> T for reflected types ----------
        template <typename T>
        void from_json_reflected(T& obj, const nlohmann::json& j) {
            // Check for non-reflected types
            static_assert(refl::trait::is_reflectable_v<T>,
                "Type T is not reflected with REFL_TYPE/REFL_FIELD");

            using namespace refl;
            constexpr auto type = reflect<T>();

            util::for_each(type.members, [&](auto member) {
                using MemberT = decltype(member);
                if constexpr (refl::trait::is_field_v<MemberT>) {
                    constexpr auto cname = member.name;
                    const auto key = detail::name_to_string(cname);

                    if (!j.contains(key)) return;

                    using FieldRef = decltype(member.get(obj));
                    using Field = std::remove_reference_t<FieldRef>;
                    auto& field_ref = member.get(obj);

                    // Fast path for primitives & common scalars
                    if constexpr (std::is_arithmetic_v<Field> ||
                        std::is_same_v<Field, std::string> ||
                        std::is_same_v<Field, bool>) {
                        field_ref = j.at(key).get<Field>();
                    }
                    else {
                        detail::from_json_value(static_cast<Field&>(field_ref), j.at(key));
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
#endif