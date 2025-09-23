/*****************************************************************//**
 * \file   sAssetCompiler.cpp
 * \brief  Definition of asset compiler service
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#include "pch.h"
#include "Applications/AppSystem.h"
#include "sAssetCompiler.h"

namespace PAIN {
	namespace ASSET_COMPILER {

		void TextureCompiler::compile(const std::string& input_path, const std::string& output_path)
		{

		}

		void AudioCompiler::compile(const std::string& input_path, const std::string& output_path)
		{
		}

		void ShaderCompiler::compile(const std::string& input_path, const std::string& output_path)
		{
			std::ifstream src(input_path);
			if (!src.good()) {
				PN_CORE_WARN("[ShaderCompiler] Missing shader file {}\n", input_path);
				return;
			}

			// Read shader source
			std::string source((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());

			// TODO: optionally run shaderc/glslangValidator to pre-compile to SPIR-V
			// For now: just store raw GLSL
			std::ofstream dst(output_path, std::ios::binary);
			dst.write(source.data(), source.size());

			PN_CORE_INFO("[ShaderCompiler] Compiled shader to {}\n", output_path);
		}

		void Service::onAttach() {
			PN_CORE_INFO("Asset Compiler init");

			// Init specific asset compilers
			compilers[ASSET_TYPE::TEXTURE] = std::make_unique<TextureCompiler>();
			compilers[ASSET_TYPE::SHADER] = std::make_unique<ShaderCompiler>();

			// Setting assets path
			asset_path = "assets/"
		}

		void Service::onUpdate()
		{
			// TODO: COMPILE ALL ASSETS
		}

		void Service::onDetach() {
			
		}

		void Service::onEvent(Event::Event& e) {

		}

		void Service::compileAsset(ASSET_TYPE type, const std::string& input_path, const std::string& output_path)
		{
			auto it = compilers.find(type);
			if (it != compilers.end()) {
                std::ifstream file(input_path, std::ios::binary);
                if (!file.good()) {
                    PN_CORE_WARN("[AssetCompilerService] Input file does not exist: \n");
                    return;
                }
				it->second->compile(input_path, output_path);
			}
			else {
				// Log: No compiler registered for this asset type
                PN_CORE_WARN("[AssetCompilerService] Compiler not registered \n");
			}
		}



	}
}