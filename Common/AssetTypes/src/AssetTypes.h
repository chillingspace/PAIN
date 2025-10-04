#pragma once

#ifndef ASSET_TYPES_HPP
#define ASSET_TYPES_HPP

#include <unordered_map>
#include <set>
#include <string>
#include <filesystem>

namespace PAIN {

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

    namespace Assets {

        //Asset types
        enum class Type {
            Texture,    // .png, .jpg
            Model,      // .obj
            Audio,      // .wav, .mp3, .ogg
            Script,     // .lua
            Data,       // .json
            Shader,     // .vert, .frag
            Other
        };

        //Boolean to check if the asset is compilable
        static bool isAssetCompilable(Type type) {
            if (type == Type::Texture || type == Type::Model || type == Type::Audio) return true;
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

            return temp;
        }

        //All folder types
        static std::filesystem::path raw_assets_folder = "Raw";
        static std::filesystem::path desc_assets_folder = "Descriptors";
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

        //Asset info
        struct Info {

            //Details
            GUID guid;
            Type type;
            std::string name;

            //Raw asset details
            std::filesystem::path raw_path;
            uint64_t raw_last_modified = 0;

            //Shipped asset details
            std::filesystem::path shipped_path;
            uint64_t shipped_last_modified = 0;
            uint64_t file_size = 0;

            //Relative folder
            std::filesystem::path relative_folder;

            //Dependencies
            std::vector<GUID> dependencies;

            //Metadata
            std::unordered_map<std::string, std::string> metadata;
        };
    }
}

//Hashing got unordered_map
namespace std {
    template <>
    struct hash<PAIN::GUID> {
        size_t operator()(const PAIN::GUID& guid) const noexcept {
            const uint64_t* p64 = reinterpret_cast<const uint64_t*>(guid.bytes);
            // Combine two 64-bit parts
            return std::hash<uint64_t>{}(p64[0]) ^ (std::hash<uint64_t>{}(p64[1]) << 1);
        }
    };
}

#endif