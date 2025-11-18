#include "AssetOrganizer.h"

namespace PAIN {
    namespace Assets {

        bool Organizer::isPathPartOfRoot(std::filesystem::path const& path, std::filesystem::path const& root) const {

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

        void Organizer::instantiateFolder(std::filesystem::path const& path) const {

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

        bool Organizer::removeFile(std::filesystem::path const& file_path) const {
            //Reposition asset into the right directory
            if (!std::filesystem::exists(file_path) || deleteFile(file_path)) {

                //Identify if there are lagging files
                if (std::filesystem::is_directory(file_path)) {
                    return true;
                }

                //Update lagging desc files if there are any
                auto lagging_desc = assets_root / std::filesystem::relative(file_path, assets_root).parent_path() / (file_path.string() + desc_ext);
                if (std::filesystem::exists(lagging_desc)) {
                    deleteFile(lagging_desc);
                }

                return true;
            }

            return false;
        }

        bool Organizer::moveFile(std::filesystem::path const& from, std::filesystem::path const& to) const {
            //Reposition asset into the right directory
            if ((!std::filesystem::exists(from) && std::filesystem::exists(to)) || repositionFile(from, to)) {

                //Identify if there are lagging files
                if (std::filesystem::is_directory(from)) {
                    return true;
                }

                //Update lagging desc files if there are any
                auto lagging_desc = assets_root / std::filesystem::relative(from, assets_root).parent_path() / (from.filename().string() + desc_ext);
                if (std::filesystem::exists(lagging_desc)) {
                    auto target_desc = assets_root / std::filesystem::relative(to, assets_root).parent_path() / (to.filename().string() + desc_ext);
                    repositionFile(lagging_desc, target_desc);
                }

                return true;
            }

            return false;
        }

        bool Organizer::moveFile(Info& asset, std::filesystem::path const& to) const {
            //Reposition asset into the right directory
            if ((!std::filesystem::exists(asset.raw_path) && std::filesystem::exists(to)) || repositionFile(asset.raw_path, to)) {

                //Update lagging desc files if there are any
                auto lagging_desc = assets_root / std::filesystem::relative(asset.raw_path, assets_root).parent_path() / (asset.raw_path.filename().string() + desc_ext);
                if (std::filesystem::exists(lagging_desc)) {
                    auto target_desc = assets_root / std::filesystem::relative(to, assets_root).parent_path() / (to.filename().string() + desc_ext);
                    repositionFile(lagging_desc, target_desc);
                }

                //Update asset details
                asset.raw_path = to;
                asset.relative_folder = std::filesystem::relative(asset.raw_path, assets_root).parent_path();

                return true;
            }

            return false;
        }

        void Organizer::enforceGameAssetLocation(Info& asset) const {

            //Identify asset relative folder
            std::filesystem::path relative_path;

            //Get relative path
            relative_path = std::filesystem::relative(asset.relative_folder, getAllGameFolders()[Assets::Type::Model]);
            if (relative_path.empty()) {
                relative_path = std::filesystem::relative(asset.relative_folder, getAllEngineFolders()[Assets::Type::Model]);
            }

            //Check if game dir has asset type
            if (game_dir.find(asset.type) != game_dir.end()) {

                //Get engine dir
                auto dir_path = game_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = assets_root / dir_path / relative_path / asset.name;

                    //Create parent path just to be sure
                    std::filesystem::create_directories(target.parent_path());

                    //Reposition asset into the right directory
                    moveFile(asset, target);
                }
            }
            else {
                //Get engine dir
                auto dir_path = engine_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = assets_root / dir_path / relative_path / asset.name;

                    //Create parent path just to be sure
                    std::filesystem::create_directories(target.parent_path());

                    //Reposition asset into the right directory
                    moveFile(asset, target);
                }
            }
        }

        void Organizer::enforceEngineAssetLocation(Info& asset) const {

            //Identify asset relative folder
            std::filesystem::path relative_path;

            //Get relative path
            relative_path = std::filesystem::relative(asset.relative_folder, getAllGameFolders()[Assets::Type::Model]);
            if (relative_path.empty()) {
                relative_path = std::filesystem::relative(asset.relative_folder, getAllEngineFolders()[Assets::Type::Model]);
            }

            //Check if game dir has asset type
            if (engine_dir.find(asset.type) != engine_dir.end()) {

                //Get engine dir
                auto dir_path = engine_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = assets_root / dir_path / relative_path / asset.name;

                    //Create parent path just to be sure
                    std::filesystem::create_directories(target.parent_path());

                    //Reposition asset into the right directory
                    moveFile(asset, target);
                }
            }
            else {

                //Get engine dir
                auto dir_path = game_dir.at(asset.type);

                //Check if asset is in the right directory
                if (!isPathPartOfRoot(asset.relative_folder, dir_path)) {

                    //Get target path
                    auto target = assets_root / dir_path / relative_path / asset.name;

                    //Create parent path just to be sure
                    std::filesystem::create_directories(target.parent_path());

                    //Reposition asset into the right directory
                    moveFile(asset, target);
                }
            }
        }

