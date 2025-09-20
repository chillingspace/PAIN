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

namespace fs = std::filesystem;

namespace PAIN {
	namespace Serialization {
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