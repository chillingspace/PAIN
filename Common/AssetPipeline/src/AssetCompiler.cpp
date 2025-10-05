#include "AssetCompiler.h"

#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_image_write.h"


namespace PAIN {
	namespace Assets {

        uint64_t Compiler::getCurrentTimeStamp() const {
            //Get current timestamp in milliseconds since epoch
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto current_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            return current_timestamp;
        }

        bool Compiler::verifyDirectory(std::filesystem::path const& dest) const {

            //Check if directory exists
            if (!std::filesystem::exists(dest)) {

                //Create directory if it doesnt exist
                std::filesystem::path parent_dir = dest.parent_path();
                if (!parent_dir.empty() && !std::filesystem::exists(parent_dir)) {
                    if (!std::filesystem::create_directories(parent_dir)) {
                        std::cout << "Failed to create parent directory: " << parent_dir << std::endl;
                        return false;
                    }

                    std::cout << "Created directory: " << parent_dir << std::endl;
                    return true;
                }
            }

            return true;
        }

		bool Compiler::copyFile(std::filesystem::path const& copy, std::filesystem::path const& dest) const {
            try {

                //Verify directory
                if (!verifyDirectory(dest)) {
                    std::cout << "Directory doesnt exist: " << dest << std::endl;
                    return false;
                }

                //Use std::filesystem::copy_file with update_existing option
                std::filesystem::copy_options options =
                    std::filesystem::copy_options::update_existing;

                if (std::filesystem::copy_file(copy, dest, options)) {
                    std::cout << "File Copied From: " << copy << " To: " << dest << std::endl;
                    return true;
                }
                else {
                    std::cout << copy << " Reposition Failed." << std::endl;
                    return false;
                }
            }
            catch (const std::filesystem::filesystem_error& e) {
                std::cout << copy << " Reposition Failed." << e.what() << std::endl;
                return false;
            }
		}

        nlohmann::json Compiler::generateDefaultCompileSettings(Type const& type) const {
            nlohmann::json settings;

            switch (type) {
            case Type::Texture:
                settings["window_compression"] = "BC7";
                settings["android_compression"] = "ASTC_4x4";
                settings["generate_mipmaps"] = true;
                settings["max_size"] = 1024;
                settings["srgb"] = true;
                break;

            case Type::Audio:
                //settings["compression"] = "OGG";
                //settings["quality"] = 0.8;
                //settings["loop"] = false;
                break;

            case Type::Model:
                //settings["generate_lods"] = true;
                //settings["optimize_vertices"] = true;
                //settings["weld_threshold"] = 0.001;
                break;
            default:
                break;
            }

            return settings;
        }

        bool Compiler::verifyCompileSettings(Type const& type, nlohmann::json const& settings) const {

            try {

                bool checker = true;

                switch (type) {
                case Type::Texture:
                    checker =   (settings.contains("window_compression") &&
                                settings.contains("android_compression") &&
                                settings.contains("generate_mipmaps") &&
                                settings.contains("max_size") &&
                                settings.contains("srgb"));
                    break;

                case Type::Audio:
                    //checker =   (settings.contains("compression") &&
                    //            settings.contains("quality") &&
                    //            settings.contains("loop"));
                    break;

                case Type::Model:
                    //checker =   (settings.contains("generate_lods") &&
                    //            settings.contains("optimize_vertices") &&
                    //            settings.contains("weld_threshold"));
                    break;
                default:
                    break;
                }

                if (!checker) return false;
                return true;
            }
            catch (const std::exception& e) {
                return false;
            }

        }

        Descriptor Compiler::createDefaultDesc(Info const& asset, std::filesystem::path const& path) const {

            //Extract asset name from path
            std::string asset_name = asset.raw_path.stem().string();

            // Create default descriptor
            Descriptor desc;

            // Identity
            desc.guid = GUID::Generate();
            desc.descriptor_version = 1;
            desc.created_timestamp = getCurrentTimeStamp();

            // Asset classification
            desc.type = asset.type;
            desc.name = asset_name;

            // Import/processing settings
            desc.import_settings = generateDefaultCompileSettings(desc.type);

            // Build data
            desc.raw_last_modified = asset.raw_last_modified;

            // Dependencies
            desc.dependencies.clear();

            // Metadata (start empty, expandable)
            desc.meta_data = nlohmann::json::object();

            // Add some basic metadata based on asset type
            desc.meta_data["source_file"] = asset.raw_path;

            //Save desc file
            saveDescFile(desc, path);

            return desc;
        }

