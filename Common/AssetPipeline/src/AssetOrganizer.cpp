#include "AssetOrganizer.h"

namespace PAIN {
    namespace Assets {

        bool AssetOrganizer::isPathPartOfRoot(std::filesystem::path const& path, std::filesystem::path const& root) const {

            // Convert both paths to absolute to avoid issues with relative paths
            std::filesystem::path absFile = std::filesystem::absolute(path);
            std::filesystem::path absRoot = std::filesystem::absolute(root);

            std::filesystem::path currentPath = absFile;

            //Recursively search for the root
            while (currentPath != currentPath.parent_path()) {
                if (currentPath == absRoot) {
                    return true; // Found the root!
                }
                currentPath = currentPath.parent_path();
            }

            // Check the final case (filesystem root)
            return currentPath == absRoot;
        }

        bool AssetOrganizer::repositionFile(std::filesystem::path const& file_path, std::filesystem::path const& target_path) const {
            try {
                std::filesystem::rename(file_path, target_path);
                std::cout << "File Moved From: " << file_path << " To: " << target_path << std::endl;
                return true;
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cout << file_path << "Reposition Failed." << std::endl;
                return false;
            }
        }

        bool AssetOrganizer::deleteFile(std::filesystem::path const& file_path) const {
            try {
                if (std::filesystem::remove(file_path)) {
                    std::cout << file_path << " - Deleted." << std::endl;
                    return true;
                }
                else {
                    std::cout << file_path << " - Deletion Failed." << std::endl;
                    return false;
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cout << file_path << " - Deletion Failed." << std::endl;
                return false;
            }
        }

        void AssetOrganizer::instantiateFolder(std::filesystem::path const& path) const {
            if (!std::filesystem::exists(path)) {
                try {
                    std::filesystem::create_directories(path);
                }
                catch (const std::exception& e) {
                    std::cout << "Failed to create new directory. Directory missing!" << std::endl;
                }
            }
            else {
                try {
                    std::filesystem::path temp = path.parent_path() / (path.filename().string() + "_TEMP_");
                    std::filesystem::rename(path, temp);
                    std::filesystem::rename(temp, path);
                }
                catch (const std::exception& e) {
                    std::cout << "Folder rename failed. Unable to ensure correct folder convention." << std::endl;
                }
            }
        }

        void AssetOrganizer::enforceGameAssetLocation(Info& asset) const {

            //Check if game dir has asset type
            if (game_dir.find(asset.type) != game_dir.end()) {

                //Get engine dir
                auto dir_path = game_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = raw_path / dir_path / asset.name;

                    //Reposition asset into the right directory
                    if (repositionFile(asset.raw_path, target)) {

                        //Update asset details
                        asset.raw_path = target;
                        asset.relative_folder = std::filesystem::relative(asset.raw_path, raw_path).parent_path();
                    }
                }
            }
            else {
                //Get engine dir
                auto dir_path = engine_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = raw_path / dir_path / asset.name;

                    //Reposition asset into the right directory
                    if (repositionFile(asset.raw_path, target)) {

                        //Update asset details
                        asset.raw_path = target;
                        asset.relative_folder = std::filesystem::relative(asset.raw_path, raw_path).parent_path();
                    }
                }
            }
        }

