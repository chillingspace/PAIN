
#include "AssetOrganizer.h"

#include <iostream>
#include <filesystem>

int main(int argc, char* argv[]) {

    std::filesystem::path input_path = PAIN::Assets::findProjectRoot() / "assets";
    std::filesystem::path output_path = PAIN::Assets::findProjectRoot() / "bin/Debug/assets";
    PAIN::Assets::Platform build_platform = PAIN::Assets::Platform::Windows;

    //Retrieve input and output directories
    for (int i = 1; i < argc; ++i) {

        if (std::string(argv[i]) == "--input" && i + 1 < argc) {
            input_path = std::string(argv[i + 1]);
        }

        if (std::string(argv[i]) == "--output" && i + 1 < argc) {
            output_path = std::string(argv[i + 1]);
        }

        if (std::string(argv[i]) == "--target" && i + 1 < argc) {
            auto platform_str = std::string(argv[i + 1]);

            if (platform_str == "windows") {
                build_platform = PAIN::Assets::Platform::Windows;
            }
            else if(platform_str == "android") {
                build_platform = PAIN::Assets::Platform::Android;
            }
            else {
                build_platform = PAIN::Assets::Platform::Windows;
            }

            std::cout << "Asset Compiler Running On: " << platform_str << std::endl;
        }
    }

	//create assset compiler
    PAIN::Assets::Organizer* organizer = new PAIN::Assets::Organizer(input_path, output_path, build_platform, PAIN::Assets::getExecutablePath());

    //Enforce a standard structure for assets
    organizer->enforceStandardStructure();

    //Scan directory
    organizer->scanAssetDirectories();

    //Tidy up any additional directories
    organizer->tidyUpDirectories();

    //Output stamp file
    std::filesystem::path debug_file = output_path / "assetcompiler_stamp.txt";
    std::ofstream test_out(debug_file);
    if (test_out.is_open()) {
        test_out << "AssetCompiler ran successfully at: " << std::time(nullptr) << std::endl;
        test_out.close();
    }
    else {
        std::cerr << "Failed to write debug test file: " << debug_file << std::endl;
    }

    //Clean up resource
	delete organizer;
}