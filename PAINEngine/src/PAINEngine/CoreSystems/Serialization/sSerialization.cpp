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


 // Fail at compile time if reflection didn't bind
static_assert(refl::trait::is_reflectable_v<PAIN::Serialization::SceneDoc>,
    "SceneDoc not reflectable");
static_assert(refl::trait::is_reflectable_v<PAIN::Serialization::SceneDoc::Layer>,
    "Layer not reflectable");

namespace fs = std::filesystem;

namespace PAIN {
    namespace Serialization {

        // This function will remove commons symbols form the input string
         inline std::string sanitize_base(std::string s) {
        s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
            return !(std::isalnum(c) || c == '_' || c == '-' || c == '.');
            }), s.end());
        return s;
    }

        //inline std::string sanitize_base(std::string s) {
        //    s.erase(std::remove_if(s.begin(), s.end(), [](unsigned char c) {
        //        return !(std::isalnum(c) || c == '_' || c == '-');
        //        }), s.end());
        //    return s;
        //}

        void PAIN::Serialization::Service::onAttach() {
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
            nlohmann::json scene = to_json_from_doc_(); 
            std::string path = file_path;
            if (path.size() < 4 || path.rfind(".scn") != path.size() - 4) path += ".scn";
            const bool ok = saveJsonFile(path, scene);
            if (ok) { curr_scene_file_ = file_path; isModifiedScene = false; }
            return ok;
        }
        bool Service::loadSceneFromFile(const std::string& file_path) {
            const auto j = loadJsonFile(file_path);
            if (!j.is_object()) {
                PN_CORE_WARN("[Scene] expected object root (reflection)");
                return false;
            }

            // Reflect into doc_
            // set doc_ + clear dirty
            doc_from_json_(j);  

            // Rebuild ECS from the new bolt on section if present
            if (auto ecsIt = j.find("ecs"); ecsIt != j.end() && ecsIt->is_object()) {
                if (auto controller = services->get<PAIN::ECS::Controller>()) {
                    controller->destroyAllEntities();

                    if (auto entsIt = ecsIt->find("Entities"); entsIt != ecsIt->end() && entsIt->is_array()) {
                        for (const auto& ewrap : *entsIt) {
                            if (!ewrap.is_object()) continue;
                            auto eit = ewrap.find("Entity");
                            if (eit == ewrap.end() || !eit->is_object()) continue;

                            const auto& E = *eit;
                            auto e = controller->createEntity();

                            // Name
                            if (auto n = E.find("Name"); n != E.end() && n->is_string())
                                controller->addEntityComponent(e, MetaData::EntityName{ n->get<std::string>() });
                            else
                                controller->addEntityComponent(e, MetaData::EntityName{ "Entity " + std::to_string((int)e) });

                            // Components
                            if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
                                const auto& comps = *compsIt;

                                // Transform::Transform
                                if (auto jt = comps.find("Transform::Transform"); jt != comps.end() && jt->is_object()) {
                                    const auto& obj = *jt;
                                    auto rot = obj.value("rotation", nlohmann::json::array());
                                    auto pos = obj.value("position", nlohmann::json::array());
                                    auto scl = obj.value("scale", nlohmann::json::array());

                                    if (rot.size() == 4 && pos.size() == 3 && scl.size() == 3) {
                                        Transform t;
                                        t.rotation = glm::f32quat{ rot[3].get<float>(), rot[0].get<float>(), rot[1].get<float>(), rot[2].get<float>() };
                                        t.position = { pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>() };
                                        t.scale = { scl[0].get<float>(), scl[1].get<float>(), scl[2].get<float>() };
                                        controller->addEntityComponent<PAIN::Transform>(e, std::move(t));
                                    }
                                }

                                // TODO: add other components here (Audio, Renderers,.)
                            }
                        }
                    }
                }
            }

            // Remember which file is loaded for saving
            curr_scene_file_ = file_path;

            PN_CORE_INFO("[Serialization] loadSceneFromFile OK, marking scene changed");
            markSceneChanged();
            return true;
        }

        std::string Service::MakeScenePathFromBase(std::string_view base)
        {
            std::string b = sanitize_base(std::string(base));
            return std::string("assets/Raw/Game/Scenes/") + b;
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
            // !TODO: ADD CHECK FOR WHEN TRYING TO LOAD LEVEL THAT DOESN'T EXIST

            PN_CORE_INFO("[Serialization] loadSceneById called with '{}'", sceneIdWithExt);
            const std::string path = MakeScenePathFromBase(sceneIdWithExt); // normalizes to .scn
            PN_CORE_INFO("[Serialization] Trying scene path: {}", path);
            return loadSceneFromFile(path);
        }



        //bool Service::deleteSceneById(std::string_view sceneIdWithExt)
        //{
        //    std::string base(sceneIdWithExt);
        //    if (base.size() >= 4 && base.substr(base.size() - 4) == ".scn") base.erase(base.size() - 4);
        //    const std::string path = MakeScenePathFromBase(base);
        //    std::error_code ec;
        //    std::filesystem::remove(path, ec);
        //    if (ec) return false;
        //    if (curr_scene_file_ == path) curr_scene_file_.clear();
        //    return true;
        //}

        bool Service::deleteSceneById(std::string_view sceneId)
        {
            // Normalize to the actual on-disk path (adds .scn if needed)
            const std::string path = MakeScenePathFromBase(sceneId);
            PN_CORE_INFO("[Serialization] deleteSceneById trying '{}'", path);

#if !defined(PN_PLATFORM_ANDROID)
            // Debug for desktop
            if (!std::filesystem::exists(path)) {
                PN_CORE_WARN("[Serialization] deleteSceneById: file does not exist: {}", path);
                return false;
            }
#else
            // On Android, you cannot delete packaged assets from the APK.
            // Only delete files in your app's writable directory (e.g., Path::Service user dir).
            // If 'path' points inside assets, bail:
            // if (isInApkAssets(path)) { PN_CORE_WARN(...); return false; }
#endif

            std::error_code ec;
            const bool removed = std::filesystem::remove(path, ec);

            if (ec) {
                PN_CORE_ERROR("[Serialization] deleteSceneById: std::filesystem::remove failed for {}: {}", path, ec.message());
                return false;
            }
            if (!removed) {
                // No error, but nothing was removed (likely didn't exist)
                PN_CORE_WARN("[Serialization] deleteSceneById: nothing removed for {}", path);
                return false;
            }

            if (curr_scene_file_ == path) curr_scene_file_.clear();

            PN_CORE_INFO("[Serialization] Deleted scene: {}", path);
            return true;
        }

        nlohmann::json Service::to_json_from_doc_() const
        {
            // Reflection Check
            /*
#ifdef _DEBUG
            {
                using T = PAIN::Serialization::SceneDoc;
                constexpr auto type = refl::reflect<T>();
                size_t field_count = 0;

                refl::util::for_each(type.members, [&](auto member) {
                    if constexpr (refl::trait::is_field_v<decltype(member)>) {
                        auto key = PAIN::Serialization::detail::name_to_string(member.name);
                        PN_CORE_INFO("[Reflection] SceneDoc field: {}", key);
                        ++field_count;
                    }
                    });

                PN_CORE_INFO("[Reflection] SceneDoc total fields: {}", field_count);
            }
#endif
*/

            // reflection SceneDoc object
            nlohmann::json root = to_json_reflected(doc_);

            // Attach ECS dump
            nlohmann::json ecs = nlohmann::json::object();
            nlohmann::json ents = nlohmann::json::array();

            if (auto controller = services->get<PAIN::ECS::Controller>()) {
                // Use EnTT view to iterate all entities with EntityName component
                auto& registry = controller->getRegistry();
                auto view = registry.view<MetaData::EntityName>();
                for (auto entity : view) {

                    // Cast to own entity type 
                    ECS::Entity::Type e = static_cast<ECS::Entity::Type>(entity);

                    nlohmann::json E = nlohmann::json::object();

                    // Name
                    if (auto name = controller->getEntityComponent<MetaData::EntityName>(e);
                        name.has_value()) {
                        E["Name"] = name->get().name;
                    }
                    else {
                        E["Name"] = "Entity " + std::to_string((int)e);
                    }

                    // ID
                    E["ID"] = int(e);

                    // Components
                    nlohmann::json C = nlohmann::json::object();

                    // Transform (quat + vec3)
                    if (auto t = controller->getEntityComponent<Transform>(e)) {
                        const auto& tr = t->get();
                        C["Transform::Transform"] = {
                            {"rotation", {tr.rotation.x, tr.rotation.y, tr.rotation.z, tr.rotation.w}},
                            {"position", {tr.position.x, tr.position.y, tr.position.z}},
                            {"scale",    {tr.scale.x,    tr.scale.y,    tr.scale.z}}
                        };
                    }

                    // TODO: add other components

                    E["Components"] = std::move(C);

                    ents.push_back(nlohmann::json{ {"Entity", std::move(E)} });
                }
            }

            ecs["Entities"] = std::move(ents);
            root["ecs"] = std::move(ecs);            // bolt on section

            return root;
        }


        void Service::doc_from_json_(const nlohmann::json& j)
        {
            /*
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
            */

            doc_ = {};
            from_json_reflected(doc_, j);
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

        void Service::markSceneChanged() {
            scene_changed_.store(true, std::memory_order_relaxed);
        }

        bool Service::consumeSceneChanged() {
            // atomically read+clear
            return scene_changed_.exchange(false, std::memory_order_relaxed);
        }

    }
}
