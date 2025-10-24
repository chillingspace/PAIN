
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
#elif __APPLE__
    char buffer[PATH_MAX];
    uint32_t size = sizeof(buffer);
    if (_NSGetExecutablePath(buffer, &size) == 0) {
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

    //Auto-discover project root from executable location
    std::filesystem::path projectRoot = findProjectRoot();
    std::filesystem::path assetsRoot = projectRoot / "assets";

	//create assset compiler
    PAIN::Assets::AssetOrganizer* compiler = new PAIN::Assets::AssetOrganizer(assetsRoot, getExecutablePath());

    //Enforce a standard structure for assets
    compiler->enforceStandardStructure();

    //Scan directory
    compiler->scanAssetDirectories();

    //Tidy up any additional directories
    compiler->tidyUpDirectories();

    //Clean up resource
	delete compiler;
}