        void AssetOrganizer::enforceEngineAssetLocation(Info& asset) const {

            //Check if game dir has asset type
            if (engine_dir.find(asset.type) != engine_dir.end()) {

                //Get engine dir
                auto dir_path = engine_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = raw_path / dir_path / asset.name;

                    //Reposition asset into the right directory
                    if (repositionFile(asset.raw_path, target)) {

                        //Update asset details
                        asset.raw_path = target;
                        asset.relative_folder = std::filesystem::relative(asset.raw_path, raw_path).parent_path();
                    }
                }
            }
            else {

                //Get engine dir
                auto dir_path = game_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = raw_path / dir_path / asset.name;

                    //Reposition asset into the right directory
                    if (repositionFile(asset.raw_path, target)) {

                        //Update asset details
                        asset.raw_path = target;
                        asset.relative_folder = std::filesystem::relative(asset.raw_path, raw_path).parent_path();
                    }
                }
            }
        }

        void AssetOrganizer::recursiveScanAllDirectories(std::filesystem::path const& path, std::function<void(std::filesystem::path const& file)> func) {

            //Look for Engine and Game directories
            for (const auto& entry : std::filesystem::directory_iterator(path)) {

                //Directory actions
                if (entry.is_directory()) {

                    //Scan all directories
                    recursiveScanAllDirectories(entry.path(), func);
                }

                //File actions
                if (entry.is_regular_file()) {

                    //Execute action if its a file
                    func(entry.path());
                }
            }
        }

        AssetOrganizer::AssetOrganizer(std::filesystem::path const& assets_root) : assets_root{ assets_root } {

            //Set desc extensions
            desc_ext = Assets::descriptor_ext;

            //Set all extensions
            extensions = Assets::getAllExtensions();

            //Set all folders
            raw_folder = Assets::raw_assets_folder;
            desc_folder = Assets::desc_assets_folder;
            game_folder = Assets::game_assets_folder;
            engine_folder = Assets::engine_assets_folder;

            //Set all game folders
            game_dir = Assets::getAllGameFolders();

            //Set all engine folders
            engine_dir = Assets::getAllEngineFolders();

            //Setup raw & desc path
            raw_path = assets_root / raw_folder;
            desc_path = assets_root / desc_folder;

            //Create compiler
            compiler = std::make_unique<Compiler>(desc_path);
        }

        void AssetOrganizer::initGameFolders(Type type, std::string const& folder) {
            game_dir.emplace(type, game_folder / folder);
        }

        void AssetOrganizer::initEngineFolders(Type type, std::string const& folder) {
            engine_dir.emplace(type, engine_folder / folder);
        }

        void AssetOrganizer::enforceStandardStructure() {

            //Ensure creation of base folders
            instantiateFolder(raw_path);
            instantiateFolder(desc_path);
            instantiateFolder(raw_path / game_folder);
            instantiateFolder(raw_path / engine_folder);
            instantiateFolder(desc_path / game_folder);
            instantiateFolder(desc_path / engine_folder);

            //Ensure standard structure for game directory
            for (const auto& dir : game_dir) {

                //Ensure raw directory exists
                std::filesystem::path fullPath = raw_path / dir.second;
                instantiateFolder(fullPath);

                //Check if directory belongs to assets that is Compilable
                if (!isAssetCompilable(dir.first)) continue;

                //Ensure descriptor directory exists
                fullPath = desc_path / dir.second;
                instantiateFolder(fullPath);
            }

            //Ensure standard structure for engine directory
            for (const auto& dir : engine_dir) {

                //Ensure raw directory exists
                std::filesystem::path fullPath = raw_path / dir.second;
                instantiateFolder(fullPath);

                //Check if directory belongs to assets that is Compilable
                if (!isAssetCompilable(dir.first)) continue;

                //Ensure descriptor directory exists
                fullPath = desc_path / dir.second;
                instantiateFolder(fullPath);
            }
        }

        void AssetOrganizer::scanAssetDirectories() {
            std::cout << "=== Scanning Asset Directories ===" << std::endl;
            std::cout << "Assets Root: " << assets_root << std::endl;

            //Clear all outstanding assets
            assets.clear();

            //Tidy up root directory
            for (const auto& entry : std::filesystem::directory_iterator(assets_root)) {
                //Directory actions
                if (entry.is_directory()) {
                    if (entry.path().filename() != raw_folder || entry.path().filename() != desc_folder) {
                        //Recursively handle all assets and delete directory
                    }
                }
                if (entry.is_regular_file()) {
                    //Handle files by placing them in correct directories
                }
            }

            //Scan raw asset directory
            recursiveScanAllDirectories(raw_path, [&](std::filesystem::path const& file) {

                //create asset info
                Info asset;
                asset.raw_path = file;
                asset.name = asset.raw_path.filename().string();
                asset.relative_folder = std::filesystem::relative(asset.raw_path, raw_path).parent_path();
                asset.type = getAssetType(asset.raw_path);
                asset.raw_last_modified = getFileLastModified(asset.raw_path);

                //Check if asset is in engine or game
                if (isPathPartOfRoot(asset.relative_folder, game_folder)) {

                    //Enforce asset location
                    enforceGameAssetLocation(asset);
                }
                if (isPathPartOfRoot(asset.relative_folder, engine_folder)) {

                    //Enforce asset location
                    enforceEngineAssetLocation(asset);
                }

                //Compile asset
                compiler->processAsset(asset);

                //Inser asset into assets
                assets.push_back(asset);

                //Insert GUID into cache for assets without GUID
                if (guid_cache.find(asset.raw_path) == guid_cache.end()) {
                    guid_cache[asset.raw_path] = GUID::Generate();
                }

                });
        }

        void AssetOrganizer::tidyUpDirectories() {
            //Tidy up additional directories
            for (const auto& entry : std::filesystem::directory_iterator(assets_root)) {
                if (entry.is_directory()) {
                    if (entry.path().filename() != raw_folder && entry.path().filename() != desc_folder) {
                        deleteFile(entry.path());
                    }
                }
            }

            for (const auto& entry : std::filesystem::directory_iterator(raw_path)) {
                if (entry.is_directory()) {
                    if (entry.path().filename() != game_folder && entry.path().filename() != engine_folder) {
                        deleteFile(entry.path());
                    }
                }
            }

            for (const auto& entry : std::filesystem::directory_iterator(desc_path)) {
                if (entry.is_directory()) {
                    if (entry.path().filename() != game_folder && entry.path().filename() != engine_folder) {
                        deleteFile(entry.path());
                    }
                }
            }

            for (const auto& entry : std::filesystem::directory_iterator(raw_path / game_folder)) {
                if (entry.is_directory()) {

                    bool b_found = false;
                    for (const auto& dir : game_dir) {
                        if (raw_path / dir.second == entry.path()) {
                            b_found = true;
                            break;
                        }
                    }
                    if (!b_found) deleteFile(entry.path());
                }
            }

            for (const auto& entry : std::filesystem::directory_iterator(raw_path / engine_folder)) {
                if (entry.is_directory()) {

                    bool b_found = false;
                    for (const auto& dir : engine_dir) {
                        if (raw_path / dir.second == entry.path()) {
                            b_found = true;
                            break;
                        }
                    }
                    if (!b_found) deleteFile(entry.path());
                }
            }
        }
    }
}