        Descriptor Compiler::readDescFile(Info const& asset, std::filesystem::path const& path) const {
            try {
                std::ifstream file(path);
                nlohmann::json desc_json;
                file >> desc_json;

                Descriptor desc;
                desc.descriptor_version = desc_json.value("descriptor_version", 1);
                desc.guid = GUID(desc_json["guid"].get<std::string>());
                desc.created_timestamp = desc_json.value("created_timestamp", 0ULL);

                //Asset info
                auto asset_info = desc_json["asset_info"];
                desc.type = stringToAssetType(asset_info["type"].get<std::string>());
                desc.name = asset_info.value("name", "");

                //Settings and build data
                desc.import_settings = desc_json.value("import_settings", nlohmann::json{});
                auto build_data = desc_json["build_data"];
                desc.raw_last_modified = build_data.value("raw_last_modified", 0ULL);

                //Dependencies
                auto deps_array = desc_json.value("dependencies", nlohmann::json::array());
                for (const auto& dep_str : deps_array) {
                    desc.dependencies.push_back(GUID(dep_str.get<std::string>()));
                }

                desc.meta_data = desc_json.value("metadata", nlohmann::json{});

                file.close();

                return desc;
            }
            catch (const std::exception& e) {
                std::cout << "Error encountered reading desc file, reverting to default." << std::endl;
                return createDefaultDesc(asset, path);
            }
        }

        bool Compiler::saveDescFile(Descriptor const& desc_file, std::filesystem::path const& path) const {
            //Save generated descriptor
            try {
                //Verify directory
                if (!verifyDirectory(path)) {
                    std::cout << "Directory doesnt exist: " << path << std::endl;
                    return false;
                }

                nlohmann::json desc_json;

                //Core identity
                desc_json["descriptor_version"] = desc_file.descriptor_version;
                desc_json["guid"] = desc_file.guid.ToString();
                desc_json["created_timestamp"] = desc_file.created_timestamp;

                // Asset info
                desc_json["asset_info"]["type"] = assetTypeToString(desc_file.type);
                desc_json["asset_info"]["name"] = desc_file.name;

                // Settings and build data
                desc_json["import_settings"] = desc_file.import_settings;
                desc_json["build_data"]["raw_last_modified"] = desc_file.raw_last_modified;

                // Dependencies and metadata
                nlohmann::json deps_array = nlohmann::json::array();
                for (const auto& dep : desc_file.dependencies) {
                    deps_array.push_back(dep.ToString());
                }
                desc_json["dependencies"] = deps_array;
                desc_json["meta_data"] = desc_file.meta_data;

                std::ofstream file(path, std::ios::out);
                if (file << desc_json.dump(2)) {
                    std::cout << "Descriptor file saved at: " << path << std::endl;
                    file.close();
                    return true;
                }
                std::cout << "Error saving default desc file to: " << path << std::endl;
                file.close();
                return false;
            }
            catch (const std::exception& e) {
                std::cout << "Error saving default desc file to: " << path << std::endl;
                return false;
            }
        }

        void Compiler::compileAndShip(Descriptor const& desc_file, Info& asset_info) const {

            //Find asset type and platform
            switch (desc_file.type) {
            case Type::Texture:
                compileTexture(desc_file, asset_info);
                break;

            case Type::Audio:
                compileAudio(desc_file, asset_info);
                break;

            case Type::Model:
                compileModel(desc_file, asset_info);
                break;
            default:
                break;
            }
        }

        void Compiler::compileTexture(Descriptor const& desc_file, Info& asset_info) const {

            //Determine output format and shipped path
            std::string output_extension;
            std::string compression_format;

            switch (platform) {
            case Platform::Windows:
                output_extension = ".dds";
                compression_format = desc_file.import_settings.value("compression", "BC7");
                asset_info.shipped_path = output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + output_extension);
                break;
            case Platform::Android:
                output_extension = ".astc";
                compression_format = desc_file.import_settings.value("compression", "ASTC_4x4");
                asset_info.shipped_path = output_dir / asset_info.relative_folder / (asset_info.raw_path.stem().string() + output_extension);
                break;
            default:
                std::cout << "ERROR: Unsupported platform for texture compilation" << std::endl;
                return;
            }

