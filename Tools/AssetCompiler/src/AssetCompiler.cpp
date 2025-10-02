#include "AssetCompiler.h"

bool AssetCompiler::isAssetCompilable(AssetType type) const {
    if (type == AssetType::Texture || type == AssetType::Model || type == AssetType::Audio) return true;
    return false;
}
void AssetCompiler::initExtensions() {

    //Compiled assets extensions
    texture_exts = { ".png", ".jpg", ".jpeg" };
    model_exts = { ".obj" };
    audio_exts = { ".wav", ".mp3", ".ogg" };

    //Non compiled assets extensions
    script_exts = { ".lua" };
    data_exts = { ".json" };
    shader_exts = { ".vert", ".frag" };

    //Desc file extension
    desc_ext = ".desc";
}

void AssetCompiler::initGameFolders() {
    std::filesystem::path game_path = "Game";

    game_dir.emplace(AssetType::Texture, game_path / "Textures");
    game_dir.emplace(AssetType::Model, game_path / "Models");
    game_dir.emplace(AssetType::Audio, game_path / "Audio");
    game_dir.emplace(AssetType::Script, game_path / "Scripts");
    game_dir.emplace(AssetType::Data, game_path / "Data");
    game_dir.emplace(AssetType::Other, game_path / "Others");
}

void AssetCompiler::initEngineFolders() {
    std::filesystem::path engine_path = "Engine";

    engine_dir.emplace(AssetType::Texture, engine_path / "Textures");
    engine_dir.emplace(AssetType::Model, engine_path / "Models");
    engine_dir.emplace(AssetType::Audio, engine_path / "Audio");
    engine_dir.emplace(AssetType::Script, engine_path / "Scripts");
    engine_dir.emplace(AssetType::Data, engine_path / "Data");
    engine_dir.emplace(AssetType::Shader, engine_path / "Shaders");
    engine_dir.emplace(AssetType::Other, engine_path / "Others");
}

void AssetCompiler::enforceStandardStructure() {
    //Ensure standard structure for game directory
    for (const auto& dir : game_dir) {

        //Ensure raw directory exists
        std::filesystem::path fullPath = raw_path / dir.second;
        if (!std::filesystem::exists(fullPath)) {
            try {
                std::filesystem::create_directories(fullPath);
            }
            catch (const std::exception& e) {
                std::cout << "  Failed to create: Raw/" << dir.second << " - " << e.what() << std::endl;
            }
        }

        //Check if directory belongs to assets that is Compilable
        if (!isAssetCompilable(dir.first)) continue;

        //Ensure descriptor directory exists
        fullPath = desc_path / dir.second;
        if (!std::filesystem::exists(fullPath)) {
            try {
                std::filesystem::create_directories(fullPath);
            }
            catch (const std::exception& e) {
                std::cout << "  Failed to create: Raw/" << dir.second << " - " << e.what() << std::endl;
            }
        }
    }

    //Ensure standard structure for engine directory
    for (const auto& dir : engine_dir) {

        //Ensure raw directory exists
        std::filesystem::path fullPath = raw_path / dir.second;
        if (!std::filesystem::exists(fullPath)) {
            try {
                std::filesystem::create_directories(fullPath);
            }
            catch (const std::exception& e) {
                std::cout << "  Failed to create: Raw/" << dir.second << " - " << e.what() << std::endl;
            }
        }

        //Check if directory belongs to assets that is Compilable
        if (!isAssetCompilable(dir.first)) continue;

        //Ensure descriptor directory exists
        fullPath = desc_path / dir.second;
        if (!std::filesystem::exists(fullPath)) {
            try {
                std::filesystem::create_directories(fullPath);
            }
            catch (const std::exception& e) {
                std::cout << "  Failed to create: Raw/" << dir.second << " - " << e.what() << std::endl;
            }
        }
    }
}

bool AssetCompiler::isPathPartOfRoot(std::filesystem::path const& path, std::filesystem::path const& root) const {

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

bool AssetCompiler::deleteFile(std::filesystem::path const& file_path) {
    try {
        if (std::filesystem::remove(file_path)) {
            std::cout << file_path << " - Deleted." << std::endl;
            return true;
        }
        else {
            std::cout << file_path << " - Unable To Delete." << std::endl;
            return false;
        }
    }
    catch (const std::filesystem::filesystem_error& e) {
        std::cout << file_path << " - Unable To Delete." << std::endl;
        return false;
    }
}

void AssetCompiler::recursiveScanAllDirectories(std::filesystem::path const& path) {

    //Look for Engine and Game directories
    for (const auto& entry : std::filesystem::directory_iterator(path)) {

        //Directory actions
        if (entry.is_directory()) {
            //Get directory name and log
            std::string dirName = entry.path().filename().string();
            std::cout << dirName << std::endl;

            //Scan all directories
            recursiveScanAllDirectories(entry.path());
        }

        //File actions
        else {

            std::cout << entry.path().extension().string() << std::endl;

            //Check for current directory
            //if (isPathPartOfRoot(entry.path(), desc_path)) {

            //    std::string ext = entry.path().extension().string();
            //}
            //else {

            //}

            ////Identify file type
            //AssetInfo info;
            //info.file_path = entry.path();
            //info.relative_path = std::filesystem::relative(entry.path(), rawDir);
            //info.type = detectAssetType(entry.path());
            //info. = needsDescriptor(info.type);
            //info.assetId = generateAssetId(entry.path(), rawDir);

        }
    }
}

AssetCompiler::AssetCompiler(std::filesystem::path const& assets_root) : assets_root{ assets_root } {

    //Initialize extensions
    initExtensions();
    
    //Set paths
    raw_path = assets_root / "Raw";
    desc_path = assets_root / "Descriptors";

    //Configure folders
    initGameFolders();
    initEngineFolders();

    //Ensure proper folder structure
    enforceStandardStructure();
}

void AssetCompiler::scanAssetDirectories() {
    std::cout << "=== Scanning Asset Directories ===" << std::endl;
    std::cout << "Assets Root: " << assets_root << std::endl;

    //Clear all outstanding assets
    assets.clear();

    //Look for Engine and Game directories
    recursiveScanAllDirectories(assets_root);
}

void AssetCompiler::generateMissingDescriptors() {

}