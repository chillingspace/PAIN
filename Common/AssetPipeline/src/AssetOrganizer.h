#pragma once

#ifndef ASSET_ORGANIZER_HPP
#define ASSET_ORGANIZER_HPP

#include <filesystem>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <unordered_map>
#include <functional>
#include <memory>

//Include asset compiler
#include "AssetTypes.h"
#include "AssetCompiler.h"

namespace PAIN {
    namespace Assets {

        //Asset compiler class
        class AssetOrganizer {
        private:

            //Assset compiler
            std::unique_ptr<Compiler> compiler;

            //Assets info for sorting
            std::vector<Info> assets;

            //Assets root path
            std::filesystem::path assets_root;
            std::filesystem::path exec_path;

            //Paths
            std::unordered_map<Type, std::filesystem::path> game_dir;
            std::unordered_map<Type, std::filesystem::path> engine_dir;

            //Folder names
            std::filesystem::path game_folder;
            std::filesystem::path engine_folder;

            //Desc file extension
            std::string desc_ext;

            //Compilation extensions
            std::unordered_map<Type, std::set<std::string>> extensions;

            //Check if path is derived
            bool isPathPartOfRoot(std::filesystem::path const& path, std::filesystem::path const& root) const;

            //Reposition file
            bool repositionFile(std::filesystem::path const& file_path, std::filesystem::path const& target_path) const;

            //Delete file
            bool deleteFile(std::filesystem::path const& file_path) const;

            //Create or rename folder
            void instantiateFolder(std::filesystem::path const& path) const;

            //Check asset is in right directory and reposition
            void enforceGameAssetLocation(Info& asset) const;
            void enforceEngineAssetLocation(Info& asset) const;

            //Recursively scan the directory
            void recursiveScanAllDirectories(std::filesystem::path const& path, std::function<void(std::filesystem::path const& file)> func);

        public:
            AssetOrganizer(std::filesystem::path const& assets_root, std::filesystem::path const& exec_path);
            ~AssetOrganizer() = default;

            //Game folders
            void initGameFolders(Type type, std::string const& folder);

            //Engine folders
            void initEngineFolders(Type type, std::string const& folder);

            //Create standard structure
            void enforceStandardStructure();

            //core functions
            void scanAssetDirectories();

            //Tidy additional directories
            void tidyUpDirectories();
        };
    }
}

#endif
