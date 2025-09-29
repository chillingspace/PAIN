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

        void ShaderCompiler::compile(const std::string& desc_path, const std::string& output_path) {
            // Read descriptor JSON
            std::ifstream input_file(desc_path);
            if (!input_file.good()) {
                PN_CORE_WARN("[ShaderCompiler] Cannot open descriptor: {}\n,  desc_path << ");
                return;
            }

            // To read as a json file
            json json_file;
            input_file >> json_file;

            // Get current working directory and build paths from there
            std::filesystem::path current_path = std::filesystem::current_path();
            std::filesystem::path project_root = current_path / "PAIN"; // Adjust as needed

            // Or try to find the project root by looking for a marker file
            std::filesystem::path search_path = current_path;
            while (search_path.has_parent_path()) {
                if (std::filesystem::exists(search_path / "PAIN" / "assets")) {
                    project_root = search_path / "PAIN";
                    break;
                }
                search_path = search_path.parent_path();
            }

            // Slowly, find the file paths
            std::filesystem::path tool_path= project_root / "assets" / "Engine" / "Tools" / "glslangValidator.exe";
            std::filesystem::path input_path = project_root / "assets" / "Engine" / "Shaders" / "base.vert";
            std::filesystem::path output_dir = project_root / "assets" / "Engine" / "Shaders" / "Compiled_Shaders";
            std::filesystem::path output_file = output_dir / ("base.vert.spv");

            std::ifstream check_file(tool_path);
            if (!check_file.good()) {
                PN_CORE_WARN("[ShaderCompiler] Cannot find exe: {}\n", tool_path.string());
                return;
            }

            // Ensure output folder exists
            std::filesystem::create_directories(output_dir);

            auto norm = [](std::filesystem::path p) {
                std::string s = p.string();
                std::replace(s.begin(), s.end(), '/', '\\'); 
                return s;
                };

            // Build command
            #ifdef PN_PLATFORM_WINDOWS
            std::ostringstream cmd;
            // No quotes around executable
            cmd << tool_path.string() << " "  
                << "-G \"" << input_path.string() << "\" "
                << "-o \"" << output_file.string() << "\"";
            #else
            std::ostringstream cmd;
            cmd << "\"" << tool_path.string() << "\" "
                << "-V -G \"" << input_path.string() << "\" "
                << "-o \"" << output_file.string() << "\"";
            #endif

            // Run command
            int result = std::system(cmd.str().c_str());
            if (result != 0) {
                PN_CORE_ERROR("[ShaderCompiler] Shader compilation failed: {}", cmd.str());
            }
            else {
                PN_CORE_INFO("[ShaderCompiler] Compiled successfully: {}", output_file.string());
            }

            //// Compile fragment shader
            //#ifdef PN_PLATFORM_WINDOWS
            //std::string cmd_frag = "\"" + validator + "\" -V \"" + frag_path + "\" -o \"" + frag_bin + "\"";
            //#else
            //std::string cmd_frag = "\"" + validator + "\" -V -G \"" + frag_path + "\" -o \"" + frag_bin + "\"";
            //#endif
            //PN_CORE_TRACE("[ShaderCompiler] Running: {}\n", cmd_frag);
            //int res_frag = std::system(cmd_frag.c_str());
            //if (res_frag != 0) {
            //    PN_CORE_WARN("[ShaderCompiler] Fragment shader compilation failed!\n");
            //    return;
            //}

            //PN_CORE_TRACE("[ShaderCompiler] Compiled shaders to: {}", vert_bin, " & {}",frag_bin, "\n");
        }
    


		void Service::onAttach() {
			PN_CORE_INFO("Asset Compiler init");

			// Init specific asset compilers
			compilers[ASSET_TYPE::TEXTURE] = std::make_unique<TextureCompiler>();
			compilers[ASSET_TYPE::SHADER] = std::make_unique<ShaderCompiler>();

			// Setting assets path
            asset_path = "assets/";

            // TODO: COMPILE ALL ASSETS
            compileAsset(ASSET_TYPE::SHADER, "assets/Engine/Shaders/base.desc", "assets/Engine/Shaders/Compiled Shaders/");
            
		}

		void Service::onUpdate()
		{

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