            //Load texture data using STB
            int width, height, channels;
            unsigned char* raw_pixels = stbi_load(asset_info.raw_path.string().c_str(),
                &width, &height, &channels, STBI_rgb_alpha);

            if (!raw_pixels) {
                std::cout << "ERROR: Failed to load texture: " << asset_info.raw_path
                    << " - " << stbi_failure_reason() << std::endl;
                return;
            }

            std::cout << "Loaded texture: " << width << "x" << height << " (" << channels << " channels)" << std::endl;

            //Apply import settings (resize if needed)
            int target_width = width;
            int target_height = height;
            int max_size = desc_file.import_settings.value("max_size", 2048);

            if (width > max_size || height > max_size) {
                float scale = static_cast<float>(max_size) / std::max(width, height);
                int target_width = static_cast<int>(width * scale);
                int target_height = static_cast<int>(height * scale);

                //Resize using STB
                unsigned char* resized_pixels = (unsigned char*)malloc(target_width * target_height * 4);

                // NEW API: stbir_resize (not stbir_resize_uint8)
                if (stbir_resize(raw_pixels, width, height, 0,           // input
                    resized_pixels, target_width, target_height, 0, // output  
                    STBIR_RGBA, STBIR_TYPE_UINT8, STBIR_EDGE_CLAMP, STBIR_FILTER_DEFAULT)) { 

                    stbi_image_free(raw_pixels);
                    raw_pixels = resized_pixels;
                    width = target_width;
                    height = target_height;
                    std::cout << "Resized texture to: " << width << "x" << height << std::endl;
                }
                else {
                    std::cout << "ERROR: STB resize failed!" << std::endl;
                    free(resized_pixels);
                }
            }

            //Verify output directory
            if (!verifyDirectory(asset_info.shipped_path)) {
                std::cout << "ERROR: Failed to create output directory: " << asset_info.shipped_path.parent_path() << std::endl;
                stbi_image_free(raw_pixels);
                return;
            }

            //Compress assets
            bool compression_success = false;

            if (platform == Platform::Windows) {
                // Use Cuttlefish for DDS/BC7 compression
                compression_success = CompressTextureDDS(raw_pixels, width, height, 4,
                    asset_info.shipped_path.string(),
                    compression_format, desc_file.import_settings);
            }
            else if (platform == Platform::Android) {
                // Use Cuttlefish for ASTC compression
                compression_success = CompressTextureASTC(raw_pixels, width, height, 4,
                    asset_info.shipped_path.string(),
                    compression_format, desc_file.import_settings);
            }

            //Clean up
            stbi_image_free(raw_pixels);

            //Verify output
            if (compression_success && std::filesystem::exists(asset_info.shipped_path)) {
                std::cout << "Texture compiled successfully: " << asset_info.shipped_path.filename() << std::endl;
            }
            else {
                std::cout << "ERROR: Texture compilation failed for: " << asset_info.raw_path.filename() << std::endl;
            }
        }

        void Compiler::compileAudio(Descriptor const& desc_file, Info& asset_info) const {
            //To be implemented
        }

        void Compiler::compileModel(Descriptor const& desc_file, Info& asset_info) const {
            //To be implemented
        }

        std::string Compiler::GetCuttlefishExecutable() const {
            // Try to find cuttlefish executable
            std::filesystem::path cuttle_fish_path = assets_root.parent_path() / "vendor/cuttlefish/cuttlefish.exe";
            std::cout << cuttle_fish_path << std::endl;

            //Check if exists
            if (std::filesystem::exists(cuttle_fish_path)) {
                return cuttle_fish_path.string();
            }
        

            std::cout << "WARNING: Cuttlefish executable not found!" << std::endl;
            return "cuttlefish"; // Fallback
        }

