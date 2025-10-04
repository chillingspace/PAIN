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

// Will throw error if the test struct is not reflectable
static_assert(refl::trait::is_reflectable_v<_SerSmokeTransform>, "_SerSmokeTransform not reflectable");

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

        inline std::string sanitize_base(std::string s) {
            s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
                return !(std::isalnum(c) || c == '_' || c == '-');
                }), s.end());
            return s;
        }

        void PAIN::Serialization::Service::onAttach() {
            // Placeholder path
            const std::string scenePath = MakeScenePathFromBase("lvl1_1");

            PN_CORE_INFO("[Serialization] Attempting to load scene: {0}", scenePath);

            if (loadSceneFromFile(scenePath)) {
                PN_CORE_INFO("[Serialization] Scene loaded successfully: {0}", scenePath);
            }
            else {
                PN_CORE_INFO("[Serialization] Scene load FAILED: {0}", scenePath);
            }
        }

        void PAIN::Serialization::Service::onDetach() {
            if (!curr_scene_file_.empty()) {
                if (isModifiedScene) {
                    PN_CORE_INFO("[Serialization] Autosave on shutdown: {}", curr_scene_file_);
                    if (!saveCurrentScene()) {
                        PN_CORE_WARN("[Serialization] Autosave FAILED: {}", curr_scene_file_);
                    }
                }
            }
        }

        /*void PAIN::Serialization::Service::onAttach() {
           
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
            
        }*/

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
            // TO DO: ADD THIS ONCE ENTITY AND SCENE ARE READY
            //return false;

            // Convert our in-memory document to JSON
            nlohmann::json scene = to_json_from_doc_();

            // Write to disk
            const bool ok = saveJsonFile(file_path, scene);
            if (ok) {
                curr_scene_file_ = file_path;
                isModifiedScene = false;   // clear dirty flag after saving
            }
            return ok;
        }

        bool Service::loadSceneFromFile(const std::string& file_path) {
            auto j = loadJsonFile(file_path);
            if (j.is_null() || j.empty()) {
                PN_CORE_INFO("[Scene] loadSceneFromFile: empty or invalid: {0}", file_path);
                return false;
            }
            if (!j.is_array()) {
                PN_CORE_INFO("[Scene] loadSceneFromFile: expected array at root (got {0})", j.type_name());
                return false;
            }

            // Helper: find first section by key (e.g., "Camera", "Grid ID", etc.)
            auto findSection = [&](const char* key) -> const nlohmann::json* {
                for (const auto& elem : j) {
                    if (!elem.is_object()) continue;
                    auto it = elem.find(key);
                    if (it != elem.end()) return &(*it);
                }
                return nullptr;
                };

            // Build a compact report JSON we can log/save
            nlohmann::json report;
            report["file"] = file_path;

            // Grid ID
            if (const auto* grid = findSection("Grid ID"); grid && grid->is_number_integer())
                report["grid_id"] = grid->get<int>();
            else
                report["grid_id"] = nullptr;

            // Camera
            if (const auto* cam = findSection("Camera"); cam && cam->is_object()) {
                auto it = cam->find("Active Cam ID");
                report["active_cam_id"] = (it != cam->end() && it->is_string()) ? *it : nlohmann::json(nullptr);
            }
            else {
                report["active_cam_id"] = nullptr;
            }

            // Meta tags
            if (const auto* meta = findSection("MetaData"); meta && meta->is_object()) {
                auto it = meta->find("Entity_Tags");
                report["meta_tags"] = (it != meta->end() && it->is_array()) ? *it : nlohmann::json::array();
            }
            else {
                report["meta_tags"] = nlohmann::json::array();
            }

            // Declared layer count
            if (const auto* lc = findSection("Layer Count"); lc && lc->is_number_integer())
                report["layer_count_declared"] = lc->get<int>();
            else
                report["layer_count_declared"] = nullptr;

            // Layers summary
            nlohmann::json layers_out = nlohmann::json::array();
            size_t total_entities = 0;

            for (const auto& elem : j) {
                auto it = elem.find("Layer");
                if (it == elem.end() || !it->is_object()) continue;
                const auto& L = *it;

                nlohmann::json Lout;
                Lout["ID"] = L.value("ID", 0);
                Lout["Mask"] = L.value("Mask", 0);
                Lout["B_State"] = L.value("B_State", true);

                // Entities
                nlohmann::json ents_out = nlohmann::json::array();
                if (auto ents = L.find("Entities"); ents != L.end() && ents->is_array()) {
                    for (const auto& ewrap : *ents) {
                        if (!ewrap.is_object()) continue;
                        auto eit = ewrap.find("Entity");
                        if (eit == ewrap.end() || !eit->is_object()) continue;

                        const auto& E = *eit;

                        nlohmann::json Eout;
                        // Not all scenes have a name/id here; include if present
                        if (auto n = E.find("Name"); n != E.end() && n->is_string()) Eout["Name"] = *n;
                        if (auto id = E.find("ID"); id != E.end())                  Eout["ID"] = *id;

                        // Components: output only the component names (keys)
                        nlohmann::json comps_names = nlohmann::json::array();
                        if (auto comps = E.find("Components"); comps != E.end() && comps->is_object()) {
                            for (auto cj = comps->begin(); cj != comps->end(); ++cj) {
                                comps_names.push_back(cj.key()); // e.g. "Transform::Transform", "Render::Texture"
                            }
                        }
                        Eout["Components"] = comps_names;
                        ents_out.push_back(std::move(Eout));
                    }
                }
                Lout["entity_count"] = ents_out.size();
                Lout["Entities"] = std::move(ents_out);
                total_entities += static_cast<size_t>(Lout["entity_count"]);
                layers_out.push_back(std::move(Lout));
            }

            report["layers"] = std::move(layers_out);
            report["total_entities"] = total_entities;

            // Log a concise summary
            PN_CORE_INFO("[Scene] {0}: grid={1}, active_cam='{2}', layers={3}, entities={4}",
                file_path,
                report["grid_id"].is_null() ? -1 : report["grid_id"].get<int>(),
                report["active_cam_id"].is_null() ? "(none)" : report["active_cam_id"].get<std::string>(),
                report["layers"].size(), report["total_entities"].get<size_t>());

            // Log the first few layers/entities to eyeball
            const size_t max_layers_log = 2;
            const size_t max_ents_log = 3;
            size_t lidx = 0;
            for (const auto& L : report["layers"]) {
                if (lidx++ >= max_layers_log) break;
                PN_CORE_INFO("  [Layer ID={0} Mask={1} entities={2}",
                    L["ID"].get<int>(), L["Mask"].get<int>(),
                    L["entity_count"].get<size_t>());
                size_t eidx = 0;
                for (const auto& E : L["Entities"]) {
                    if (eidx++ >= max_ents_log) break;
                    const std::string name = E.contains("Name") ? E["Name"].get<std::string>() : "(noname)";
                    PN_CORE_INFO("    - Ent name='{0}' comps={1}", name, E["Components"].size());
                }
            }

#ifdef _DEBUG
            // Save a report next to the scene (so can inspect with any JSON viewer)
            const std::string report_path = file_path + ".report.json";
            saveJsonFile(report_path, report);
            PN_CORE_INFO("[Scene] Wrote report: {0}", report_path);
#endif

            curr_scene_file_ = file_path;

            // Rebuild ECS world from loaded JSON
            if (auto ecs = services->get<PAIN::ECS::Controller>()) {
                // Clear all existing entities
                ecs->destroyAllEntities();

                // Walk through all Layers in the scene JSON
                for (const auto& elem : j) {
                    auto it = elem.find("Layer");
                    if (it == elem.end() || !it->is_object()) continue;

                    const auto& L = *it;

                    if (auto entsIt = L.find("Entities"); entsIt != L.end() && entsIt->is_array()) {
                        for (const auto& ewrap : *entsIt) {
                            if (!ewrap.is_object()) continue;

                            auto eit = ewrap.find("Entity");
                            if (eit == ewrap.end() || !eit->is_object()) continue;

                            const auto& E = *eit;

                            // Create new entity
                            auto e = ecs->createEntity();

                            // Rebuild components
                            if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
                                const auto& comps = *compsIt;

                                // Transform
                                if (auto jt = comps.find("Transform::Transform");
                                    jt != comps.end() && jt->is_object())
                                {
                                    const auto& obj = *jt;

                                    auto rot = obj.value("rotation", nlohmann::json::array());
                                    auto pos = obj.value("position", nlohmann::json::array());
                                    auto scl = obj.value("scale", nlohmann::json::array());

                                    if (rot.size() == 4 && pos.size() == 3 && scl.size() == 3) {
                                        PAIN::Transform::Transform t;

                                        // JSON saved as [x,y,z,w]
                                        t.rotation = glm::f32quat{
                                            rot[3].get<float>(),  // w
                                            rot[0].get<float>(),  // x
                                            rot[1].get<float>(),  // y
                                            rot[2].get<float>()   // z
                                        };

                                        t.position = { pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>() };
                                        t.scale = { scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>() };

                                        ecs->addEntityComponent<PAIN::Transform::Transform>(e, std::move(t));
                                    }
                                }

                                // TODO: hydrate other component types here
                            }
                        }
                    }
                }
            }

            return true;
        }

        std::string Service::MakeScenePathFromBase(std::string_view base)
        {
            std::string b = sanitize_base(std::string(base));
            return std::string("assets/Scenes/") + b + ".scn";
        }

        bool Service::createNewScene(std::string_view baseName)
        {
            const std::string path = MakeScenePathFromBase(baseName);
            return saveSceneToFile(path); // uses existing minimal payload
        }

        bool Service::saveCurrentScene()
        {
            if (curr_scene_file_.empty()) return false;
            // for now just touch the minimal payload, later then dump ECS/cameras wtv
            return saveSceneToFile(curr_scene_file_);
        }

        bool Service::saveSceneAs(std::string_view baseName)
        {
            const std::string path = MakeScenePathFromBase(baseName);
            if (!saveSceneToFile(path)) return false;
            curr_scene_file_ = path;
            return true;
        }

        bool Service::loadSceneById(std::string_view sceneIdWithExt)
        {
            // panel uses "xxx.scn" as an id, map to .scn.json
            std::string base(sceneIdWithExt);
            // strip ".scn" if present, then build full path
            if (base.size() >= 4 && base.substr(base.size() - 4) == ".scn") base.erase(base.size() - 4);
            const std::string path = MakeScenePathFromBase(base);
            return loadSceneFromFile(path);
        }

        bool Service::deleteSceneById(std::string_view sceneIdWithExt)
        {
            std::string base(sceneIdWithExt);
            if (base.size() >= 4 && base.substr(base.size() - 4) == ".scn") base.erase(base.size() - 4);
            const std::string path = MakeScenePathFromBase(base);
            std::error_code ec;
            std::filesystem::remove(path, ec);
            if (ec) return false;
            if (curr_scene_file_ == path) curr_scene_file_.clear();
            return true;
        }

        nlohmann::json Service::to_json_from_doc_() const
        {
            json scene = json::array();

            scene.push_back(json{ {"Grid ID", doc_.grid_id} });
            scene.push_back(json{ {"Camera",  json{ {"Active Cam ID", doc_.active_cam_id} } } });
            scene.push_back(json{ {"MetaData", json{ {"Entity_Tags", doc_.meta_tags} } } });
            scene.push_back(json{ {"Layer Count", int(doc_.layers.size())} });

            for (const auto& L : doc_.layers) {
                //json Lobj = {
                //    {"ID",      L.id},
                //    {"Mask",    L.mask},
                //    {"B_State", L.enabled},
                //    {"Entities", json::array()} // fill when wire ECS
                //};
                //scene.push_back(json{ {"Layer", std::move(Lobj)} });

                json ents = json::array();

                if (auto ecs = services->get<PAIN::ECS::Controller>()) {
                    const auto all = ecs->getAllEntities();
                    for (auto e : all) {
                        json E = json::object();

                        // Optional identifiers
                        E["ID"] = int(e);

                        // Components block
                        json C = json::object();

                        // Transform (quat + vec3)
                        if (auto t = ecs->getEntityComponent<PAIN::Transform::Transform>(e)) {
                            const auto& tr = t->get();
                            // Store rotation as [x,y,z,w], position/scale as [x,y,z]
                            C["Transform::Transform"] = {
                                {"rotation", {tr.rotation.x, tr.rotation.y, tr.rotation.z, tr.rotation.w}},
                                {"position", {tr.position.x, tr.position.y, tr.position.z}},
                                {"scale",    {tr.scale.x,    tr.scale.y,    tr.scale.z}}
                            };
                        }

                        // TODO: add other components in the same style

                        E["Components"] = std::move(C);
                        ents.push_back(json{ {"Entity", std::move(E)} });
                    }
                }

                json Lobj = {
                    {"ID",      L.id},
                    {"Mask",    L.mask},
                    {"B_State", L.enabled},
                    {"Entities", std::move(ents)}     // <-- now filled from ECS
                };
                scene.push_back(json{ {"Layer", std::move(Lobj)} });

            }

            // mask grid, can remove if dw
            if (!doc_.mask_matrix.empty()) {
                json m = json::array();
                for (auto& row : doc_.mask_matrix) m.push_back(row);
                scene.push_back(json{ {"LayerMaskMatrix", m} });
            }

            return scene;
        }

        void Service::doc_from_json_(const nlohmann::json& j)
        {
            doc_ = {}; // reset
            doc_.grid_id = 0;
            doc_.active_cam_id.clear();
            doc_.meta_tags.clear();
            doc_.layers.clear();
            doc_.mask_matrix.clear();

            auto findSection = [&](const char* key)->const json* {
                for (const auto& elem : j) {
                    if (!elem.is_object()) continue;
                    auto it = elem.find(key);
                    if (it != elem.end()) return &(*it);
                }
                return nullptr;
                };

            if (auto g = findSection("Grid ID"); g && g->is_number_integer())
                doc_.grid_id = g->get<int>();

            if (auto c = findSection("Camera"); c && c->is_object()) {
                auto it = c->find("Active Cam ID");
                if (it != c->end() && it->is_string()) doc_.active_cam_id = it->get<std::string>();
            }
            if (auto m = findSection("MetaData"); m && m->is_object()) {
                auto it = m->find("Entity_Tags");
                if (it != m->end() && it->is_array()) doc_.meta_tags = it->get<std::vector<std::string>>();
            }

            // layers
            for (const auto& elem : j) {
                auto it = elem.find("Layer");
                if (it == elem.end() || !it->is_object()) continue;
                const auto& L = *it;
                SceneDoc::Layer out;
                out.id = L.value("ID", 0);
                out.mask = L.value("Mask", 1);
                out.enabled = L.value("B_State", true);
                doc_.layers.push_back(std::move(out));
            }

            // mask also, can remove
            if (auto mm = findSection("LayerMaskMatrix"); mm && mm->is_array()) {
                doc_.mask_matrix.clear();
                for (const auto& row : *mm) {
                    doc_.mask_matrix.push_back(row.get<std::vector<bool>>());
                }
            }

            doc_.dirty = false;
        }

        void Service::setGrid(int g)
        {
            doc_.grid_id = g;
        }

        void Service::setActiveCam(std::string id)
        {
            doc_.active_cam_id = id;
        }

        void Service::setTags(std::vector<std::string> t)
        {
            doc_.meta_tags = t;
        }

        void Service::addLayer()
        {
            int next_id = doc_.layers.empty() ? 0 : (doc_.layers.back().id + 1);
            doc_.layers.push_back(SceneDoc::Layer{ next_id, 
                                                   1, 
                                                   true
                                                    });
        }

        void Service::removeLayer(unsigned idx)
        {
            if (idx >= doc_.layers.size() || doc_.layers.size() == 1) return;
            doc_.layers.erase(doc_.layers.begin() + idx);
            for (unsigned i = 0;i < doc_.layers.size();++i) doc_.layers[i].id = int(i);
        }

        void Service::setLayerVisible(unsigned idx, bool v)
        {
            if (idx >= doc_.layers.size()) return;
            doc_.layers[idx].enabled = v;
            doc_.dirty = true;
        }

        void Service::setLayerYSort(unsigned idx, bool v)
        {
            if (idx >= doc_.layers.size()) return;
            doc_.dirty = true;
        }

        void Service::setMask(unsigned i, unsigned j, bool v)
        {
            ensureMaskSize();
            const size_t n = doc_.layers.size();
            if (i >= n || j >= n || i == j) return;

            doc_.mask_matrix[i][j] = v;
            doc_.mask_matrix[j][i] = v;   
            doc_.dirty = true;
            isModifiedScene = true;
        }
        void Service::ensureMaskSize()
        {
            const size_t n = doc_.layers.size();

            if (doc_.mask_matrix.size() < n)
                doc_.mask_matrix.resize(n);

            for (size_t i = 0; i < n; ++i) {
                if (doc_.mask_matrix[i].size() < n)
                    doc_.mask_matrix[i].resize(n, false);
                // force diagonal off
                if (i < doc_.mask_matrix[i].size())
                    doc_.mask_matrix[i][i] = false;
            }

            if (doc_.mask_matrix.size() > n)
                doc_.mask_matrix.resize(n);
            for (size_t i = 0; i < n; ++i) {
                if (doc_.mask_matrix[i].size() > n)
                    doc_.mask_matrix[i].resize(n);
                if (i < doc_.mask_matrix[i].size())
                    doc_.mask_matrix[i][i] = false;
            }
        }
    }
}