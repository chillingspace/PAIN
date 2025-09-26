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
			TEXTURE,
			AUDIO,
			SHADER,

		};

		class IAssetCompiler {
		public:
			virtual ~IAssetCompiler() = default;
			virtual void compile(const std::string& input_path, const std::string& output_path) = 0;
			json readDescFile(const std::string& input_path)
			{
				// Read descriptor JSON
				std::ifstream input_file(input_path);
				if (!input_file.good()) {
					PN_CORE_WARN("[ShaderCompiler] Cannot open descriptor: {}\n,  desc_path << ");
					return json{};
				}

				// To read as a json file
				json json_file;
				input_file >> json_file;

				return json_file;
			}
		};

		class TextureCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& input_path, const std::string& output_path) override;
		};

		class ShaderCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& input_path, const std::string& output_path) override;
		};

		class AudioCompiler : public IAssetCompiler {
		public:
			void compile(const std::string& input_path, const std::string& output_path) override;
		};

		class Service : public AppSystem {
		public:
			Service() = default;

			void compileAsset(ASSET_TYPE type, const std::string& input_path, const std::string& output_path);

#define PN_PATH_SERVICE  services->get<Path::Service>()


			void onAttach() override;
			void onUpdate(float dt) override;
			void onDetach() override;

			void onEvent(Event::Event& e) override;

		private:
			std::unordered_map<ASSET_TYPE, std::unique_ptr<IAssetCompiler>> compilers;
			std::string asset_path;

			// Pointers to the compilers
			std::unique_ptr<TextureCompiler> texture_compiler;
			std::unique_ptr<ShaderCompiler> shader_compiler;
			//std::unique_ptr<AudioCompiler> audio_compiler;
		};

	}
}


#endif
#endif