        bool Compiler::CompressTextureDDS(unsigned char* pixels, int width, int height, int channels,
            const std::string& output_path, const std::string& format,
            const nlohmann::json& settings) const {
            try {
                std::string cuttlefish_exe = GetCuttlefishExecutable();

                // Create temporary PNG file (easier for cuttlefish to handle)
                std::string temp_input = "temp_" + std::to_string(getCurrentTimeStamp()) + ".png";
                if (!stbi_write_png(temp_input.c_str(), width, height, 4, pixels, width * 4)) {
                    std::cout << "Failed to write temporary PNG" << std::endl;
                    return false;
                }

                // WINDOWS SYSTEM() REQUIRES SPECIAL QUOTING [web:1061]
                std::stringstream cmd;

                // Method: Wrap ENTIRE command in outer quotes for Windows system()
                cmd << "\"";  // Start outer quotes
                cmd << "\"" << cuttlefish_exe << "\"";  // Quoted executable
                cmd << " -i \"" << temp_input << "\"";
                cmd << " -f " << format;
                cmd << " -Q " << settings.value("quality", "normal");
                cmd << " -s rgba";
                cmd << " -o \"" << output_path << "\"";
                cmd << " --file-format dds";
                cmd << " --create-dir";

                if (settings.value("generate_mipmaps", true)) {
                    cmd << " -m";
                }

                cmd << "\"";  // End outer quotes

                std::string final_command = cmd.str();
                std::cout << "Running: " << final_command << std::endl;

                int result = system(final_command.c_str());

                // Clean up temp file
                std::filesystem::remove(temp_input);

                if (result == 0) {
                    std::cout << "DDS compression successful" << std::endl;
                }
                else {
                    std::cout << "DDS compression failed with code: " << result << std::endl;
                }

                return result == 0;
            }
            catch (const std::exception& e) {
                std::cout << "Cuttlefish DDS compression failed: " << e.what() << std::endl;
                return false;
            }
        }


        bool Compiler::CompressTextureASTC(unsigned char* pixels, int width, int height, int channels,
            const std::string& output_path, const std::string& format,
            const nlohmann::json& settings) const {
            try {
                // Build cuttlefish ASTC command
                std::string cuttlefish_cmd = "cuttlefish";
                cuttlefish_cmd += " -f " + format;          // ASTC_4x4, ASTC_6x6, etc.
                cuttlefish_cmd += " -q " + settings.value("quality", "medium");
                cuttlefish_cmd += " -o \"" + output_path + "\"";
                cuttlefish_cmd += " -i rgba8";
                cuttlefish_cmd += " -s " + std::to_string(width) + "x" + std::to_string(height);

                // ASTC-specific options
                if (settings.value("hdr", false)) {
                    cuttlefish_cmd += " --hdr";
                }

                // Create temporary raw input
                std::string temp_input = "temp_astc_" + std::to_string(getCurrentTimeStamp()) + ".raw";
                std::ofstream temp_file(temp_input, std::ios::binary);
                temp_file.write(reinterpret_cast<char*>(pixels), width * height * channels);
                temp_file.close();

                cuttlefish_cmd += " \"" + temp_input + "\"";

                std::cout << "Running ASTC: " << cuttlefish_cmd << std::endl;
                int result = system(cuttlefish_cmd.c_str());

                // Clean up
                std::filesystem::remove(temp_input);

                return result == 0;
            }
            catch (const std::exception& e) {
                std::cout << "Cuttlefish ASTC compression failed: " << e.what() << std::endl;
                return false;
            }
        }


		void Compiler::processAsset(Info& asset_info) {

            //Check for desc files and output
            auto asset_desc_path = assets_root / asset_info.relative_folder / (asset_info.raw_path.stem().string() + desc_ext);

            //Get desc obj
            Descriptor desc_obj;

            //Check if desc exists
            if (!std::filesystem::exists(asset_desc_path)) {
                desc_obj = createDefaultDesc(asset_info, asset_desc_path);
            }
            else {
                desc_obj = readDescFile(asset_info, asset_desc_path);
            }

            //Update asset GUID
            asset_info.guid = desc_obj.guid;

            //Check if asset is compilable
            if (Assets::isAssetCompilable(asset_info.type)) {

                //Compiling operation
                compileAndShip(desc_obj, asset_info);
            }
            else {

                //If asset is not compilable ship asset straight into 
                asset_info.shipped_path = output_dir / asset_info.relative_folder / asset_info.name;

                // Only ship if source is SIGNIFICANTLY newer than destination
                if (asset_info.raw_last_modified > (getFileLastModified(asset_info.shipped_path) + TOLERANCE_MS)) {

                    //Copy all assets
                    copyFile(asset_info.raw_path, asset_info.shipped_path);
                }
            }
		}
	}
}