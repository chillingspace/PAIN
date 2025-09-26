/*****************************************************************//**
 * \file   sAssetCompiler.cpp
 * \brief  Definition of asset compiler service
 *
 * \author Bryan Lim, 2301214, bryanlicheng.l@digipen.edu (100%)
 * \co-author
 * \date   September 2025
 * All content � 2025 DigiPen Institute of Technology Singapore, all rights reserved.
 *********************************************************************/

#ifdef PN_PLATFORM_WINDOWS

#include "pch.h"
#include "Applications/Application.h"
#include "sAssetCompiler.h"

#include "sPath.h"


namespace PAIN {
    namespace Compiler {

        void TextureCompiler::compile(const std::string& desc_path)
        {

        }

        void AudioCompiler::compile(const std::string& desc_path)
        {
        }

        void ShaderCompiler::compile(const std::string& desc_path)
        {
            // Read descriptor JSON
            json data = readDescFile(desc_path);

            // Get asset ID from descriptor
            if (!data.contains("asset_id")) {
                PN_CORE_WARN("[ShaderCompiler] Descriptor missing 'asset_id': {}", desc_path);
                return;
            }
            std::string asset_id = data["asset_id"].get<std::string>();

            // Get source files from descriptor
            if (!data.contains("source_files") ||
                !data["source_files"].contains("vertex") ||
                !data["source_files"].contains("fragment"))
            {
                PN_CORE_WARN("[ShaderCompiler] Descriptor missing vertex or fragment shader: {}", desc_path);
                return;
            }

            std::string vert_file = data["source_files"]["vertex"].get<std::string>();
            std::string frag_file = data["source_files"]["fragment"].get<std::string>();

            // Resolve paths using Path Service
            std::filesystem::path tool_path = "assets/Engine/Tools/glslangValidator.exe";
            std::filesystem::path vert_path = "assets/Engine/Shaders/" + vert_file;
            std::filesystem::path frag_path = "assets/Engine/Shaders/" + frag_file;

            // Output directory
            std::filesystem::path output_dir = "assets/Engine/Shaders/Compiled_Shaders";
            std::filesystem::create_directories(output_dir);

            // Output file paths based on asset ID
            std::filesystem::path vert_output = output_dir / (asset_id + "_vert.spv");
            std::filesystem::path frag_output = output_dir / (asset_id + "_frag.spv");

            //// Check if tool exists
            if (!std::filesystem::exists(tool_path)) {
                PN_CORE_WARN("[ShaderCompiler] Cannot find glslangValidator: {}", tool_path.string());
                return;
            }

            auto buildCommand = [](const std::filesystem::path& tool, const std::filesystem::path& input, const std::filesystem::path& output) {
                std::ostringstream cmd;
#ifdef PN_PLATFORM_WINDOWS
                cmd << "\"" << std::filesystem::absolute(tool).string() << " "
                    << "-G \"" << std::filesystem::absolute(input).string() << "\" "
                    << "-o \"" << std::filesystem::absolute(output).string() << "\"";
#endif
                return cmd.str();
                };

            // Compile vertex shader
            std::string vert_cmd = buildCommand(tool_path, vert_path, vert_output);
            if (std::system(vert_cmd.c_str()) != 0) {
                PN_CORE_ERROR("[ShaderCompiler] Vertex shader compilation failed: {}", vert_cmd);
                return;
            }

            // Compile fragment shader
            std::string frag_cmd = buildCommand(tool_path, frag_path, frag_output);
            if (std::system(frag_cmd.c_str()) != 0) {
                PN_CORE_ERROR("[ShaderCompiler] Fragment shader compilation failed: {}", frag_cmd);
                return;
            }

            PN_CORE_INFO("[ShaderCompiler] Compiled shaders successfully: {} & {}", vert_output.string(), frag_output.string());
        }


    


		void Service::onAttach() {
			PN_CORE_INFO("Asset Compiler init");

			// Init specific asset compilers
			compilers[ASSET_TYPE::Texture] = std::make_unique<TextureCompiler>();
			compilers[ASSET_TYPE::Shader] = std::make_unique<ShaderCompiler>();

            //Texture extensions
            addValidExtensions(".png");
            addValidExtensions(".jpg");
            addValidExtensions(".jpeg");
            addValidExtensions(".tex");

            ////Font extension
            //addValidExtensions(".ttf");
              
            // Shader extension
            addValidExtensions(".frag");
            addValidExtensions(".vert");

            //Audio extension
            addValidExtensions(".wav");

            ////Video extension
            //addValidExtensions(".mpg");

            ////Other extension
            //addValidExtensions(".prefab");
            //addValidExtensions(".scn");
            //addValidExtensions(".grid");
            //addValidExtensions(".lua");
            //addValidExtensions(".json");

            // TODO: COMPILE ALL ASSETS            
		}

