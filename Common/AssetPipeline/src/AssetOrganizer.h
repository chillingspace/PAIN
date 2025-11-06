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
#include <unordered_set>

//Include asset compiler
#include "AssetData.h"
#include "AssetCompiler.h"

namespace PAIN {
    namespace Assets {

        //Asset compiler class
        class Organizer {
        private:

            //Assset compiler
            std::unique_ptr<Compiler> compiler;

            //Assets info for sorting
            std::vector<IAsset> assets;

            //Assets root path
            std::filesystem::path assets_root;
            std::filesystem::path exec_path;
            std::filesystem::path output_dir;

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

            //Create or rename folder
            void instantiateFolder(std::filesystem::path const& path) const;

            //Move file
            bool moveFile(Info& asset, std::filesystem::path const& to) const;

            //Check asset is in right directory and reposition
            void enforceGameAssetLocation(Info& asset) const;
            void enforceEngineAssetLocation(Info& asset) const;

            //Recursively scan the directory
            void recursiveScanAllDirectories(std::filesystem::path const& path,
                std::function<void(std::filesystem::path const& file)> file_func,
                std::function<void(std::filesystem::path const& file)> dir_func);

            //Export asset registry
            void ExportAssetRegistry();
        public:
            Organizer(std::filesystem::path const& input_path, std::filesystem::path const& output_path, Platform const& platform, std::filesystem::path const& exec_path);
            ~Organizer() = default;

            //Game folders
            void initGameFolders(Type type, std::string const& folder);

            //Engine folders
            void initEngineFolders(Type type, std::string const& folder);

            //Create standard structure
            void enforceStandardStructure();

            //Move file function
            bool moveFile(std::filesystem::path const& from, std::filesystem::path const& to) const;

            //Delete file function
            bool removeFile(std::filesystem::path const& file_path) const;

            //Organize and process asset
            IAsset organizeAndProcessAsset(std::filesystem::path const& file_path) const;

            //core functions
            void scanAssetDirectories();

            //Tidy additional directories
            void tidyUpDirectories();
        };
    }
}

#endif
