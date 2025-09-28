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
			// Read descriptor JSON
			json data = readDescFile(desc_path);

			// Validate required fields
			if (!data.contains("asset_id") || !data.contains("source_file")) {
				PN_CORE_WARN("[TextureCompiler] Descriptor missing 'asset_id' or 'source_file': {}", desc_path);
				return;
			}

			std::string asset_id = data["asset_id"].get<std::string>();
			std::string source_file = data["source_file"].get<std::string>();

			// Options with default values
			// ASTC format for Android
			std::string android_format = "ASTC_4x4";  
			// BC7 format for Windows
			std::string windows_format = "BC7";       
			bool mipmaps = true;
			bool srgb = false;
			int quality_level = 128; // Quality setting
			std::filesystem::path android_output_dir = "assets/Textures/Compiled_Textures_Android";
			std::filesystem::path windows_output_dir = "assets/Textures/Compiled_Textures_Windows";

			// Override defaults with values from desc file
			if (data.contains("options")) {
				auto const& options = data["options"];
				if (options.contains("android_format")) android_format = options["android_format"].get<std::string>();
				if (options.contains("windows_format")) windows_format = options["windows_format"].get<std::string>();
				if (options.contains("mipmaps")) mipmaps = options["mipmaps"].get<bool>();
				if (options.contains("srgb")) srgb = options["srgb"].get<bool>();
				if (options.contains("quality")) quality_level = options["quality"].get<int>();
				if (options.contains("output_dir_android")) android_output_dir = options["output_dir_android"].get<std::string>();
				if (options.contains("output_dir_windows")) windows_output_dir = options["output_dir_windows"].get<std::string>();
			}

			// Resolve paths
			// ARM ASTC encoder
			std::filesystem::path astcenc_tool = "assets/Engine/Tools/astcenc-avx2.exe";  
			// DirectX texture converter
			std::filesystem::path texconv_tool = "assets/Engine/Tools/texconv.exe";  
			std::filesystem::path input_path = "assets/Textures/" + source_file;

			// Create output directories
			std::filesystem::create_directories(android_output_dir);
			std::filesystem::create_directories(windows_output_dir);

			// Output paths for both platforms
			std::filesystem::path android_output = android_output_dir / (asset_id + ".astc");
			std::filesystem::path windows_output = windows_output_dir / (asset_id + ".dds");

			// Check if tools exist
			if (!std::filesystem::exists(astcenc_tool)) {
				PN_CORE_WARN("[TextureCompiler] Cannot find ASTC encoder: {}", astcenc_tool.string());
				return;
			}

			if (!std::filesystem::exists(texconv_tool)) {
				PN_CORE_WARN("[TextureCompiler] Cannot find texconv: {}", texconv_tool.string());
				return;
			}

			// Check input texture
			if (!std::filesystem::exists(input_path)) {
				PN_CORE_WARN("[TextureCompiler] Cannot find source texture: {}", input_path.string());
				return;
			}

			// Build Android ASTC command
			auto buildAndroidCommand = [&]() {
				std::ostringstream cmd;

				// Start with the executable in quotes
				cmd << std::filesystem::absolute(astcenc_tool).string() << " ";

				// Color profile mode
				if (srgb) {
					cmd << " -cs"; // LDR sRGB color profile
				}
				else {
					cmd << " -cl"; // LDR linear color profile
				}

				// Input file (quoted)
				cmd << " \"" << std::filesystem::absolute(input_path).string() << "\"";

				// Output file (quoted)
				cmd << " \"" << std::filesystem::absolute(android_output).string() << "\"";

				// Block size (comes after input/output)
				std::string block_size = "4x4"; // default
				if (android_format == "ASTC_4x4") block_size = "4x4";
				else if (android_format == "ASTC_6x6") block_size = "6x6";
				else if (android_format == "ASTC_8x8") block_size = "8x8";
				else if (android_format == "ASTC_5x5") block_size = "5x5";
				else if (android_format == "ASTC_10x10") block_size = "10x10";

				cmd << " " << block_size;

				// Quality preset based on quality_level
				if (quality_level >= 200) cmd << " -exhaustive";      // highest quality
				else if (quality_level >= 150) cmd << " -thorough";   // high quality
				else if (quality_level >= 100) cmd << " -medium";     // balanced
				else if (quality_level >= 50) cmd << " -fast";        // fast
				else cmd << " -fastest";                               // fastest

				// Additional flags for better output
				cmd << " -silent";

				return cmd.str();
				};

			// Build Windows BC7 command  
			auto buildWindowsCommand = [&]() {
				std::ostringstream cmd;
				cmd << "\"" << std::filesystem::absolute(texconv_tool).string() << " ";
				// overwrite existing
				cmd << "-y ";  

				// Format selection
				if (windows_format == "BC7") cmd << "-f BC7_UNORM ";
				else if (windows_format == "BC3") cmd << "-f BC3_UNORM ";
				else if (windows_format == "BC1") cmd << "-f BC1_UNORM ";
				// default
				else cmd << "-f BC7_UNORM "; 

				// Mipmaps
				if (mipmaps) {
					// generate full mipmap chain
					cmd << "-m 0 "; 
				}
				else {
					// only base level
					cmd << "-m 1 "; 
				}

				// sRGB handling
				if (srgb) {
					cmd << "-srgb ";
				}

				// Output directory and input file
				cmd << "-o \"" << std::filesystem::absolute(windows_output_dir).string() << "\" ";
				cmd << "\"" << std::filesystem::absolute(input_path).string() << "\"";

				return cmd.str();
				};

			// Compile for Android (ASTC)
			PN_CORE_INFO("[TextureCompiler] Compiling ASTC texture for Android: {}", asset_id);
			std::string android_cmd = buildAndroidCommand();
			if (std::system(android_cmd.c_str()) != 0) {
				PN_CORE_ERROR("[TextureCompiler] Android ASTC compilation failed: {}", android_cmd);
				return;
			}
			PN_CORE_INFO("[TextureCompiler] Android compilation successful: {}", android_output.string());

			// Compile for Windows (BC7/DDS)
			PN_CORE_INFO("[TextureCompiler] Compiling BC texture for Windows: {}", asset_id);
			std::string windows_cmd = buildWindowsCommand();
			if (std::system(windows_cmd.c_str()) != 0) {
				PN_CORE_ERROR("[TextureCompiler] Windows BC compilation failed: {}", windows_cmd);
				return;
			}
			PN_CORE_INFO("[TextureCompiler] Windows compilation successful: {}", windows_output.string());

			PN_CORE_INFO("[TextureCompiler] Cross-platform texture compilation completed: {}", asset_id);
		}

		// Helper function to get the appropriate texture file for current platform
