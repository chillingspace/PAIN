
#include "AssetOrganizer.h"

#include <iostream>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#elif __linux__ || __APPLE__
#include <unistd.h>
#include <limits.h>
#endif

static std::filesystem::path getExecutablePath() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::filesystem::path(buffer).parent_path();
#elif __linux__
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
    return std::filesystem::current_path();
#else
    return std::filesystem::current_path();
#endif
}

static std::filesystem::path findProjectRoot() {
    // Get the actual executable directory
    std::filesystem::path execDir = getExecutablePath();

    std::cout << "Executable directory: " << execDir << std::endl;

    // Search upward from executable location
    std::filesystem::path currentPath = execDir;

    for (int levels = 0; levels < 10; levels++) {
        std::filesystem::path assetsPath = currentPath / "assets";


        if (std::filesystem::exists(assetsPath)) {
            std::cout << "Found project root: " << currentPath << std::endl;
            return currentPath;
        }

        currentPath = currentPath.parent_path();
        if (currentPath.empty() || currentPath == currentPath.root_path()) {
            break;
        }
    }

    throw std::runtime_error("Could not find project root containing Assets/ directory");
}

int main(int argc, char* argv[]) {

    std::filesystem::path input_path;
    std::filesystem::path output_path;
    PAIN::Assets::Platform build_platform;

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
    PAIN::Assets::AssetOrganizer* compiler = new PAIN::Assets::AssetOrganizer(input_path, output_path, build_platform,getExecutablePath());

    //Enforce a standard structure for assets
    compiler->enforceStandardStructure();

    //Scan directory
    compiler->scanAssetDirectories();

    //Tidy up any additional directories
    compiler->tidyUpDirectories();

    //Clean up resource
	delete compiler;
}