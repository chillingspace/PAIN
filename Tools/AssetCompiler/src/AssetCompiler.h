#pragma once

#ifndef ASSET_COMPILER_HPP
#define ASSET_COMPILER_HPP

#include <filesystem>
#include <string>
#include <vector>
#include <set>
#include <iostream>
#include <unordered_map>
#include <functional>

enum class AssetType {
    Texture,    // .png, .jpg
    Model,      // .obj
    Audio,      // .wav, .mp3, .ogg
    Script,     // .lua
    Data,       // .json
    Shader,     // .vert, .frag
    Other
};

struct AssetInfo {
    std::filesystem::path file_path;
    std::filesystem::path relative_path;
    std::filesystem::path directory_path;
    std::string asset_id;                 
    AssetType type;       

    //Desc files optional
    bool b_needs_desc;     
    bool b_has_desc;  
    std::filesystem::path desc_path;
};

//Asset compiler class
class AssetCompiler {
private:
    //Assets info for sorting
    std::vector<AssetInfo> assets;

    //Assets root path
    std::filesystem::path assets_root;
    std::filesystem::path raw_path;
    std::filesystem::path desc_path;

    //Paths
    std::unordered_map<AssetType, std::filesystem::path> game_dir;
    std::unordered_map<AssetType, std::filesystem::path> engine_dir;

    //Compilation extensions
    std::unordered_map<AssetType, std::set<std::string>> extensions;

    //Desc file extension
    std::string desc_ext;

    //Folder names
    std::filesystem::path raw_folder;
    std::filesystem::path desc_folder;
    std::filesystem::path game_folder;
    std::filesystem::path engine_folder;

    //Function to check if asset is compilable
    bool isAssetCompilable(AssetType type) const;
    
    //Extension types
    void initExtensions();

    //Game folders
    void initGameFolders();

    //Engine folders
    void initEngineFolders();

    //Create standard structure
    void enforceStandardStructure();

    //Check if path is derived
    bool isPathPartOfRoot(std::filesystem::path const& path, std::filesystem::path const& root) const;

    //Internal to lower case for string
    std::string toLowerCase(std::string const& string) const;

    //Get Asset type from path
    AssetType getAssetType(std::filesystem::path const& file) const;

    //Reposition file
    bool repositionFile(std::filesystem::path const& file_path, std::filesystem::path const& target_path) const;

    //Delete file
    bool deleteFile(std::filesystem::path const& file_path) const;

    //Check asset is in right directory and reposition
    void enforceGameAssetLocation(AssetInfo& asset) const;
    void enforceEngineAssetLocation(AssetInfo& asset) const;

    //Recursively scan the directory
    void recursiveScanAllDirectories(std::filesystem::path const& path, std::function<void(std::filesystem::path const& file)> func);

public:
	AssetCompiler(std::filesystem::path const& assets_root);
	~AssetCompiler() = default;

    //core functions
    void scanAssetDirectories();
    void generateMissingDescriptors();
};

#endif