//		std::filesystem::path TextureCompiler::getPlatformTexture(const std::string& asset_id)
//		{
//#ifdef PN_PLATFORM_ANDROID
//			return std::filesystem::path("assets/Textures/Compiled_Textures_Android") / (asset_id + ".astc");
//#elif defined(PN_PLATFORM_WINDOWS)  
//			return std::filesystem::path("assets/Textures/Compiled_Textures_Windows") / (asset_id + ".dds");
//#else
//			// Fallback to Android version
//			return std::filesystem::path("assets/Textures/Compiled_Textures_Android") / (asset_id + ".astc");
//#endif
//		}



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
			// For now have to set it like this, using path service here will crash somehow...if i put IassetCompiler
			// To inherit AppSystem also feels wrong...
			std::filesystem::path tool_path = "assets/Engine/Tools/glslangValidator.exe";
			std::filesystem::path vert_path = "assets/Engine/Shaders/" + vert_file;
			std::filesystem::path frag_path = "assets/Engine/Shaders/" + frag_file;

			// Output directory
			std::filesystem::path output_dir = "assets/Engine/Shaders/Compiled_Shaders";
			std::filesystem::create_directories(output_dir);

			// Output file paths based on asset ID
			std::filesystem::path vert_output = output_dir / (asset_id + "_vert.spv");
			std::filesystem::path frag_output = output_dir / (asset_id + "_frag.spv");

			// Check if tool exists
			if (!std::filesystem::exists(tool_path)) {
				PN_CORE_WARN("[ShaderCompiler] Cannot find glslangValidator: {}", tool_path.string());
				return;
			}

			// Check if very and frag files exists
			if (!std::filesystem::exists(vert_path) && !std::filesystem::exists(frag_path)) {
				PN_CORE_WARN("[ShaderCompiler] Cannot find vertex or fragment shader: {} / {}", vert_path.string(), frag_path.string());
				return;
			}

			// To build the sys command to execute the compiler exe, have to be aboslute
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

		void Service::pushFileEvent(std::function<void()> callback)
		{
			std::lock_guard<std::mutex> lock(file_event_mutex);
			file_event_queue.push(std::move(callback));
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

			for (auto const& [alias, path] : PN_PATH_SERVICE->getAllRegisteredVirtualPaths())
			{
				if (alias.find("Game_Assets:/") != 0) continue;

				// Init compilation
				scanAssetDirectory(alias, true);

				// Watch dir for changes
				if (PN_PATH_SERVICE->getAllDirWatchers().count(path)) continue;

				PN_PATH_SERVICE->watchDirectoryTree("Game_Assets:/",
					[this](const std::filesystem::path& file, filewatch::Event event) {

						if (std::filesystem::is_directory(file)) return;
						// Queue file processing
						switch (event) {
						case filewatch::Event::added:
							pushFileEvent([file, event, this]() {
								processAssetFile(file);
								});
							break;
						case filewatch::Event::modified:
							pushFileEvent([file, event, this]() {
								processAssetFile(file);
								});
							break;
						case filewatch::Event::removed:
							//pushFileEvent([file, event, this]() {
							//	processAssetFile(file);
							//	});
							break;
						default:
							break;
						}

					});

				PN_CORE_INFO("[AssetCompilerService] Watching directory: {}", path.string());
			}

		}

		void Service::onUpdate(float dt)
		{
			//Update with file change events
			while (!file_event_queue.empty()) {

				try {
					//Call callback function if valid
					if (file_event_queue.front()) {
						PN_CORE_INFO("Executing file event callback with file watcher.");
						//Execute file event callback
						file_event_queue.front()();
					}

					//Pop from queue
					file_event_queue.pop();
				}
				catch (std::exception const&) {
					PN_CORE_WARN("Invalid Callback From FileWatcher Handled. Loop Continues.");
				}
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

			// Keep track of processed shader base names to avoid double compilation
			std::unordered_set<std::string> processed_shaders;

			auto compileFile = [&](const std::filesystem::path& file_path)
				{
					// Only process valid extensions
					if (valid_extensions.find(file_path.extension().string()) == valid_extensions.end()) {
						//PN_CORE_WARN("[AssetCompilerService] Invalid extension: {}", file_path.string());
						return;
					}

					// For shaders: skip if base name already compiled
					// Only for shaders
					if (file_path.extension() == ".vert" || file_path.extension() == ".frag") {
						// Removes extension
						std::string base_name = file_path.stem().string();
						if (processed_shaders.find(base_name) != processed_shaders.end()) {
							// already compiled this shader, as alr taken in from the desc file
							return;
						}
						processed_shaders.insert(base_name);
					}

					// Process asset normally
					processAssetFile(file_path);
				};

			if (!b_directory_tree) {
				for (const auto& file : std::filesystem::directory_iterator(root_path)) {
					if (!file.is_regular_file()) continue;
					compileFile(file.path());
				}
				return;
			}

			for (const auto& file : std::filesystem::recursive_directory_iterator(root_path)) {
				if (!file.is_regular_file()) continue;
				compileFile(file.path());
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