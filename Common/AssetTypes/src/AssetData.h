#pragma once

#ifndef ASSET_DATA_HPP
#define ASSET_DATA_HPP

#include <unordered_map>
#include <set>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <thread> // For std::this_thread::sleep_for

#ifdef _WIN32
#include <windows.h>
#elif __linux__ || __APPLE__
#include <unistd.h>
#include <limits.h>
#endif

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace PAIN {
    namespace Assets {

        //Asset platform
        enum class Platform {
            Windows = 0,
            Android
        };

        struct GUID {
            uint8_t bytes[16] = {};

            //Default constructor
            GUID() = default;

            //Generate new GUID
            static GUID Generate() noexcept;

            //Construct GUID from string
            explicit GUID(const std::string& str);

            //Serialize GUID to string
            std::string ToString(bool withHyphens = true) const;

            bool IsValid() const noexcept {
                for (auto b : bytes) {
                    if (b != 0) return true;
                }
                return false;
            }

            bool operator==(const GUID& other) const noexcept {
                return std::equal(std::begin(bytes), std::end(bytes), std::begin(other.bytes));
            }

            bool operator!=(const GUID& other) const noexcept {
                return !(*this == other);
            }
        };

        //Asset types
        enum class Type {
            Texture,    // .png, .jpg, .jpeg, .hdr, .tex
            Model,      // .obj, .gltf
            Audio,      // .wav, .mp3, .ogg
            Script,     // .lua
            Data,       // .json
            Shader,     // .vert, .frag
            Scenes,
            Font,
            Prefabs, // .prefab
            Templates,
            Material,
            Other
        };

        static std::unordered_map<Type, std::string> getTypeStringMapping() {
            std::unordered_map<Type, std::string> temp;

            temp[Type::Texture] = "Texture";
            temp[Type::Model] = "Model";
            temp[Type::Audio] = "Audio";
            temp[Type::Script] = "Script";
            temp[Type::Data] = "Data";
            temp[Type::Shader] = "Shader";
            temp[Type::Scenes] = "Scenes";
            temp[Type::Font] = "Font";
            temp[Type::Prefabs] = "Prefabs";
            temp[Type::Templates] = "Templates";
            temp[Type::Material] = "Materials";
            temp[Type::Other] = "Other";

            return temp;
        }

        static std::string assetTypeToString(Type const& type) {
            auto map = getTypeStringMapping();
            return map.at(type);
        }

        static Type stringToAssetType(std::string const& string) {
            auto map = getTypeStringMapping();
            for (auto entry : map) {
                if (entry.second == string) return entry.first;
            }

            return Type::Other;
        }

        static bool isAssetCacheable(Type const& type) {
            if (type == Type::Texture || type == Type::Audio || type == Type::Model || type == Type::Shader || type == Type::Font || type == Type::Script || type == Type::Material) return true;
            return false;
        }

        //Boolean to check if the asset is compilable
        static bool isAssetCompilable(Type const& type) {
            if (type == Type::Texture || type == Type::Audio || type == Type::Model) return true;
            return false;
        }

        //Descriptor file extension
        static std::string descriptor_ext = ".desc";

        //Asset registry path
        static std::string asset_registry_filename = "asset_registry.json";

        //All extensions
        static std::unordered_map<Type, std::set<std::string>> getAllExtensions() {

            std::unordered_map<Type, std::set<std::string>> temp;

            //Set up extensions for asset types
            temp[Type::Texture] = { ".png", ".jpg", ".jpeg", ".hdr", ".tex" };
            temp[Type::Model] = { ".obj", ".gltf", ".bin" };
            temp[Type::Audio] = { ".wav", ".mp3", ".ogg" };
            temp[Type::Script] = { ".lua" };
            temp[Type::Data] = { ".json" };
            temp[Type::Shader] = { ".vert", ".frag" };
            temp[Type::Scenes] = { ".scn" };
            temp[Type::Prefabs] = { ".prefab" };
            temp[Type::Templates] = { ".tmpl" };
            temp[Type::Material] = { ".material" };
            temp[Type::Font] = { ".ttf" };

            return temp;
        }

        //All folder types
        static std::filesystem::path game_assets_folder = "game";
        static std::filesystem::path engine_assets_folder = "engine";

        //All folder types based on types

        static std::unordered_map<Type, std::filesystem::path> getAllGameFolders() {
            std::unordered_map<Type, std::filesystem::path> temp;

            temp[Type::Texture] = game_assets_folder / "textures";
            temp[Type::Model] = game_assets_folder / "models";
            temp[Type::Audio] = game_assets_folder / "audio";
            temp[Type::Script] = game_assets_folder / "scripts";
            temp[Type::Data] = game_assets_folder / "data";
            temp[Type::Scenes] = game_assets_folder / "scenes";
            temp[Type::Prefabs] = game_assets_folder / "prefabs";
            temp[Type::Templates] = game_assets_folder / "templates";
            temp[Type::Material] = game_assets_folder / "materials";
            temp[Type::Other] = game_assets_folder / "others";

            return temp;
        }
        static std::unordered_map<Type, std::filesystem::path> getAllEngineFolders() {
            std::unordered_map<Type, std::filesystem::path> temp;

            temp[Type::Texture] = engine_assets_folder / "textures";
            temp[Type::Model] = engine_assets_folder / "models";
            temp[Type::Audio] = engine_assets_folder / "audio";
            temp[Type::Script] = engine_assets_folder / "scripts";
            temp[Type::Data] = engine_assets_folder / "data";
            temp[Type::Shader] = engine_assets_folder / "shaders";
            temp[Type::Font] = engine_assets_folder / "fonts";
            temp[Type::Material] = engine_assets_folder / "materials";
            temp[Type::Other] = engine_assets_folder / "others";

            return temp;
        }

        static std::string toLowerCase(std::string const& string) {
            std::string t = string;
            std::transform(t.begin(), t.end(), t.begin(), ::tolower);
            return t;
        }

        static Type getAssetType(std::filesystem::path const& file) {

            //Ensure that file is a standard type
            auto ext = toLowerCase(file.extension().string());

            //return type based on ext
            for (auto const& asset_ext : getAllExtensions()) {

                //Asset type found
                if (asset_ext.second.find(ext) != asset_ext.second.end()) {
                    return asset_ext.first;
                }
            }

            //Other asset type found
            return Type::Other;
        }

        static uint64_t getFileLastModified(std::filesystem::path const& file) {
            try {
                auto file_time = std::filesystem::last_write_time(file);

                // Convert to nanoseconds since the file_time_type epoch
                // This gives us a comparable number without clock conversion complexity
                auto duration = file_time.time_since_epoch();
                return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            }
            catch (const std::filesystem::filesystem_error&) {
                return 0; // File doesn't exist or access denied
            }
        }

        static uint64_t fileHashing(const std::filesystem::path& path) {
            const uint64_t fnv_offset_basis = 14695981039346656037ULL;
            const uint64_t fnv_prime = 1099511628211ULL;

            // Retry parameters
            const int max_attempts = 10;
            const auto retry_delay = std::chrono::milliseconds(10);

            for (int attempt = 0; attempt < max_attempts; ++attempt) {
                std::ifstream file(path, std::ios::binary);
                
                if (file.is_open()) {
                    uint64_t hash = fnv_offset_basis;
                    char buffer[4096];
                    while (file) {
                        file.read(buffer, sizeof(buffer));
                        std::streamsize count = file.gcount();
                        for (std::streamsize i = 0; i < count; ++i) {
                            hash ^= static_cast<unsigned char>(buffer[i]);
                            hash *= fnv_prime;
                        }
                    }
                    return hash;
                }

                // File failed to open (likely locked by antivirus or build process). Wait and retry.
                std::this_thread::sleep_for(retry_delay);
            }

            // If we get here, the file is genuinely inaccessible
            std::cerr << "[Warning] fileHashing failed to open file after retries: " << path << std::endl;
            return 0;
        }

        static bool deleteFile(std::filesystem::path const& file_path) {
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
                std::cout << file_path << " - Deletion Failed. " << e.what() << std::endl;
                return false;
            }
        }

        static bool repositionFile(std::filesystem::path const& file_path, std::filesystem::path const& target_path) {
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

            for (int levels = 0; levels < 25; levels++) {
                std::filesystem::path readme = currentPath / "README.md";
                std::filesystem::path buildbat = currentPath / "build.bat";


                if (std::filesystem::exists(readme) || std::filesystem::exists(buildbat)) {
                    std::cout << "Found project root: " << currentPath << std::endl;
                    return currentPath;
                }
                
                // Check for "assets" folder for portable/release builds
                std::filesystem::path asset_dir = currentPath / "assets";
                if (std::filesystem::exists(asset_dir) && std::filesystem::is_directory(asset_dir)) {
                     std::cout << "Found project root (via assets): " << currentPath << std::endl;
                     return currentPath;
                }

                currentPath = currentPath.parent_path();
                if (currentPath.empty() || currentPath == currentPath.root_path()) {
                    break;
                }
            }

            std::cerr << "Could not find project root containing Assets/ directory" << std::endl;
            throw std::runtime_error("Could not find project root containing Assets/ directory");
        }

        static bool isSubPath(const std::filesystem::path& descendantPath, const std::filesystem::path& rootPath) {
            std::error_code ec;
            auto canonDescendant = std::filesystem::weakly_canonical(descendantPath, ec);
            auto canonRoot = std::filesystem::weakly_canonical(rootPath, ec);

            if (ec) return false; // if any canonicalization fails

            // Compare each path component
            auto rootIt = canonRoot.begin();
            auto detIt = canonDescendant.begin();

            for (; rootIt != canonRoot.end(); ++rootIt, ++detIt) {
                if (detIt == canonDescendant.end() || *rootIt != *detIt)
                    return false; // paths diverge
            }
            // Passed all root elements matched; can be equal or a subpath
            return true;
        }

        static bool isMusic(std::filesystem::path const& path) {

            auto audio_exts = getAllExtensions()[Type::Audio];
            if (audio_exts.find(path.extension().string()) == audio_exts.end()) {
                std::cout << "Invalid audio type" << std::endl;
                return false;
            }

            if (path.string().find("/music/") != std::string::npos) return true;
            if (path.string().find("/bgm/") != std::string::npos) return true;
            if (path.string().find("/ambience/") != std::string::npos) return true;

            //uintmax_t fileSize = std::filesystem::file_size(path);
            //constexpr uintmax_t MUSIC_FILESIZE_THRESHOLD = 1 * 1024 * 1024;
            //return fileSize > MUSIC_FILESIZE_THRESHOLD;

            return false;
        }

        //Asset info
        struct Info {

            //Details
            GUID guid;
            Type type;
            std::string name;

            //Raw asset details
            std::filesystem::path raw_path;
            std::filesystem::path shipped_path;

            //Asset relative folder
            std::filesystem::path relative_folder;
            std::filesystem::path relative_path;
        };

        static std::filesystem::path extractSubfolderPath(Info& asset, std::filesystem::path const& assets_root) {
            std::error_code ec;

            // Get the directory containing the asset file
            auto assetDir = std::filesystem::weakly_canonical(
                asset.raw_path.parent_path(), ec);

            if (ec) {
                std::cerr << "Could not canonicalize asset directory: " << asset.raw_path.string() << std::endl;
                return std::filesystem::path();
            }

            // Check game folders (Prefabs, Scenes, etc.)
            for (const auto& [type, folder] : getAllGameFolders()) {
                // Build full path to the fixed folder
                auto fixedFolderPath = std::filesystem::weakly_canonical(
                    assets_root / folder, ec);

                if (ec) continue;

                // Check if asset is within this fixed folder
                if (isSubPath(assetDir, fixedFolderPath)) {
                    // Extract the path RELATIVE to the fixed folder
                    auto relativePath = std::filesystem::relative(assetDir, fixedFolderPath, ec);

                    if (!ec) {
                        return relativePath;
                    }
                }
            }

            // Check engine folders
            for (const auto& [type, folder] : getAllEngineFolders()) {
                auto fixedFolderPath = std::filesystem::weakly_canonical(
                    assets_root / folder, ec);

                if (ec) continue;

                if (isSubPath(assetDir, fixedFolderPath)) {
                    auto relativePath = std::filesystem::relative(assetDir, fixedFolderPath, ec);

                    if (!ec) {
                        return relativePath;
                    }
                }
            }

            return std::filesystem::path();
        }

        struct Descriptor {

            //Identity
            GUID guid;
            uint32_t descriptor_version = 1;

            //Asset class
            Type type;
            std::string name;

            //Import/processing settings
            nlohmann::json import_settings;

            //Build data
            // CHANGED: strictly 64-bit to avoid 32-bit/64-bit build inconsistencies
            uint64_t hash; 

            //Dependencies
            std::vector<GUID> dependencies;

            //Meta data
            nlohmann::json meta_data;
        };

        inline bool operator==(const Descriptor& lhs, const Descriptor& rhs)
        {
            return lhs.guid == rhs.guid
                && lhs.descriptor_version == rhs.descriptor_version
                && lhs.type == rhs.type
                && lhs.name == rhs.name
                && lhs.import_settings == rhs.import_settings
                && lhs.hash == rhs.hash
                && lhs.dependencies == rhs.dependencies
                && lhs.meta_data == rhs.meta_data;
        }

        inline bool operator!=(const Descriptor& lhs, const Descriptor& rhs)
        {
            return !(lhs == rhs);
        }
    }
}

//Hashing got unordered_map
namespace std {
    template <>
    struct hash<PAIN::Assets::GUID> {
        size_t operator()(const PAIN::Assets::GUID& guid) const noexcept {
            const uint64_t* p64 = reinterpret_cast<const uint64_t*>(guid.bytes);
            // Combine two 64-bit parts
            return std::hash<uint64_t>{}(p64[0]) ^ (std::hash<uint64_t>{}(p64[1]) << 1);
        }
    };
}

#endif