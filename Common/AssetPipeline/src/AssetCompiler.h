#pragma once

#ifndef ASSET_COMPILER_HPP
#define ASSET_COMPILER_HPP

#include "AssetData.h"
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
			nlohmann::json generateDefaultCompileSettings(Type const& type, Info const& asset) const;

			//Read desc file
			Descriptor readDescFile(Info& asset, std::filesystem::path const& path) const;

			//Create default desc file
			Descriptor createDefaultDesc(Info& asset, std::filesystem::path const& path) const;

			//Compile and ship
			void compileAndShip(Descriptor& desc_file, Info& asset_info, std::vector<IAsset>& opt_assets);

			//Internal asset compilers
			void compileTexture(Descriptor& desc_file, Info& asset_info) const;
			void compileAudio(Descriptor& desc_file, Info& asset_info) const;
			void compileModel(Descriptor& desc_file, Info& asset_info, std::vector<IAsset>& opt_assets);
			void compileScene(Descriptor& desc_file, Info& asset_info, std::vector<IAsset>& opt_assets);
			std::string GetCuttlefishExecutable() const;
			bool CuttlefishCompressor(unsigned char* pixels, int width, int height, int channels, const std::string& output_path, const std::string& format, const nlohmann::json& settings) const;
			bool CuttlefishCompressor(float* pixels, int width, int height, int channels, const std::string& output_path, const std::string& format, const nlohmann::json& settings) const;
			bool CuttlefishCubemapCompressor(const std::array<std::vector<float>, 6>& face_pixels, int face_size, int channels, const std::string& output_path, const std::string& format, const nlohmann::json& settings) const;
			std::string GetFFMPEGExecutable() const;
			void ExportModel(const Model& asset, const std::filesystem::path& out_path) const;

			//Recompilation check
			bool needsRecompilation(Info const& asset_info, Descriptor const& desc_file) const;

			//Verify import settings
			bool verifyCompileSettings(Type const& type, nlohmann::json const& settings) const;
		public:

			//Default compiler
			Compiler(std::filesystem::path const& input_path, std::filesystem::path const& output_path, Platform const& platform, std::filesystem::path const& exec_path) : assets_root{ input_path }, output_dir{ output_path }, platform{ platform }, exec_path{ exec_path }, desc_ext{ Assets::descriptor_ext } {
			}
			~Compiler() = default;

			//Public export material
			bool ExportMaterial(Material const& asset, std::filesystem::path const& out_path) const;

			//Public process asset
			std::optional<std::vector<IAsset>> processAsset(Info& asset_info);

			//Read desc file overload, read only
			Descriptor readDescFile(std::filesystem::path const& path) const;

			//Save desc file
			bool saveDescFile(Descriptor const& desc_file, std::filesystem::path const& path) const;
		};
	}
}


#endif

