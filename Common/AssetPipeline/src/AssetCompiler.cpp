#include "AssetCompiler.h"

namespace PAIN {
	namespace Assets {


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
                settings["android_compression"] = "ASTC";
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

            //Get current timestamp in milliseconds since epoch
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto current_timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

            //Extract asset name from path
            std::string asset_name = asset.raw_path.stem().string();

            // Create default descriptor
            Descriptor desc;

            // Identity
            desc.guid = GUID::Generate();
            desc.descriptor_version = 1;
            desc.created_timestamp = current_timestamp;

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