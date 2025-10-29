#pragma once

#ifndef ASSET_TYPES_HPP
#define ASSET_TYPES_HPP

#include <unordered_map>
#include <set>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>

#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace PAIN {
    namespace Assets {

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
            Texture,    // .png, .jpg
            Model,      // .obj
            Audio,      // .wav, .mp3, .ogg
            Script,     // .lua
            Data,       // .json
            Shader,     // .vert, .frag
            Scenes,
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
            temp[Type::Other] = "Other";

            return temp;
        }

        static std::string assetTypeToString(Type type) {
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

        //Boolean to check if the asset is compilable
        static bool isAssetCompilable(Type type) {
            if (type == Type::Texture /*|| type == Type::Model || type == Type::Audio*/) return true;
            return false;

        }

        //Descriptor file extension
        static std::string descriptor_ext = ".desc";

        //All extensions
        static std::unordered_map<Type, std::set<std::string>> getAllExtensions() {

            std::unordered_map<Type, std::set<std::string>> temp;

            //Set up extensions for asset types
            temp[Type::Texture] = { ".png", ".jpg", ".jpeg" };
            temp[Type::Model] = { ".obj" };
            temp[Type::Audio] = { ".wav", ".mp3", ".ogg" };
            temp[Type::Script] = { ".lua" };
            temp[Type::Data] = { ".json" };
            temp[Type::Shader] = { ".vert", ".frag" };
            temp[Type::Scenes] = { ".scn" };

            return temp;
        }

        //All folder types
        static std::filesystem::path game_assets_folder = "Game";
        static std::filesystem::path engine_assets_folder = "Engine";

        //All folder types based on types

        static std::unordered_map<Type, std::filesystem::path> getAllGameFolders() {
            std::unordered_map<Type, std::filesystem::path> temp;

            temp[Type::Texture] = game_assets_folder / "Textures";
            temp[Type::Model] = game_assets_folder / "Models";
            temp[Type::Audio] = game_assets_folder / "Audio";
            temp[Type::Script] = game_assets_folder / "Scripts";
            temp[Type::Data] = game_assets_folder / "Data";
            temp[Type::Scenes] = game_assets_folder / "Scenes";
            temp[Type::Other] = game_assets_folder / "Others";

            return temp;
        }
        static std::unordered_map<Type, std::filesystem::path> getAllEngineFolders() {
            std::unordered_map<Type, std::filesystem::path> temp;

            temp[Type::Texture] = engine_assets_folder / "Textures";
            temp[Type::Model] = engine_assets_folder / "Models";
            temp[Type::Audio] = engine_assets_folder / "Audio";
            temp[Type::Script] = engine_assets_folder / "Scripts";
            temp[Type::Data] = engine_assets_folder / "Data";
            temp[Type::Shader] = engine_assets_folder / "Shaders";
            temp[Type::Other] = engine_assets_folder / "Others";

            return temp;
        }

        static std::string toLowerCase(std::string const& string) {
            std::string new_string;
            for (auto c : string) {
                //new_string += __ascii_tolower(c);
                new_string += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

            }
            return new_string;
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

            //Build-critical timestamps
            uint64_t raw_last_modified = 0;
        };


        struct Descriptor {

            //Identity
            GUID guid;
            uint32_t descriptor_version = 1;
            uint64_t created_timestamp = 0;

            //Asset class
            Type type;
            std::string name;

            //Import/processing settings
            nlohmann::json import_settings;

            //Build data
            uint64_t raw_last_modified;

            //Dependencies
            std::vector<GUID> dependencies;

            //Meta data
            nlohmann::json meta_data;
        };
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