#pragma once

#ifndef ASSET_COMPILER_HPP
#define ASSET_COMPILER_HPP

#include "AssetTypes.h"

namespace PAIN {
	namespace Assets {

		//Asset compiler
		class Compiler {
		private:

			//Platform specific
			std::filesystem::path output_dir;
			Platform platform;

			//Paths
			std::filesystem::path assets_root;
			std::filesystem::path exec_path;

			//Extensions
			std::string desc_ext;

			//Shipped asset tolerence
			const uint64_t TOLERANCE_MS = 2000;

			//Get current time
			uint64_t getCurrentTimeStamp() const;

			//Verify directory
			bool verifyDirectory(std::filesystem::path const& dest) const;

			//Copy asset
			bool copyFile(std::filesystem::path const& copy, std::filesystem::path const& dest) const;

			//Generate default asset settings
			nlohmann::json generateDefaultCompileSettings(Type const& type) const;

			//Create default desc file
			Descriptor createDefaultDesc(Info const& asset, std::filesystem::path const& path) const;

			//Read desc file
			Descriptor readDescFile(Info const& asset, std::filesystem::path const& path) const;

			//Save desc file
			bool saveDescFile(Descriptor const& desc_file, std::filesystem::path const& path) const;

			//Compile and ship
			void compileAndShip(Descriptor const& desc_file, Info& asset_info) const;

			//Internal asset compilers
			void compileTexture(Descriptor const& desc_file, Info& asset_info) const;
			void compileAudio(Descriptor const& desc_file, Info& asset_info) const;
			void compileModel(Descriptor const& desc_file, Info& asset_info) const;
			std::string GetCuttlefishExecutable() const;
			bool CompressTextureDDS(unsigned char* pixels, int width, int height, int channels, const std::string& output_path, const std::string& format, const nlohmann::json& settings) const;
			std::string GetASTCEncoderExecutable() const;
			std::string ConvertToASTCBlockSize(const std::string& format) const;
			bool CompressTextureASTC(unsigned char* pixels, int width, int height, int channels, const std::string& output_path, const std::string& format, const nlohmann::json& settings) const;
		public:

			//Default compiler
			Compiler(std::filesystem::path const& input_path, std::filesystem::path const& output_path, Platform const& platform, std::filesystem::path const& exec_path) : assets_root{ input_path }, output_dir{ output_path }, platform{ platform }, exec_path{ exec_path }, desc_ext{ Assets::descriptor_ext } {
			}
			~Compiler() = default;

			//Public process asset
			void processAsset(Info& asset_info);

			//Verify import settings
			bool verifyCompileSettings(Type const& type, nlohmann::json const& settings) const;
		};
	}
}


#endif