        void Organizer::recursiveScanAllDirectories(std::filesystem::path const& path,
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

        void Organizer::ExportAssetRegistry() {
            try {
                nlohmann::json registry;
                for (const auto& info : assets) {
                    registry[info.guid.ToString()] = {
                        {"type", assetTypeToString(info.type)},
                        {"name", info.name},
                        {"main_relative_path", info.main_relative_path.string()},
                        {"shipped_relative_path", info.shipped_relative_path.string()}
                    };
                }

                //Get asset registry path
                std::filesystem::path out_path = output_dir / asset_registry_filename;

                //Dump json
                std::ofstream file(out_path);
                file << registry.dump(2);

                //Success output
                std::cout << "Asset registry successfully saved to: " << out_path << std::endl;
            }
            catch (const std::exception& e) {
                throw std::runtime_error("Error saving asset registry.");
            }
        }

        Organizer::Organizer(std::filesystem::path const& input_path, std::filesystem::path const& output_path, Platform const& platform, std::filesystem::path const& exec_path) : assets_root{ input_path }, output_dir{ output_path }, exec_path{ exec_path } {

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

        void Organizer::initGameFolders(Type type, std::string const& folder) {
            game_dir.emplace(type, game_folder / folder);
        }

        void Organizer::initEngineFolders(Type type, std::string const& folder) {
            engine_dir.emplace(type, engine_folder / folder);
        }

        void Organizer::enforceStandardStructure() {

            //Ensure creation of base folders
            instantiateFolder(assets_root / game_folder);
            instantiateFolder(assets_root / engine_folder);

            //Ensure standard structure for game directory
            for (const auto& dir : game_dir) {

                //Ensure raw directory mesh_id
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

                //Ensure raw directory mesh_id
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

        std::optional<std::vector<IAsset>> Organizer::organizeAndProcessAsset(std::filesystem::path const& file_path) const {

            //Return if not a valid file
            if (!std::filesystem::exists(file_path) || file_path.extension() == desc_ext) return std::nullopt;

            //Lamda function
            auto organize_and_process = [&](std::filesystem::path const& file_path)->std::optional<IAsset> {
                //create asset info
                Info asset;
                asset.raw_path = file_path;
                asset.name = asset.raw_path.filename().string();
                asset.relative_folder = std::filesystem::relative(asset.raw_path, assets_root).parent_path();
                asset.type = getAssetType(asset.raw_path);

                //Check if asset is in engine or game
                if (isPathPartOfRoot(asset.relative_folder, game_folder)) {

                    //Enforce asset location
                    enforceGameAssetLocation(asset);
                }
                else if (isPathPartOfRoot(asset.relative_folder, engine_folder)) {

                    //Enforce asset location
                    enforceEngineAssetLocation(asset);
                }
                else {
                    //Enforce asset location into game folder
                    enforceGameAssetLocation(asset);
                }

                //Skip bin files
                if (asset.raw_path.extension() == ".bin" || asset.raw_path.extension() == desc_ext) return std::nullopt;

                //Update asset relative path
                asset.relative_path = std::filesystem::relative(asset.raw_path, assets_root);

                //Compile asset
                compiler->processAsset(asset);

                //Craft asset interface
                IAsset asset_interface;
                asset_interface.guid = asset.guid;
                asset_interface.name = asset.shipped_path.filename().string();
                asset_interface.type = asset.type;
                asset_interface.main_relative_path = asset.relative_path;
                asset_interface.shipped_relative_path = asset.relative_path.parent_path() / asset.shipped_path.filename();

                return asset_interface;
                };

            //Check if directory
            if (std::filesystem::is_directory(file_path)) {

                //Empty vec
                std::vector<IAsset> vec;

                //Recursive iterate
                for (auto const& entry : std::filesystem::recursive_directory_iterator(file_path)) {
                    auto asset = organize_and_process(file_path);
                    if (asset.has_value()) vec.push_back(asset.value());
                }

                if(!vec.empty()) return vec;
            }
            else {
                auto asset = organize_and_process(file_path);
                if (asset.has_value()) {
                    std::vector<IAsset> vec = { asset.value() };
                    return vec;
                }
            }

            return std::nullopt;
        }

        void Organizer::scanAssetDirectories() {
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
                    if (!std::filesystem::exists(file.parent_path() / file.stem())) deleteFile(file);
                    return;
                }

                //Organize and process asset
                auto asset_vec = organizeAndProcessAsset(file);

                //Insert asset into assets
                if (asset_vec.has_value()) {
                    
                    //Iterate through all values
                    for (auto const& i_asset : asset_vec.value()) {
                        assets.push_back(i_asset);
                    }
                }
                },
                [](std::filesystem::path) {});

            //Craft asset registry
            ExportAssetRegistry();
        }

        void Organizer::tidyUpDirectories() {
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
