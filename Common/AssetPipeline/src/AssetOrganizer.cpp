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
                    std::cout << "Folder rename failed. Unable to ensure correct folder convention." << e.what() << std::endl;
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
                    auto target = assets_root / dir_path / asset.name;

                    //Reposition asset into the right directory
                    if (repositionFile(asset.raw_path, target)) {

                        //Update lagging desc files if there are any
                        auto lagging_desc = assets_root / std::filesystem::relative(asset.raw_path, assets_root).parent_path() / (asset.raw_path.string() + desc_ext);
                        if (std::filesystem::exists(lagging_desc)) {
                            auto target_desc = assets_root / std::filesystem::relative(target, assets_root).parent_path() / (asset.raw_path.string() + desc_ext);
                            repositionFile(lagging_desc, target_desc);
                        }

                        //Update asset details
                        asset.raw_path = target;
                        asset.relative_folder = std::filesystem::relative(asset.raw_path, assets_root).parent_path();
                    }
                }
            }
            else {
                //Get engine dir
                auto dir_path = engine_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = assets_root / dir_path / asset.name;

                    //Reposition asset into the right directory
                    if (repositionFile(asset.raw_path, target)) {

                        //Update lagging desc files if there are any
                        auto lagging_desc = assets_root / std::filesystem::relative(asset.raw_path, assets_root).parent_path() / (asset.raw_path.string() + desc_ext);
                        if (std::filesystem::exists(lagging_desc)) {
                            auto target_desc = assets_root / std::filesystem::relative(target, assets_root).parent_path() / (asset.raw_path.string() + desc_ext);
                            repositionFile(lagging_desc, target_desc);
                        }

                        //Update asset details
                        asset.raw_path = target;
                        asset.relative_folder = std::filesystem::relative(asset.raw_path, assets_root).parent_path();
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
                    auto target = assets_root / dir_path / asset.name;

                    //Reposition asset into the right directory
                    if (repositionFile(asset.raw_path, target)) {

                        //Update lagging desc files if there are any
                        auto lagging_desc = assets_root / std::filesystem::relative(asset.raw_path, assets_root).parent_path() / (asset.raw_path.string() + desc_ext);
                        if (std::filesystem::exists(lagging_desc)) {
                            auto target_desc = assets_root / std::filesystem::relative(target, assets_root).parent_path() / (asset.raw_path.string() + desc_ext);
                            repositionFile(lagging_desc, target_desc);
                        }

                        //Update asset details
                        asset.raw_path = target;
                        asset.relative_folder = std::filesystem::relative(asset.raw_path, assets_root).parent_path();
                    }
                }
            }
            else {

                //Get engine dir
                auto dir_path = game_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = assets_root / dir_path / asset.name;

                    //Reposition asset into the right directory
                    if (repositionFile(asset.raw_path, target)) {

                        //Update lagging desc files if there are any
                        auto lagging_desc = assets_root / std::filesystem::relative(asset.raw_path, assets_root).parent_path() / (asset.raw_path.string() + desc_ext);
                        if (std::filesystem::exists(lagging_desc)) {
                            auto target_desc = assets_root / std::filesystem::relative(target, assets_root).parent_path() / (asset.raw_path.string() + desc_ext);
                            repositionFile(lagging_desc, target_desc);
                        }

                        //Update asset details
                        asset.raw_path = target;
                        asset.relative_folder = std::filesystem::relative(asset.raw_path, assets_root).parent_path();
                    }
                }
            }
        }

        void AssetOrganizer::recursiveScanAllDirectories(std::filesystem::path const& path,
            std::function<void(std::filesystem::path const& file)> file_func,
            std::function<void(std::filesystem::path const& file)> dir_func) {

            //Look for Engine and Game directories
            for (const auto& entry : std::filesystem::directory_iterator(path)) {

                //Directory actions
                if (entry.is_directory()) {

                    //Scan all directories
                    recursiveScanAllDirectories(entry.path(), file_func, dir_func);

                    //Operate dir func
                    dir_func(entry.path());
                }

                //File actions
                if (entry.is_regular_file()) {

                    //Execute action if its a file
                    file_func(entry.path());
                }
            }
        }

        AssetOrganizer::AssetOrganizer(std::filesystem::path const& input_path, std::filesystem::path const& output_path, Platform const& platform, std::filesystem::path const& exec_path) : assets_root{ input_path }, exec_path{ exec_path } {

            //Set desc extensions
            desc_ext = Assets::descriptor_ext;

            //Set all extensions
            extensions = Assets::getAllExtensions();

            //Set all folders
            game_folder = Assets::game_assets_folder;
            engine_folder = Assets::engine_assets_folder;

            //Set all game folders
            game_dir = Assets::getAllGameFolders();

            //Set all engine folders
            engine_dir = Assets::getAllEngineFolders();

            //Create compiler
            compiler = std::make_unique<Compiler>(input_path, output_path, platform, exec_path);
        }

        void AssetOrganizer::initGameFolders(Type type, std::string const& folder) {
            game_dir.emplace(type, game_folder / folder);
        }

        void AssetOrganizer::initEngineFolders(Type type, std::string const& folder) {
            engine_dir.emplace(type, engine_folder / folder);
        }

        void AssetOrganizer::enforceStandardStructure() {

            //Ensure creation of base folders
            instantiateFolder(assets_root / game_folder);
            instantiateFolder(assets_root / engine_folder);

            //Ensure standard structure for game directory
            for (const auto& dir : game_dir) {

                //Ensure raw directory exists
                std::filesystem::path fullPath = assets_root / dir.second;
                instantiateFolder(fullPath);

                //Recursive folders
                recursiveScanAllDirectories(fullPath, [](std::filesystem::path) {}, [&](std::filesystem::path const& dir) {
                    auto parent = dir.parent_path();
                    auto dir_name = dir.filename().string();
                    auto lower_name = Assets::toLowerCase(dir_name);
                    if (lower_name != dir_name) {
                        auto new_path = parent / lower_name;
                        instantiateFolder(new_path);
                    }
                    });
            }

            //Ensure standard structure for engine directory
            for (const auto& dir : engine_dir) {

                //Ensure raw directory exists
                std::filesystem::path fullPath = assets_root / dir.second;
                instantiateFolder(fullPath);

                //Recursive folders
                recursiveScanAllDirectories(fullPath, [](std::filesystem::path) {}, [&](std::filesystem::path const& dir) {
                    auto parent = dir.parent_path();
                    auto dir_name = dir.filename().string();
                    auto lower_name = Assets::toLowerCase(dir_name);
                    if (lower_name != dir_name) {
                        auto new_path = parent / lower_name;
                        instantiateFolder(new_path);
                    }
                    });
            }
        }

        void AssetOrganizer::scanAssetDirectories() {
            std::cout << "=== Scanning Asset Directories ===" << std::endl;
            std::cout << "Assets Root: " << assets_root << std::endl;

            //Clear all outstanding assets
            assets.clear();

            //Scan raw asset directory
            recursiveScanAllDirectories(assets_root, [&](std::filesystem::path const& file) {

                //Temp skip config.json
                if (file.filename() == "Config.json") return;

                //Check if asset is a desc file, locate raw asset in same directory
                if (file.extension() == desc_ext) {

                    //Flag desc for deletion
                    if(!std::filesystem::exists(file.parent_path()/file.stem())) deleteFile(file);
                    return;
                }

                //create asset info
                Info asset;
                asset.raw_path = file;
                asset.name = asset.raw_path.filename().string();
                asset.relative_folder = std::filesystem::relative(asset.raw_path, assets_root).parent_path();
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
            },
            [](std::filesystem::path) {});
        }

        void AssetOrganizer::tidyUpDirectories() {
            //Tidy up additional directories
            for (const auto& entry : std::filesystem::directory_iterator(assets_root)) {
                if (entry.is_directory()) {
                    if (entry.path().filename() != game_folder && entry.path().filename() != engine_folder) {
                        deleteFile(entry.path());
                    }
                }
            }

            for (const auto& entry : std::filesystem::directory_iterator(assets_root / game_folder)) {
                if (entry.is_directory()) {

                    bool b_found = false;
                    for (const auto& dir : game_dir) {
                        if (assets_root / dir.second == entry.path()) {
                            b_found = true;
                            break;
                        }
                    }
                    if (!b_found) deleteFile(entry.path());
                }
            }

            for (const auto& entry : std::filesystem::directory_iterator(assets_root / engine_folder)) {
                if (entry.is_directory()) {

                    bool b_found = false;
                    for (const auto& dir : engine_dir) {
                        if (assets_root / dir.second == entry.path()) {
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
