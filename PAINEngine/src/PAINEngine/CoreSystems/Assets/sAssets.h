#pragma once

#include "sLoader.h"
#include "Applications/AppSystem.h"

namespace PAIN {
    namespace Assets {

        struct MetaData {
            Loader::Types type;
            std::filesystem::path primary_path;

            MetaData() : type(Loader::Types::None), primary_path("") {}
            MetaData(Loader::Types t, const std::filesystem::path& path)
                : type(t), primary_path(path) {
            }
        };

        class Service : public AppSystem {
        public:
            Service() = default;
            ~Service() = default;

            void init();

            std::string registerAsset(const std::string& path, bool b_virtual);
            void unregisterAsset(const std::string& asset_id);

            void cacheAsset(const std::string& asset_id);
            void uncacheAsset(const std::string& asset_id);
            void recacheAsset(const std::string& asset_id);

            std::filesystem::path getAssetPath(const std::string& asset_id) const;
            Loader::Types getAssetType(const std::string& asset_id) const;
            Loader::Types getAssetType(const std::filesystem::path& path) const;

            bool isAssetCached(const std::string& asset_id) const;
            bool isAssetRegistered(const std::string& asset_id) const;

            void scanAssetDirectory(const std::string& virtual_path, bool recursive);
            void cacheAssetDirectory(const std::string& virtual_path, bool recursive);
            void uncacheAssetDirectory(const std::string& virtual_path, bool recursive);

            void clearCache();
            void logAssetsRegistry() const;

            nlohmann::json serialize() const;
            void deserialize(const nlohmann::json& data);

            std::string getIDFromPath(const std::string& path, bool b_virtual) const;

            bool hasAssets() const;

        private:
            std::map<std::string, MetaData> asset_registry;
            std::map<std::string, std::shared_ptr<void>> asset_cache;

            std::set<std::string> valid_extensions;
            std::set<std::string> invalid_keys;

            bool isPathValid(const std::string& path, bool b_virtual) const;
        };

    } // namespace Assets
} // namespace PAIN
