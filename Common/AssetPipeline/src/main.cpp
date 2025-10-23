#include "AssetOrganizer.h"
#include <iostream>

int main(int argc, char** argv) {
    // 1) Assets root (defaults to ../Assets relative to this exe)
    std::filesystem::path assets_root =
        (argc > 1) ? argv[1] : std::filesystem::absolute("../Assets");

    std::cout << "=== PAIN Bake ===\nAssets root: " << assets_root << "\n";

    // 2) Organizer + standard folders
    PAIN::Assets::AssetOrganizer org(assets_root);

    // Game folders
    org.initGameFolders(PAIN::Assets::Type::Texture, "Textures");
    org.initGameFolders(PAIN::Assets::Type::Model,   "Models");
    org.initGameFolders(PAIN::Assets::Type::Audio,   "Audio");
    org.initGameFolders(PAIN::Assets::Type::Script,  "Scripts");
    org.initGameFolders(PAIN::Assets::Type::Data,    "Data");
    org.initGameFolders(PAIN::Assets::Type::Scenes,  "Scenes");

    // Engine folders
    org.initEngineFolders(PAIN::Assets::Type::Texture, "Textures");
    org.initEngineFolders(PAIN::Assets::Type::Model,   "Models");
    org.initEngineFolders(PAIN::Assets::Type::Audio,   "Audio");
    org.initEngineFolders(PAIN::Assets::Type::Script,  "Scripts");
    org.initEngineFolders(PAIN::Assets::Type::Data,    "Data");
    org.initEngineFolders(PAIN::Assets::Type::Shader,  "Shaders");
    org.initEngineFolders(PAIN::Assets::Type::Other,   "Others");

    // 3) Create folders + scan/bake
    org.enforceStandardStructure();
    org.scanAssetDirectories();     // generates .desc if missing & bakes
    org.tidyUpDirectories();

    std::cout << "=== Done ===\n";
    return 0;
}
