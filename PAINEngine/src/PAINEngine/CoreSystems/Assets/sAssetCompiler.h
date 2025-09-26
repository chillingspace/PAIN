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

//#ifdef PN_PLATFORM_ANDROID
//#include <android/asset_manager.h>
//#include <android/asset_manager_jni.h>
//#endif

namespace PAIN {
	namespace Compiler {

		enum class ASSET_TYPE {
			Texture,
			Audio,
			Shader,
			None
		};

		class IAssetCompiler : public AppSystem {
		public:
			virtual ~IAssetCompiler() = default;
			virtual void compile(const std::string& desc_path) = 0;

			//Optional virtual functions
			void onAttach() override {};
			void onDetach() override {};
			void onUpdate(float dt) override {};

			//Event handler for app layer
			void onEvent(Event::Event& e) override {};

#define PN_PATH_SERVICE  services->get<Path::Service>()
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
		};

		class TextureCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class ShaderCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class AudioCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& desc_path) override;
		};

		class Service : public AppSystem {
		public:
			Service() = default;

			void scanAssetDirectory(std::string const& virtual_path, bool b_diretory_tree);

			void compileAsset(ASSET_TYPE type, const std::string& desc_file_path);

			// Helper function
			std::string getIDFromPath(std::string const& path);
			std::string typeToString(ASSET_TYPE type) const;
			ASSET_TYPE getAssetType(std::filesystem::path const& path) const;

			void processAssetFile(std::filesystem::path const& file_path);
			void addValidExtensions(std::string const& ext);


			void onAttach() override;
			void onUpdate(float dt) override;
			void onDetach() override;

			void onEvent(Event::Event& e) override;

#define PN_PATH_SERVICE  services->get<Path::Service>()

		private:
			std::unordered_map<ASSET_TYPE, std::unique_ptr<IAssetCompiler>> compilers;

			//List of valid extension
			std::set<std::string> valid_extensions;

			// Pointers to the compilers
			std::unique_ptr<TextureCompiler> texture_compiler;
			std::unique_ptr<ShaderCompiler> shader_compiler;
			//std::unique_ptr<AudioCompiler> audio_compiler;
		};

	}
}


#endif
#endif