#pragma once

#ifndef ASSET_COMPILER_HPP
#define ASSET_COMPILER_HPP

#include "AssetTypes.h"
#include "nlohmann/json.hpp"
using json = nlohmann::json;

namespace PAIN {
	namespace Assets {

		struct Descriptor {
			GUID guid;
			Type type;
			nlohmann::json import_settings;
			uint64_t raw_last_modified;
		};

		//Asset platform
		enum class Platform {
			Windows = 0,
			Android
		};

		//Asset compiler
		class Compiler {
		private:

			//Platform specific
			std::filesystem::path output_dir;
			Platform platform;

			//Paths
			std::filesystem::path desc_path;

			//Extensions
			std::string desc_ext;

			//Shipped asset tolerence
			const uint64_t TOLERANCE_MS = 2000;

			//Verify directory
			bool verifyDirectory(std::filesystem::path const& dest) const;

			//Copy asset
			bool copyFile(std::filesystem::path const& copy, std::filesystem::path const& dest) const;

			//Generate default asset settings
			nlohmann::json generateDefaultCompileSettings(Type const& type) const;

			//Create default desc file
			Descriptor createDefaultDesc(Info const& asset, std::filesystem::path const& desc_path) const;

			//Read desc file
			Descriptor readDescFile(Info const& asset, std::filesystem::path const& desc_path) const;

			//Save desc file
			bool saveDescFile(Descriptor const& desc_file, std::filesystem::path const& desc_path) const;

		public:

			//Default compiler
			Compiler(std::filesystem::path const& desc_path) : desc_path{ desc_path }, desc_ext{ Assets::descriptor_ext } {
#ifdef PN_PLATFORM_WINDOWS
				output_dir = std::string(PAIN_ASSETS_OUTPUT_DIR);

#ifdef PAIN_ASSET_DEBUG
				output_dir = output_dir / "Debug" / "Assets" / "Raw";
#else
				output_dir = output_dir / "Release" / "Assets" / "Raw";
#endif
				platform = Platform::Windows;
#elif defined(PN_PLATFORM_ANDROID)
				output_dir = std::string(PAIN_ASSETS_OUTPUT_DIR);
				platform = Platform::Android;
#endif
			}
			~Compiler() = default;

			//Public process asset
			void processAsset(Info& asset_info);
		};
	}
}


#endif
