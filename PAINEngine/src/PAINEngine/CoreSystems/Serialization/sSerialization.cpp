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
#include "ECS/sMetaData.h"
#include "CoreSystems/Path/Path.h"
#include "CoreSystems/Path/Android/AndroidPath.h"
#include "ECS/Controller.h"


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

            // Handled in Tools panel
            //if (!curr_scene_file_.empty()) {
            //    if (isModifiedScene) {
            //        PN_CORE_INFO("[Serialization] Autosave on shutdown: {}", curr_scene_file_);
            //        if (!saveCurrentScene()) {
            //            PN_CORE_WARN("[Serialization] Autosave FAILED: {}", curr_scene_file_);
            //        }
            //    }
            //}
        }

        // ----------------------------
        // File helpers
        // ----------------------------
        bool Service::saveJsonFile(const std::string& file_path, const nlohmann::json& data) {
            try {
                auto path_service = services->get<Path::Path>();
                
                auto stream = path_service->createFileStream(file_path, Path::FileMode::Write);

                if (!stream || !stream->good()) {
                    PN_CORE_ERROR("Could not open file for writing JSON: {}", file_path);
                    return false;
                }

                // Output as a std::string with pretty formatting
                std::string contents = data.dump(4);
                stream->write(contents.data(), contents.size());
                // Good practice for custom streams
                stream->flush(); 

                PN_CORE_INFO("Save file: {}", file_path);
                return true;
            }
            catch (const std::exception& e) {
                PN_CORE_ERROR("Exception when saving JSON: {} | Reason: {}", file_path, e.what());
                return false;
            }
        }


        nlohmann::json Service::loadJsonFile(const std::string& file_path) {
            nlohmann::json j;

            auto path_service = services->get<Path::Path>();

            // Crash is happening in create file stream
            auto stream = path_service->createFileStream(file_path, Path::FileMode::Read);

            // Check if unique_ptr is null
            if (!stream) {
                PN_CORE_ERROR("Failed to create file stream: {}", file_path);
                return j;
            }

            if (!stream->good()) {
                PN_CORE_ERROR("Could not open scene file for JSON: {}", file_path);
                return j;
            }

            // Check if file is empty
            if (stream->size() == 0) {
                PN_CORE_ERROR("JSON file is empty: {}", file_path);
                return j;
            }

            std::string contents;
            contents.resize(stream->size());
            stream->read(contents.data(), contents.size());

            // Validate JSON syntax before parsing
            if (!nlohmann::json::accept(contents)) {
                PN_CORE_ERROR("Invalid JSON syntax: {}", file_path);
                return j;
            }

            // Validate JSON syntax before parsing
            if (!nlohmann::json::accept(contents)) {
                PN_CORE_ERROR("Invalid JSON syntax: {}", file_path);
                return j;
            }

            try {
                j = nlohmann::json::parse(contents);

                if (!j.is_object()) {
                    PN_CORE_WARN("JSON root is not an object: {}", file_path);
                }
            }
            catch (const nlohmann::json::exception& e) {
                PN_CORE_ERROR("JSON parse exception: {} - File: {}", e.what(), file_path);
            }
            catch (...) {
                PN_CORE_ERROR("Unknown error parsing JSON: {}", file_path);
            }


            PN_CORE_INFO("Successfully Load file {}", file_path);
            return j;
        }

        // ----------------------------
        // Scene-level placeholders
        // ----------------------------

        bool Service::saveSceneToFile(const std::string& file_path) {
            nlohmann::json scene = to_json_from_doc_(); 

            // For metadata seri
            if (auto metadata_service = services->get<PAIN::MetaData::Service>()) {
                scene["metadata_service"] = metadata_service->serializeServiceState();
            }

            std::string path = file_path;
            if (path.size() < 4 || path.rfind(".scn") != path.size() - 4) path += ".scn";
            const bool ok = saveJsonFile(path, scene);
            if (ok) { curr_scene_file_ = file_path; isModifiedScene = false; }
            return ok;
        }

        bool Service::loadSceneFromFile(const std::string& file_path) {
            auto path_service = services->get<Path::Path>();
            std::string virtPath = file_path;

            if (virtPath.size() < 4 || virtPath.rfind(".scn") != virtPath.size() - 4)
                virtPath += ".scn";

#ifdef PN_PLATFORM_WINDOWS
            // avoid the assert in WindowsPath::createFileStream
            const std::string realPath = path_service->resolvePath(virtPath);
            if (!std::filesystem::exists(realPath)) {
                PN_CORE_WARN("[Scene] File not found: {}", realPath);
                return false;
            }
#endif
            const auto j = loadJsonFile(file_path);
            if (!j.is_object()) {
                PN_CORE_WARN("[Scene] expected object root (reflection)");
                return false;
            }

            // Reflect into doc_
            // set doc_ + clear dirty
            doc_from_json_(j);

            if (auto metadata_service = services->get<PAIN::MetaData::Service>()) {
                if (j.contains("metadata_service")) {
                    metadata_service->deserializeServiceState(j["metadata_service"]);
                }
            }

            // Rebuild ECS from the new bolt on section if present
            if (auto ecsIt = j.find("ecs"); ecsIt != j.end() && ecsIt->is_object()) {
                if (auto controller = services->get<PAIN::ECS::Controller>()) {
                    controller->destroyAllEntities();

                    if (auto entsIt = ecsIt->find("Entities"); entsIt != ecsIt->end() && entsIt->is_array()) {
                        // PASS 1: Create all entities with their original GUIDs
                        // This ensures all entities exist in the GUID registry before we deserialize Hierarchy
                        std::vector<std::pair<entt::entity, const nlohmann::json*>> entity_data_pairs;

                        for (const auto& ewrap : *entsIt) {
                            if (!ewrap.is_object()) continue;
                            auto eit = ewrap.find("Entity");
                            if (eit == ewrap.end() || !eit->is_object()) continue;

                            const auto& E = *eit;

                            // Extract the GUID from Components
                            Assets::GUID entity_guid;
                            bool has_guid = false;

                            if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
                                if (auto guidIt = compsIt->find("GUID"); guidIt != compsIt->end() && guidIt->is_object()) {
                                    try {
                                        // Deserialize the GUID using reflection
                                        Entity::GUID guid_comp;
                                        PAIN::Serialization::from_json_reflected(guid_comp, *guidIt);
                                        entity_guid = guid_comp.guid;
                                        has_guid = true;
                                    }
                                    catch (const std::exception& ex) {
                                        PN_CORE_ERROR("[Scene Load] Failed to deserialize GUID: {}", ex.what());
                                    }
                                }
                            }

                            // Create entity with the original GUID if available
                            entt::entity e;
                            if (has_guid && entity_guid.IsValid()) {
                                e = controller->createEntity(entity_guid);
                                PN_CORE_INFO("[Scene Load] Created entity {} with preserved GUID {}",
                                    static_cast<uint32_t>(e), entity_guid.ToString());
                            }
                            else {
                                e = controller->createEntity();
                                PN_CORE_WARN("[Scene Load] Created entity {} without GUID, generated new one",
                                    static_cast<uint32_t>(e));
                            }

                            // Store for second pass
                            entity_data_pairs.emplace_back(e, &E);
                        }

                        // PASS 2: Deserialize all components now that all entities exist in GUID registry
                        for (const auto& [e, E_ptr] : entity_data_pairs) {
                            const auto& E = *E_ptr;

                            // Name
                            if (auto n = E.find("Name"); n != E.end() && n->is_string())
                                controller->addEntityComponent(e, Entity::Name{ n->get<std::string>() });
                            else
                                controller->addEntityComponent(e, Entity::Name{ "Entity " + std::to_string((int)e) });

                            // Deserialize all components (including Hierarchy, which now can resolve GUIDs)
                            if (auto compsIt = E.find("Components"); compsIt != E.end() && compsIt->is_object()) {
                                controller->loadAllComponentsFromJson(e, *compsIt);
                            }
                        }

                        PN_CORE_INFO("[Scene Load] Successfully loaded {} entities with hierarchy", entity_data_pairs.size());
                    }
                }
            }

            // Remember which file is loaded for saving
            curr_scene_file_ = file_path;

            PN_CORE_INFO("[Serialization] loadSceneFromFile OK, marking scene changed");
            markSceneChanged();
            return true;
        }

        std::string Service::getCurrSceneId() const
        {
            // Find the last '/' in the path to get the base filename
            size_t lastSlash = curr_scene_file_.find_last_of('/');
            std::string baseName = (lastSlash != std::string::npos) ? curr_scene_file_.substr(lastSlash + 1) : curr_scene_file_;

            // Remove ".scn" extension if present
            size_t scnPos = baseName.rfind(".scn");
            if (scnPos != std::string::npos && scnPos == baseName.size() - 4) {
                baseName.erase(scnPos, 4);
            }

            // Additional sanitize if needed
            return baseName;
        }

        std::string Service::getSceneId(std::string file_path) const
        {
            size_t lastSlash = file_path.find_last_of("/\\");
            std::string baseName = (lastSlash != std::string::npos) ? file_path.substr(lastSlash + 1) : file_path;

            // Remove file extension ".scn" if present
            size_t extPos = baseName.rfind(".scn");
            if (extPos != std::string::npos && extPos == baseName.size() - 4) {
                baseName.erase(extPos, 4);
            }

            return baseName + ".scn";
        }

