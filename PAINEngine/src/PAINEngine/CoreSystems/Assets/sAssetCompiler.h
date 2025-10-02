/*****************************************************************//**
 * \file   sAssetCompiler.h
 * \brief  Declaration of asset compiler service
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#pragma once

#ifdef PN_PLATFORM_WINDOWS
#ifndef S_ASSET_COMPILER_H
#define S_ASSET_COMPILER_H

#include "Applications/AppSystem.h"
#include "Utility/guid.h"
//#include <assimp/include/assimp/Importer.hpp>
//#include <assimp/include/assimp/scene.h>
//#include <assimp/include/assimp/postprocess.h>

//#ifdef PN_PLATFORM_ANDROID
//#include <android/asset_manager.h>
//#include <android/asset_manager_jni.h>
//#endif

namespace PAIN {
	namespace Compiler {

		enum class COMPILE_ASSET_TYPE {
			Texture,
			Object,
			Shader,
			None
		};

		static json readDescFile(const std::string& input_path)
		{

			// Read descriptor JSON
			std::ifstream input_file(input_path);
			if (!input_file.is_open()) {
				PN_CORE_WARN("Cannot open descriptor: {}", input_path);
				return json{};
			}

			json json_file;
			try {
				input_file >> json_file;
			}
			catch (const nlohmann::json::exception& e) {
				PN_CORE_WARN("Failed to parse JSON: {}", e.what());
				return json{};
			}

			return json_file;
		}

		static std::string infer_usage_from_name(const std::string& stemLower) {
			// crude but effective: _n / _normal → normal map; _mask/_orm → masks; else color
			if (stemLower.find("_n") != std::string::npos || stemLower.find("normal") != std::string::npos)
				return "normal";
			if (stemLower.find("mask") != std::string::npos || stemLower.find("orm") != std::string::npos)
				return "mask";
			return "color";
		}

		static bool write_default_texture_desc(const std::filesystem::path& imagePath,
			const std::filesystem::path& descPath,
			std::string* outAssetId = nullptr)
		{
			try {
				const std::string asset_id = imagePath.stem().string(); // e.g. "heart"
				if (outAssetId) *outAssetId = asset_id;

				std::string stemLower = asset_id;
				std::transform(stemLower.begin(), stemLower.end(), stemLower.begin(), ::tolower);
				const std::string usage = infer_usage_from_name(stemLower);

				// defaults you already use
				const bool default_mips = true;
				const int  default_quality = 128;

				// usage → sensible defaults (srgb + target formats)
				bool srgb = (usage == "color");
				std::string android_fmt = "ASTC_6x6";
				std::string windows_fmt = "BC7";
				if (usage == "normal") { srgb = false; windows_fmt = "BC5"; android_fmt = "ASTC_6x6"; }
				if (usage == "mask") { srgb = false; windows_fmt = "BC7"; android_fmt = "ASTC_6x6"; }

				json j{
					{"version", 1},
					{"type", "texture"},
					{"guid", PAIN::Util::make_guid_v4()},
					{"asset_id", asset_id},
					{"source_file", imagePath.filename().string()},
					{"usage", usage},
					{"options", {
						{"android_format", android_fmt},
						{"windows_format", windows_fmt},
						{"quality", default_quality},
						{"mipmaps", default_mips},
						{"srgb", srgb},
						// keep your current defaults so it matches existing output layout
						{"output_dir_windows", "assets/Textures/Compiled_Textures_Windows"},
						{"output_dir_android", "assets/Textures/Compiled_Textures_Android"}
					}}
				};

				std::ofstream out(descPath, std::ios::binary);
				if (!out) return false;
				out << j.dump(4);
				return true;
			}
			catch (...) {
				return false;
			}
		}


		class IAssetCompiler{
		public:
			virtual ~IAssetCompiler() = default;
			virtual void compile(const std::string& desc_path) = 0;
		};

		class TextureCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class ShaderCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class ObjectMeshCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class Service : public AppSystem {
		public:
			~Service() = default;

			void scanAssetDirectory(std::string const& virtual_path, bool b_diretory_tree);

			void compileAsset(COMPILE_ASSET_TYPE type, const std::string& desc_file_path);

			// Helper function
			std::string getIDFromPath(std::string const& path);
			std::string typeToString(COMPILE_ASSET_TYPE type) const;
			COMPILE_ASSET_TYPE getAssetType(std::filesystem::path const& path) const;

			void processAssetFile(std::filesystem::path const& file_path);
			void addValidExtensions(std::string const& ext);

			//Thread safe insertion for file event queue
			void pushFileEvent(std::function<void()> callback);

			void onAttach() override;

			void onFixedUpdate(AppTiming timing) override;
			void onUpdate(AppTiming timing) override;
			void onDetach() override;

			void onEvent(Event::Event& e) override;

#define PN_PATH_SERVICE  services->get<Path::Service>()

		private:
			std::unordered_map<COMPILE_ASSET_TYPE, std::unique_ptr<IAssetCompiler>> compilers;

			//List of valid extension
			std::set<std::string> valid_extensions;

			//File Watching Queue
			std::queue<std::function<void()>> file_event_queue;

			//Mutex for thread safety
			std::mutex file_event_mutex;
		};

	}
}


#endif
#endif