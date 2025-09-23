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

#ifndef S_ASSET_COMPILER_H
#define S_ASSET_COMPILER_H

#include "Applications/AppSystem.h"

//#ifdef PN_PLATFORM_ANDROID
//#include <android/asset_manager.h>
//#include <android/asset_manager_jni.h>
//#endif

namespace PAIN {
	namespace ASSET_COMPILER {

		enum class ASSET_TYPE {
			TEXTURE,
			AUDIO,
			SHADER,

		};

		class IAssetCompiler {
		public:
			virtual ~IAssetCompiler() = default;
			virtual void compile(const std::string& input_path, const std::string& output_path) = 0;
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


			void onAttach() override;
			void onUpdate() override;
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