#ifdef PN_PLATFORM_WINDOWS
        std::string Service::OpenSceneFileDialog(HWND ownerWindow)
        {
            OPENFILENAME ofn;       // common dialog box structure
            char szFile[260] = { 0 }; // buffer for file name

            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = ownerWindow;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);

            auto path_service = services->get<Path::Path>();

            std::string initialDirStr = path_service->resolvePath("main_game_assets://scenes");
            //std::string initialDirStr = path_service->resolvePath("game_assets://scenes");
            ofn.lpstrInitialDir = initialDirStr.c_str();
            ofn.lpstrFilter = "Scene Files (*.scn)\0*.scn\0All Files (*.*)\0*.*\0";
            ofn.nFilterIndex = 1;

            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetOpenFileName(&ofn)) {
                return std::string(ofn.lpstrFile);
            }
            return {};
        }
#endif

        std::string Service::makeVirtualScenePathFromBase(std::string_view base)
        {
            std::string b = sanitize_base(std::string(base));

            //auto path_service = services->get<Path::Path>();

#ifdef PN_PLATFORM_WINDOWS
            // Windows file path
            return "main_game_assets://scenes/" + b;
#elif PN_PLATFORM_ANDROID
            return "game_assets://scenes/" + b;
#endif
        }

        bool Service::createNewScene(std::string_view baseName)
        {
            const std::string path = makeVirtualScenePathFromBase(baseName);
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
            const std::string path = makeVirtualScenePathFromBase(baseName);
            if (!saveSceneToFile(path)) return false;
            curr_scene_file_ = path;
            return true;
        }

        bool Service::loadSceneById(std::string_view sceneIdWithExt)
        {
            PN_CORE_INFO("[Serialization] loadSceneById called with '{}'", sceneIdWithExt);
            const std::string path = makeVirtualScenePathFromBase(sceneIdWithExt); 
            PN_CORE_INFO("[Serialization] Trying scene path: {}", path);
            return loadSceneFromFile(path);
        }



        //bool Service::deleteSceneById(std::string_view sceneIdWithExt)
        //{
        //    std::string base(sceneIdWithExt);
        //    if (base.size() >= 4 && base.substr(base.size() - 4) == ".scn") base.erase(base.size() - 4);
        //    const std::string path = makeVirtualScenePathFromBase(base);
        //    std::error_code ec;
        //    std::filesystem::remove(path, ec);
        //    if (ec) return false;
        //    if (curr_scene_file_ == path) curr_scene_file_.clear();
        //    return true;
        //}

        bool Service::deleteSceneById(std::string_view sceneId)
        {
            // Normalize to the actual on-disk path (adds .scn if needed)
            const std::string vir_path = makeVirtualScenePathFromBase(sceneId);
            const std::string full_path = services->get<Path::Path>()->resolvePath(vir_path);
            PN_CORE_INFO("[Serialization] deleteSceneById trying '{}'", full_path);

#if !defined(PN_PLATFORM_ANDROID)
            // Debug for desktop
            if (!std::filesystem::exists(full_path)) {
                PN_CORE_WARN("[Serialization] deleteSceneById: file does not exist: {}", full_path);
                return false;
            }
#else
            // On Android, you cannot delete packaged assets from the APK.
            // Only delete files in your app's writable directory (e.g., Path::Service user dir).
            // If 'path' points inside assets, bail:
            // if (isInApkAssets(path)) { PN_CORE_WARN(...); return false; }
#endif

            std::error_code ec;
            const bool removed = std::filesystem::remove(full_path, ec);

            if (ec) {
                PN_CORE_ERROR("[Serialization] deleteSceneById: std::filesystem::remove failed for {}: {}", full_path, ec.message());
                return false;
            }
            if (!removed) {
                // No error, but nothing was removed (likely didn't exist)
                PN_CORE_WARN("[Serialization] deleteSceneById: nothing removed for {}", full_path);
                return false;
            }

            if (curr_scene_file_ == full_path) curr_scene_file_.clear();

            PN_CORE_INFO("[Serialization] Deleted scene: {}", full_path);
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
                auto view = registry.view<Entity::Name>();
                for (auto e : view) {

                    nlohmann::json E = nlohmann::json::object();

                    // Name
                    if (auto name = controller->getEntityComponent<Entity::Name>(e);
                        name.has_value()) {
                        E["Name"] = name->get().name;
                    }
                    else {
                        E["Name"] = "Entity " + std::to_string((int)e);
                    }

                    // Components
                    E["Components"] = controller->getAllComponentsAsJson(e);

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
