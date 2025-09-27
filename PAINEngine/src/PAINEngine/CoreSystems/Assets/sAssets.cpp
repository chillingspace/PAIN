#include "pch.h"
#include "Applications/Application.h"
#include "sAssets.h"
#include "sPath.h"

#define PN_PATH_SERVICE  services->get<Path::Service>()
#define PN_LOADER_SERVICE  services->get<Loader::Service>()

namespace PAIN {
    namespace Assets {

        void Service::init() {
            // Register valid extensions (can be extended later)
            valid_extensions = { ".png", ".jpg", ".jpeg", ".tex", ".ttf",
                                 ".model", ".wav", ".mpg", ".prefab",
                                 ".scn", ".grid", ".lua", ".json" };

            // Add invalid keys
            invalid_keys.insert("batched_");

            // Register known extensions with Loader
            auto loader = PN_LOADER_SERVICE;
            loader->addValidExtension(".png", Loader::Types::Texture);
            loader->addValidExtension(".jpg", Loader::Types::Texture);
            loader->addValidExtension(".jpeg", Loader::Types::Texture);
            loader->addValidExtension(".tex", Loader::Types::Texture);
            loader->addValidExtension(".ttf", Loader::Types::Font);
            loader->addValidExtension(".model", Loader::Types::Model);
            loader->addValidExtension(".wav", Loader::Types::Sound);  // music/sound split later
            loader->addValidExtension(".scn", Loader::Types::Scene);
            loader->addValidExtension(".prefab", Loader::Types::Prefab);
            loader->addValidExtension(".grid", Loader::Types::Grid);
            loader->addValidExtension(".lua", Loader::Types::Script);
            loader->addValidExtension(".mpg", Loader::Types::Video);
        }

        std::string Service::registerAsset(const std::string& path, bool b_virtual) {
            if (!isPathValid(path, b_virtual)) {
                return "";
            }
            auto asset_id = getIDFromPath(path, b_virtual);
            auto asset_type = getAssetType(std::filesystem::path(path));
            asset_registry[asset_id] = MetaData(asset_type, path);
            return asset_id;
        }

        void Service::unregisterAsset(const std::string& asset_id) {
            asset_registry.erase(asset_id);
            asset_cache.erase(asset_id);
        }

        void Service::cacheAsset(const std::string& asset_id) {
            if (isAssetCached(asset_id)) return;
            auto it = asset_registry.find(asset_id);
            if (it == asset_registry.end()) return;

            auto loader = PN_LOADER_SERVICE;
            auto loaded = loader->load(it->second.type, it->second.primary_path);
            asset_cache[asset_id] = loaded;
        }

        void Service::uncacheAsset(const std::string& asset_id) {
            asset_cache.erase(asset_id);
        }

        void Service::recacheAsset(const std::string& asset_id) {
            uncacheAsset(asset_id);
            cacheAsset(asset_id);
        }

        std::filesystem::path Service::getAssetPath(const std::string& asset_id) const {
            return asset_registry.at(asset_id).primary_path;
        }

        Loader::Types Service::getAssetType(const std::string& asset_id) const {
            if (asset_registry.find(asset_id) == asset_registry.end()) return Loader::Types::None;
            return asset_registry.at(asset_id).type;
        }

        Loader::Types Service::getAssetType(const std::filesystem::path& path) const {
            return PN_LOADER_SERVICE->getTypeFromExtension(path);
        }

        bool Service::isAssetCached(const std::string& asset_id) const {
            return asset_cache.find(asset_id) != asset_cache.end();
        }

        bool Service::isAssetRegistered(const std::string& asset_id) const {
            return asset_registry.find(asset_id) != asset_registry.end();
        }

        void Service::scanAssetDirectory(const std::string& virtual_path, bool recursive) {
            auto root = PN_PATH_SERVICE->resolvePath(virtual_path);
            if (!std::filesystem::exists(root)) return;

            if (!recursive) {
                for (auto& file : std::filesystem::directory_iterator(root)) {
                    if (!file.is_regular_file()) continue;
                    registerAsset(file.path().string(), false);
                }
            }
            else {
                for (auto& file : std::filesystem::recursive_directory_iterator(root)) {
                    if (!file.is_regular_file()) continue;
                    registerAsset(file.path().string(), false);
                }
            }
        }

        void Service::cacheAssetDirectory(const std::string& virtual_path, bool recursive) {
            auto root = PN_PATH_SERVICE->resolvePath(virtual_path);
            if (!std::filesystem::exists(root)) return;

            if (!recursive) {
                for (auto& file : std::filesystem::directory_iterator(root)) {
                    if (!file.is_regular_file()) continue;
                    cacheAsset(getIDFromPath(file.path().string(), false));
                }
            }
            else {
                for (auto& file : std::filesystem::recursive_directory_iterator(root)) {
                    if (!file.is_regular_file()) continue;
                    cacheAsset(getIDFromPath(file.path().string(), false));
                }
            }
        }

        void Service::uncacheAssetDirectory(const std::string& virtual_path, bool recursive) {
            auto root = PN_PATH_SERVICE->resolvePath(virtual_path);
            if (!std::filesystem::exists(root)) return;

            if (!recursive) {
                for (auto& file : std::filesystem::directory_iterator(root)) {
                    if (!file.is_regular_file()) continue;
                    uncacheAsset(getIDFromPath(file.path().string(), false));
                }
            }
            else {
                for (auto& file : std::filesystem::recursive_directory_iterator(root)) {
                    if (!file.is_regular_file()) continue;
                    uncacheAsset(getIDFromPath(file.path().string(), false));
                }
            }
        }

        void Service::clearCache() {
            asset_cache.clear();
        }

        void Service::logAssetsRegistry() const {
            for (auto& [id, meta] : asset_registry) {
                PN_CORE_INFO("Asset ID: {} Path: {}", id, meta.primary_path.string());
            }
        }

        nlohmann::json Service::serialize() const {
            nlohmann::json j;
            for (auto& [id, meta] : asset_registry) {
                j[id] = {
                    {"Type", static_cast<int>(meta.type)},
                    {"Path", meta.primary_path.string()}
                };
            }
            return j;
        }

        void Service::deserialize(const nlohmann::json& data) {
            for (auto& [id, meta] : data.items()) {
                asset_registry[id] =
                    MetaData(static_cast<Loader::Types>(meta["Type"].get<int>()),
                        meta["Path"].get<std::string>());
            }
        }

        std::string Service::getIDFromPath(const std::string& path, bool b_virtual) const {
            auto actual_path = b_virtual ?
                PN_PATH_SERVICE->normalizePath(PN_PATH_SERVICE->resolvePath(path)).string()
                : PN_PATH_SERVICE->normalizePath(path).string();

            size_t start = actual_path.find_last_of("\\/") + 1;
            return actual_path.substr(start);
        }

        bool Service::isPathValid(const std::string& path, bool b_virtual) const {
            auto ext = b_virtual ?
                PN_PATH_SERVICE->resolvePath(path).extension().string()
                : std::filesystem::path(path).extension().string();

            return valid_extensions.find(ext) != valid_extensions.end();
        }

        bool Service::hasAssets() const {
            return 0;
        }


    } // namespace Assets
} // namespace PAIN