        void Service::onUpdate(float dt)
        {
            for (auto const& [alias, path] : PN_PATH_SERVICE->getAllRegisteredVirtualPaths())
            {
                if (alias.find("Game_Assets:/") != 0) continue;

                // Initial compilation
                scanAssetDirectory(alias, true);

                //// Watch directory if not already watched
                //if (PN_PATH_SERVICE->getAllDirWatchers().count(path)) continue;

                //PN_PATH_SERVICE->watchDirectoryTree(alias, [this](std::filesystem::path const& changed_file, filewatch::Event event_type)
                //    {
                //        if (event_type == filewatch::Event::modified || event_type == filewatch::Event::added) {
                //            processAssetFile(changed_file);
                //        }
                //    });

                //PN_CORE_INFO("[AssetCompilerService] Watching directory: {}", path.string());
            }
        }



		void Service::onDetach() {
			
		}

		void Service::onEvent(Event::Event& e) {

		}

        std::string Service::typeToString(ASSET_TYPE type) const {
            switch (type) {
            case ASSET_TYPE::Texture:
                return "Texture";
                break;
            case ASSET_TYPE::Audio:
                return "Music";
                break;
            case ASSET_TYPE::Shader:
                return "Shader";
                break;
            //case Types::Scene:
            //    return "Scene";
            //    break;
            //case Types::Prefab:
            //    return "Prefab";
            //    break;
            //case Types::Grid:
            //    return "Grid";
            //    break;
            //case Types::Script:
            //    return "Script";
            //    break;
            //case Types::Video:
            //    return "Video";
            //    break;
            default:
                return "Unknown";
                break;
            }
        }

        Compiler::ASSET_TYPE Service::getAssetType(std::filesystem::path const& path) const {
            auto ext = path.extension().string();
            // constexpr size_t music_threshold = 5 * 1024 * 1024; // 5 MB
            if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tex") {
                return Compiler::ASSET_TYPE::Texture;
            }
            else if (ext == ".frag" || ext == ".vert") {
                return Compiler::ASSET_TYPE::Shader;
            }
            //else if (ext == ".ttf") {
            //    return Assets::Types::Font;
            //}
            else if (ext == ".wav") {
                return Compiler::ASSET_TYPE::Audio;
            }
            //else if (ext == ".scn") {
            //    return Assets::Types::Scene;
            //}
            //else if (ext == ".prefab") {
            //    return Assets::Types::Prefab;
            //}
            //else if (ext == ".grid") {
            //    return Assets::Types::Grid;
            //}
            //else if (ext == ".lua") {
            //    return Assets::Types::Script;
            //}
            //else if (ext == ".mpg") {
            //    return Assets::Types::Video;
            //}
            else {
                return Compiler::ASSET_TYPE::None;
            }
        }

        void Service::processAssetFile(std::filesystem::path const& file_path)
        {
            if (!file_path.has_extension()) return;

            // Extension check
            if (valid_extensions.find(file_path.extension().string()) == valid_extensions.end()) {
                // PN_CORE_WARN("[AssetCompilerService] Invalid extension: {}", file_path.string());
                return;
            }

            // Get asset type
            ASSET_TYPE type = getAssetType(file_path);

            std::string virtual_path = PN_PATH_SERVICE->convertToVirtualPath("Game_Assets:/", file_path.string());

            // Build asset id
            std::string asset_id = getIDFromPath(virtual_path);
            size_t dot_pos = asset_id.find('.');
            std::string asset_name = (dot_pos != std::string::npos) ? asset_id.substr(0, dot_pos) : asset_id;

            // Build descriptor file path            
            std::filesystem::path desc_file = file_path.parent_path() / (asset_name + ".desc");

            // Compile (if desc exists, you could check here)
            compileAsset(type, desc_file.string());
        }
        

        void Service::addValidExtensions(std::string const& ext)
        {
            valid_extensions.insert(ext);
        }

        void Service::scanAssetDirectory(std::string const& virtual_path, bool b_directory_tree)
        {
            auto root_path = PN_PATH_SERVICE->resolvePath(virtual_path);

            if (!b_directory_tree) {
                for (const auto& file : std::filesystem::directory_iterator(root_path)) {
                    if (!file.is_regular_file()) continue;
                    processAssetFile(file.path());
                }
                return;
            }

            for (const auto& file : std::filesystem::recursive_directory_iterator(root_path)) {
                if (!file.is_regular_file()) continue;
                processAssetFile(file.path());
            }
        }


        void Service::compileAsset(ASSET_TYPE type, const std::string& desc_file_path)
		{
			auto it = compilers.find(type);
			if (it != compilers.end()) {
                std::ifstream file(desc_file_path, std::ios::binary);
                if (!file.good()) {
                    PN_CORE_WARN("[AssetCompilerService] Input file does not exist: {}\n", desc_file_path);
                    return;
                }
				it->second->compile(desc_file_path);
			}
			else {
				// Log: No compiler registered for this asset type
                // PN_CORE_WARN("[AssetCompilerService] Compiler not registered", typeToString(type));
                return;
			}
		}

        std::string Service::getIDFromPath(std::string const& path)
        {
            // Using virtual paths
            auto actual_path = PN_PATH_SERVICE->normalizePath(PN_PATH_SERVICE->resolvePath(path)).string();
            //string variables
            size_t start = actual_path.find_last_of('\\') + 1;
            //size_t size = actual_path.find_first_of('.', start) - start;
            std::string asset_id = actual_path.substr(start);

            return asset_id;
        }



	}
}

#endif